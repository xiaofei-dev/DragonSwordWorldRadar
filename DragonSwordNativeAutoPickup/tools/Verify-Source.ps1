[CmdletBinding()]
param([switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$adapter = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'src\ue4ss\main_0_9.cpp')
$lua = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Scripts\main.lua')
$cmake = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt')
$nativeBuild = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'tools\Build-Native.ps1')

if ($adapter -notmatch '0\.9\.0-native-visibility-f-input' -or
    $adapter -notmatch 'OWNER_AUTHORIZED_NATIVE_VISIBILITY_F_INPUT') {
    throw 'Wrong active adapter version or authorization label.'
}
if ($cmake -notmatch 'add_library\(DragonSwordNativeAutoPickup SHARED src/ue4ss/main_0_9\.cpp\)' -or
    $cmake -match 'add_library\(DragonSwordNativeAutoPickup SHARED src/ue4ss/main(_0_8)?\.cpp\)') {
    throw 'CMake does not select only the 0.9 adapter.'
}

foreach ($pattern in @(
    'FindAllOf', 'FindFirstOf', 'FindObjects\s*\(', 'UObjectArray', 'GetAllActorsOfClass',
    'UObjectGlobals::RegisterHook', 'RegisterProcessEvent', 'RegisterAActorTick', 'RegisterBeginPlay',
    'RegisterEndPlay', 'AddUObjectCreateListener', 'AddUObjectDeleteListener', 'ForEachUObject',
    'DropItemActor', 'ExecuteTargetObject', 'ExecuteTargetComponent', 'Server_RunInteractV2',
    'ExecuteWithDelay', 'ExecuteInGameThread', 'RegisterLoadMap')) {
    if ($adapter -match $pattern) { throw "Forbidden 0.9 runtime pattern: $pattern" }
}

foreach ($pattern in @(
    'kNativeVisibilityRva = 0x61B3AC0', 'kNativeVisibilityPrefix',
    'kCurrentGameSha256', 'signature_matches_guarded',
    'std::make_unique<PLH::x64Detour>', 'native_visibility_detour',
    'original\(widget, component, visible\)', 'record_native_visibility\(component, visible\)',
    'QualifiedReleaseEdge f9_input_', 'GetAsyncKeyState\(VK_F9\)',
    'RegisterEngineTickPostCallback', 'RegisterInitGameStatePreCallback',
    'GetForegroundWindow\(\)', 'GetWindowThreadProcessId', 'GetCurrentProcessId\(\)',
    'MapVirtualKeyW', 'KEYEVENTF_SCANCODE', 'KEYEVENTF_KEYUP',
    'SendInput\(', 'input_pending_\.exchange\(false', 'kInputCooldown',
    'target_visible_\.store\(false', 'current_component_\.store\(0',
    'GET_MODULE_HANDLE_EX_FLAG_PIN', 'RtlDllShutdownInProgress',
    'UnrealInitializer::StaticStorage::bIsInitialized', 'logger_\.flush\(\)',
    'object_scans=0', 'pawn_queries=0', 'direct_rpc=0')) {
    if ($adapter -notmatch $pattern) { throw "Required 0.9 invariant missing: $pattern" }
}

if ([regex]::Matches($adapter, 'SendInput\(').Count -ne 1) {
    throw 'The active adapter must contain exactly one bounded SendInput call.'
}
if ($adapter -match '->ProcessEvent\(') {
    throw 'The 0.9 adapter must not replay a reflected game action.'
}
if ($adapter -match 'std::async|std::future|std::jthread|std::thread') {
    throw 'The active adapter must not own a background thread or future.'
}
if ($cmake -match '/WX-' -or $cmake -notmatch '/W4 /WX /wd4324 /external:anglebrackets /external:W0') {
    throw 'Strict native /W4 /WX is missing.'
}
if ($lua -match 'function\s*\(|Find(All|First)Of|GameViewport|ProcessEvent') {
    throw 'Lua must remain marker-only.'
}

$onUpdate = [regex]::Match($adapter, '(?s)void on_update\(\) override \{(.*?)\r?\n    \}\r?\n\r?\nprivate:')
if (-not $onUpdate.Success) { throw 'Could not isolate on_update.' }
foreach ($forbidden in @('UObject', 'ProcessEvent', 'SendInput', 'GetForegroundWindow',
        'native_detour_', 'StaticFindObject', 'IndexToObject')) {
    if ($onUpdate.Groups[1].Value -match $forbidden) {
        throw "on_update violates scalar/file-only affinity: $forbidden"
    }
}

foreach ($pattern in @('patternSleuthBindLockBytes', 'try \{', 'finally \{',
        'WriteAllBytes\(\$patternSleuthBindLock',
        'Assert-GitCommit \$resolvedUE4SS \$expectedCommits\.UE4SS ''RE-UE4SS after build''')) {
    if ($nativeBuild -notmatch $pattern) { throw "Pinned SDK build invariant missing: $pattern" }
}

$metadata = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'metadata\interaction-contract.json') |
    ConvertFrom-Json
if ($metadata.schema_version -ne 18 -or
    $metadata.status -ne 'OWNER_AUTHORIZED_NATIVE_VISIBILITY_F_INPUT' -or
    $metadata.runtime_acceptance -ne $false -or
    $metadata.automatic_action.function -ne 'Win32.SendInput_F_scan_code_press_release' -or
    $metadata.automatic_action.direct_interaction_rpc -ne $false) {
    throw 'Interaction metadata does not match the 0.9 native visibility input source.'
}

$buildMetadata = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'metadata\build-fingerprints.json') |
    ConvertFrom-Json
if ($buildMetadata.source_build.version -ne '0.9.0-native-visibility-f-input' -or
    $buildMetadata.source_build.deployed -ne $true -or
    $buildMetadata.source_build.runtime_validated -ne $false -or
    $buildMetadata.source_build.main_dll_sha256 -ne '645E1D68171D88FE533F7457F3394F61EC13D888B803846FA0614DEFF260B843') {
    throw 'Build metadata does not match the compiled non-deployed 0.9 artifact.'
}

& (Join-Path $PSScriptRoot 'Verify-Manifest.ps1')
& (Join-Path $PSScriptRoot 'Test-PackageLayout.ps1')
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build-Core.ps1') }

Write-Host 'DragonSwordNativeAutoPickup 0.9.0 source verification passed; runtime acceptance remains pending.'
