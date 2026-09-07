[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$DllPath,
    [Parameter(Mandatory)]
    [ValidateSet('ExperimentalNested')][string]$UE4SSVariant
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$dll = Get-Item -LiteralPath (Resolve-Path -LiteralPath $DllPath).Path
if ($dll.Name -ne 'main.dll' -or $dll.Length -lt 100000) {
    throw 'The native artifact is missing, misnamed, or unexpectedly small.'
}

$bytes = [IO.File]::ReadAllBytes($dll.FullName)
if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
    throw 'The native artifact is not a Windows PE image.'
}
$ascii = [Text.Encoding]::ASCII.GetString($bytes)
$unicode = [Text.Encoding]::Unicode.GetString($bytes)
if (-not $ascii.Contains('DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1') -or
    -not $unicode.Contains('1.3.1') -or
    $ascii -match 'OVERLAP_PROXY|overlap_proxy|SetCapsuleSize' -or
    $unicode -match 'OVERLAP_PROXY|overlap_proxy|SetCapsuleSize') {
    throw 'The DLL identity or no-range/configured-key binary invariant failed.'
}

if ($UE4SSVariant -eq 'ExperimentalNested' -and
    (-not $ascii.Contains('GetAsyncKeyState') -and -not $unicode.Contains('GetAsyncKeyState'))) {
    throw 'The experimental adapter is missing its narrow Win32 key-release detector.'
}
if ($ascii.Contains('keybd_event') -or $unicode.Contains('keybd_event') -or
    $ascii.Contains('mouse_event') -or $unicode.Contains('mouse_event')) {
    throw 'The native artifact contains a forbidden Win32 input-synthesis import.'
}
foreach ($forbidden in @(
    'CONFIRMATION_SUPERSEDED',
    'scheduler_blocked=0',
    'recent_target_cooldown',
    'action_quarantine_capacity_exhausted',
    'quarantine_exact_component',
    'quarantine_size=',
    'activation_quarantine=1')) {
    if ($ascii.Contains($forbidden) -or $unicode.Contains($forbidden)) {
        throw "The native artifact contains a forbidden legacy action-retry marker: $forbidden"
    }
}
foreach ($required in @(
    'PICKUP_ACTION_INVOKED',
    'PICKUP_CONFIRMED',
    'PICKUP_UNCONFIRMED',
    'PICKUP_DISPATCH_OBSERVED',
    'DISPATCH_MARKER_IGNORED',
    'pending_action_policy=one_until_game_dispatch',
    'bounded_retry=1 max_attempts={}',
    'dispatch_observer=Server_RunInteractV2_post activation_quarantine=0',
    'signal=game_Server_RunInteractV2_post',
    'target_match_unproven=1 pickup_success_claim=0',
    'terminal_for_injection=1 same_component_reentry_ms={}',
    'attempt={}/{} cycle_terminal={} automatic_retry={} retry_after_ms={}',
    'recovery_after_ms={} activation_quarantine=0 action_record_size={}',
    'reason=interaction_owner_changed',
    'pending_and_attempt_records=cleared',
    'DROP_ITEM_RANGE_PAK_OWNED',
    'structured_drop_assets=19 native_multiplier=disabled double_apply_prevented=1',
    'action_record_capacity_exhausted',
    'required Server_RunInteractV2 post observer registration failed',
    'runtime_reflection_dual_caller_rel32_consensus_fail_closed',
    'Server_RunInteractV2 reflected virtual implementation + SetInteractUIV2 reflected direct implementation',
    'reflection_exec_runtime_function_decode_failed',
    'terminal_call_not_found',
    'terminal_call_ambiguous',
    'REFLECTION_CONTRACT_DETAIL',
    'ui_schema_advisory_valid=',
    'required_failure=',
    'ui_exec_call_rva=',
    'ObjectPtrProperty',
    'enhanced_action_mapping_action_property_class_unsupported',
    'action_property_storage={}',
    'SELECTOR_RESOLVED',
    'SELECTOR_UNAVAILABLE',
    'SELECTOR_FAULTED')) {
    if (-not $ascii.Contains($required) -and -not $unicode.Contains($required)) {
        throw "The native artifact is missing a required 1.3.1 action-lifecycle marker: $required"
    }
}
$result = [ordered]@{
    variant = $UE4SSVariant
    path = $dll.FullName
    size_bytes = [int64]$dll.Length
    sha256 = (Get-FileHash -LiteralPath $dll.FullName -Algorithm SHA256).Hash
    runtime_label = 'DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1'
    version = '1.3.1'
    hotkey_validation = 'UE4SS_KEYDOWN_PLUS_WIN32_RELEASE'
    selector_resolution_policy = 'RUNTIME_REFLECTION_DUAL_CALLER_REL32_CONSENSUS_FAIL_CLOSED'
    static_validation = 'PASSED'
    gameplay_acceptance = 'NOT_VALIDATED_FOR_EXACT_ARTIFACT'
}
[pscustomobject]$result
