[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$adapter = Get-Content -LiteralPath (Join-Path $projectRoot 'src\ue4ss\main_1_6.cpp') -Raw
$fingerprint = Get-Content -LiteralPath (Join-Path $projectRoot 'src\platform\windows_fingerprint.cpp') -Raw
$fingerprintHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\windows_fingerprint.hpp') -Raw
$eventGate = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\callback_generation.hpp') -Raw
$actionEvidence = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\action_evidence.hpp') -Raw
$cmake = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw
$nativeBuilder = Get-Content -LiteralPath (Join-Path $projectRoot 'tools\Build-Native.ps1') -Raw
$types = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\types.hpp') -Raw
$config = Get-Content -LiteralPath (Join-Path $projectRoot 'config\default.ini') -Raw
$lua = Get-Content -LiteralPath (Join-Path $projectRoot 'Scripts\main.lua') -Raw
$installerEngine = Get-Content -LiteralPath (Join-Path $projectRoot 'installer\InstallerEngine110.cs') -Raw
$installerForm = Get-Content -LiteralPath (Join-Path $projectRoot 'installer\InstallerForm.cs') -Raw
$installerTest = Get-Content -LiteralPath (Join-Path $projectRoot 'tools\Test-Installer110.ps1') -Raw
$selectorResolver = Get-Content -LiteralPath (Join-Path $projectRoot 'src\core\selector_resolver.cpp') -Raw
$selectorResolverHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\selector_resolver.hpp') -Raw
$peRuntime = Get-Content -LiteralPath (Join-Path $projectRoot 'src\core\pe_runtime.cpp') -Raw
$peRuntimeHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\pe_runtime.hpp') -Raw
$asyncLogger = Get-Content -LiteralPath (Join-Path $projectRoot 'src\platform\async_logger.cpp') -Raw
$asyncLoggerHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'include\dsnap\async_logger.hpp') -Raw
$interactionContract = Get-Content -LiteralPath `
    (Join-Path $projectRoot 'metadata\interaction-contract.json') -Raw | ConvertFrom-Json
$buildFingerprints = Get-Content -LiteralPath `
    (Join-Path $projectRoot 'metadata\build-fingerprints.json') -Raw | ConvertFrom-Json

function Assert-ContainsOrdinal {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][string]$Value,
        [Parameter(Mandatory)][string]$Description
    )
    if ($Text.IndexOf($Value, [StringComparison]::Ordinal) -lt 0) {
        throw "$Description is missing: $Value"
    }
}

function Assert-DoesNotContain {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][string]$Value,
        [Parameter(Mandatory)][string]$Description
    )
    if ($Text.IndexOf($Value, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
        throw "$Description contains a forbidden route: $Value"
    }
}

function Get-BracedBlock {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][string]$Signature,
        [Parameter(Mandatory)][string]$Description
    )

    $signatureIndex = $Text.IndexOf($Signature, [StringComparison]::Ordinal)
    if ($signatureIndex -lt 0) {
        throw "$Description signature is missing: $Signature"
    }
    $openBraceIndex = $Text.IndexOf('{', $signatureIndex)
    if ($openBraceIndex -lt 0) {
        throw "$Description opening brace is missing."
    }

    $depth = 0
    for ($index = $openBraceIndex; $index -lt $Text.Length; ++$index) {
        if ($Text[$index] -eq '{') {
            ++$depth
        } elseif ($Text[$index] -eq '}') {
            --$depth
            if ($depth -eq 0) {
                return $Text.Substring($signatureIndex, $index - $signatureIndex + 1)
            }
        }
    }
    throw "$Description closing brace is missing."
}

foreach ($value in @(
    'void set_tick_sequence(std::uint64_t tick_sequence) noexcept;',
    'std::uint64_t sequence{};',
    'std::int64_t utc_unix_ms{};',
    'std::uint64_t session_elapsed_ms{};',
    'std::uint64_t tick_sequence{};',
    'std::chrono::system_clock::now()',
    'std::chrono::steady_clock::now()',
    '++next_sequence_',
    'current_tick_sequence_.load(std::memory_order_acquire)',
    'timestamp_utc(utc_unix_ms)',
    'sequence=" << sequence',
    'utc_unix_ms=" << utc_unix_ms',
    'session_elapsed_ms=" << session_elapsed_ms',
    'tick_sequence=" << tick_sequence')) {
    Assert-ContainsOrdinal ($asyncLogger + $asyncLoggerHeader) $value `
        'Enqueue-time correlated logger contract'
}

foreach ($value in @(
    'constexpr auto kVersion = STR("1.3.0")',
    'DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0',
    'runtime_reflection_dual_caller_rel32_consensus_fail_closed',
    '/Script/DS.DInteractableComponent:Server_RunInteractV2',
    '/Script/DS.DInteractableComponent:SetInteractUIV2',
    'function->GetFuncPtr()',
    'decode_leaf_thunk_addresses(',
    'dsnap::resolve_virtual_dispatch_slot(',
    'dsnap::follow_direct_jump_chain(loaded.text, virtual_entry)',
    'resolve_reflected_direct_function(loaded, set_interact_ui_function)',
    'dsnap::select_unique_terminal_rel32_call(',
    'dsnap::resolve_selector_call(',
    'dsnap::resolve_ui_selector_call(',
    'server_selector.selector_address != ui_selector.selector_address',
    'decode_runtime_function_addresses(',
    'runtime_function.canonical_root.begin != implementation',
    'dsnap::parse_pe32_plus_loaded_image(',
    'dsnap::collect_x64_chained_runtime_fragments(',
    'function_property(set_interact_ui_function, STR("InteractActor"))',
    'set_interact_ui_parameter_bytes == sizeof(UObject*)',
    'interact_actor_property->GetOffset_Internal() == 0',
    'ui_schema_advisory_valid',
    'REFLECTION_CONTRACT_DETAIL',
    'required_failure={}',
    'invoke_selector_address_guarded(selector_capability_.address',
    'selector(receiver, output, nullptr)',
    'SELECTOR_RESOLVED',
    'SELECTOR_UNAVAILABLE',
    'SELECTOR_FAULTED',
    'parse_toggle_hotkey(configuration_result_.value.toggle_hotkey)',
    'resolve_live_interaction_action(context, &action_resolution',
    'load_game_interaction_binding(path)',
    'game_input_settings_path()',
    'mapped_key == resolved_interaction_key_',
    'INTERACTION_BINDING_RESOLVED',
    'INTERACTION_BINDING_FALLBACK',
    'configuration_result_.value.interaction_key_fallback',
    'reload=each_enable',
    'named_object(pawn, STR("Rider"))',
    '/Script/DS.DropItemActor',
    'EnhancedInputSubsystemInterface:InjectInputVectorForAction',
    'const FVector pressed_value{1.0, 0.0, 0.0}',
    'verify_build_fingerprint(binary_directory())',
    'game_hash_policy=diagnostic_only',
    'register_keydown_event(static_cast<Input::Key>(*toggle_key)',
    'Hook::RegisterEngineTickPostCallback',
    'if (shutting_down_.load(std::memory_order_acquire)) return;',
    'Hook::RegisterInitGameStatePreCallback',
    'constexpr bool kNativeDropItemRangeBridgeEnabled = false;',
    'if constexpr (kNativeDropItemRangeBridgeEnabled)',
    'DROP_ITEM_RANGE_PAK_OWNED',
    'native_multiplier=disabled double_apply_prevented=1',
    'Hook::RegisterBeginPlayPostCallback',
    'DropItemActor.SphereOverlapComp',
    '/Script/Engine.SphereComponent:SetSphereRadius',
    'reason=multiple_recognized_range_paks',
    'application=BeginPlay_exact_reflection object_scans=0',
    'session_events_.record_key_event()',
    'toggle_key_edge_.key_down()',
    'release_toggle_key_if_up();',
    'dsnap::AtomicPhysicalKeyEdge toggle_key_edge_',
    'std::uint8_t toggle_virtual_key_',
    'key_release_rearms_',
    'reason=foreground_mismatch source=UE4SS_configured_keydown',
    'reason=playable_session_not_ready',
    'session_events_.mark_playable(session_generation)',
    'resolve_viewport_world_identity(engine)',
    'PLAYABLE_SESSION_READY',
    'session_events_.publish_toggle_request(key_events.generation)',
    'session_events_.drain_toggle_requests()',
    'const auto session_generation = session_events_.reset()',
    'reason=playable_world_unavailable source=configured_hotkey_toggle',
    'retry_after_world_load=1',
    'playable_world_identity_changed',
    'active_world_identity_.store(current_world_identity',
    'active_world_identity_changed',
    'player_world_identity_changed_during_scan',
    'game_not_foreground_before_injection',
    'automation_enabled_.exchange(false, std::memory_order_acq_rel)',
    'interaction_binding_mode_ = "unresolved"',
    'automation_disabled=1',
    'reenable_required=1',
    'target_field_access=0',
    'direct_RPC=0',
    'SendInput=0',
    'dsnap::AutomaticActionState automatic_action_',
    'automatic_action_.inspect(',
    'automatic_action_.begin(',
    'automatic_action_.confirm(',
    'automatic_action_.expire(',
    'automatic_action_.cancel(',
    'automatic_action_.reset_activation()',
    'reset_action_activation("toggle_on_reset", false)',
    'reset_action_activation("toggle_off", true)',
    'reset_action_activation(source, true)',
    'AutomaticActionDecision::Quarantined',
    'AutomaticActionDecision::FailClosed',
    'kPulseInterval = std::chrono::milliseconds{25}',
    'kActiveScanInterval = std::chrono::milliseconds{25}',
    'kIdleScanInterval = std::chrono::milliseconds{33}',
    'kPostPickupCooldown = std::chrono::milliseconds{25}')) {
    Assert-ContainsOrdinal $adapter $value 'ExperimentalNested selector contract'
}

foreach ($value in @(
    'logger_.set_tick_sequence(tick_sequence);',
    'std::atomic<std::uint64_t> next_tick_sequence_{};',
    'constexpr auto kSlowTickThreshold = std::chrono::microseconds{2000};',
    'constexpr auto kSlowTickLogInterval = std::chrono::seconds{1};',
    '"AUTO_TICK_SLOW"',
    'total_us={} context_us={} selector_us={} validation_us={}',
    'action_resolution_us={} subsystem_us={} injection_us={}',
    'automation_enabled={} pending_action={} window_active={}',
    'elapsed <= kSlowTickThreshold',
    'finished - last_slow_tick_log_ < kSlowTickLogInterval',
    '"PERF_AGGREGATE"',
    '"PERF_TIMING"',
    '"TARGET_SELECTED_DIAGNOSTIC"',
    'distance_unavailable=1',
    'reason=unsafe_runtime_location_read_omitted',
    'diagnostic_only=1 decision_input=0 post_injection=1')) {
    Assert-ContainsOrdinal $adapter $value 'Debug-only correlated timing and distance diagnostics'
}

$slowTickBlock = Get-BracedBlock $adapter `
    'void maybe_log_slow_tick(' `
    'Slow EngineTickPost diagnostic'
Assert-ContainsOrdinal $slowTickBlock `
    'if (!configuration_result_.value.debug_logging) return;' `
    'Slow EngineTickPost debug gate'
$distanceLogBlock = Get-BracedBlock $adapter `
    'void emit_selected_distance_diagnostic(' `
    'Selected-distance diagnostic logger'
Assert-ContainsOrdinal $distanceLogBlock `
    'if (!configuration_result_.value.debug_logging) return;' `
    'Selected-distance debug gate'
if ($adapter.Contains('kActorLocationFunctionPath') -or
    $adapter.Contains('read_selected_distance_diagnostic') -or
    $adapter.Contains('actor_location_function_')) {
    throw 'Debug diagnostics must not resolve or call the Actor location ProcessEvent path.'
}
$injectionIndex = $adapter.IndexOf('if (!inject_live_interaction_action_once(')
$distanceDiagnosticIndex = $adapter.IndexOf('emit_selected_distance_diagnostic(invocation_activation_id')
if ($injectionIndex -lt 0 -or $distanceDiagnosticIndex -lt 0 -or
    $distanceDiagnosticIndex -lt $injectionIndex) {
    throw 'Selected-distance diagnostic enqueue must remain after interaction injection.'
}

foreach ($event in @('PICKUP_ACTION_INVOKED', 'PICKUP_CONFIRMED',
                      'PICKUP_UNCONFIRMED', 'AUTO_SCAN_DEFERRED')) {
    if ($adapter -notmatch ('"' + [regex]::Escape($event) +
            '"[\s\S]{0,700}?request_id=\{\}')) {
        throw "$event must retain request_id for tick_sequence plus request_id correlation."
    }
}

foreach ($value in @(
    'UObjectGlobals::RegisterHook',
    'OnBeginOverlap',
    'OnEndOverlap',
    'AddComponentByClass',
    'FinishAddComponent',
    'K2_DestroyComponent',
    'SetCapsuleSize',
    'SetBoxExtent',
    'OverlapProxy',
    'OVERLAP_PROXY',
    'range_mode',
    'GetOverlappingComponents',
    'UObjectArray::',
    'FindFirstOf',
    'FindAllOf',
    'ForEachUObject',
    'GetAllActorsOfClass',
    'Server_InputInteractKeyAction',
    'keybd_event',
    'mouse_event',
    'std::thread',
    'CreateThread(',
    'RegisterProcessEventPreCallback',
    'expected_game_sha256',
    'not_current_exact_build')) {
    Assert-DoesNotContain $adapter $value 'ExperimentalNested adapter'
}

foreach ($value in @(
    'resolve_reflected_virtual_function(loaded, set_interact_ui_function',
    'ui_virtual_slot_offset',
    'ui_dispatch_candidates')) {
    Assert-DoesNotContain $adapter $value 'SetInteractUIV2 direct-wrapper resolver policy'
}

$reflectionContract = Get-BracedBlock `
    $adapter `
    '[[nodiscard]] bool validate_reflection_contract(' `
    'Reflection contract validator'
foreach ($value in @(
    'else if (!report.ui_schema_advisory_valid)',
    'report.required_valid = report.ui_schema_advisory_valid',
    'return report.ui_schema_advisory_valid')) {
    Assert-DoesNotContain $reflectionContract $value `
        'Advisory SetInteractUIV2 parameter schema must not gate structural selector resolution'
}

foreach ($value in @(
    'kNativeSelectorRva',
    '0x42C0160',
    '69992800')) {
    Assert-DoesNotContain ($adapter + $selectorResolver + $selectorResolverHeader + $peRuntime + $peRuntimeHeader) $value `
        'Active dynamic-selector release path'
}
foreach ($value in @(
    'expected_game_sha256',
    'BuildFingerprint',
    'game_sha256')) {
    Assert-DoesNotContain ($selectorResolver + $selectorResolverHeader) $value `
        'Game-hash-independent selector resolver'
}
foreach ($value in @(
    'collect_direct_rel32_calls(',
    'select_unique_terminal_rel32_call(',
    'TerminalCallNotFound',
    'TerminalCallAmbiguous')) {
    Assert-ContainsOrdinal ($selectorResolver + $selectorResolverHeader) $value `
        'Decoded terminal rel32 direct-wrapper contract'
}
foreach ($value in @(
    'ProcessEvent(server_run_interact_function',
    'ProcessEvent(set_interact_ui_function',
    'RegisterHook(kServerRunInteractFunctionPath',
    'RegisterHook(server_run_interact_function',
    'RegisterHook(kSetInteractUiFunctionPath',
    'RegisterHook(set_interact_ui_function')) {
    Assert-DoesNotContain $adapter $value 'Reflection-only selector anchors'
}

Assert-DoesNotContain $adapter 'automation_preserved' 'World reset lifecycle policy'

foreach ($value in @(
    'CONFIRMATION_SUPERSEDED',
    'scheduler_blocked=0',
    'kRecentTargetCooldown',
    'recent_target_cooldown',
    'recent_target_identity_',
    'recent_target_until_',
    'kToggleDebounce',
    'last_key_event_accepted_')) {
    Assert-DoesNotContain $adapter $value '1.3.0 pending-action and physical-edge policy'
}

foreach ($pattern in @(
    '\bSendInput\s*\(',
    '\bkeybd_event\s*\(',
    '\bmouse_event\s*\(')) {
    if ([regex]::IsMatch($adapter, $pattern, [Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
        throw "ExperimentalNested adapter contains forbidden input synthesis: $pattern"
    }
}

foreach ($value in @(
    'inline constexpr std::size_t kAutomaticActionQuarantineCapacity = 128',
    'kAutomaticActionRetryDelay = std::chrono::milliseconds{100}',
    'kMaximumAutomaticActionAttempts = 2',
    'enum class AutomaticActionDecision',
    'class AutomaticActionState',
    'AutomaticActionDecision::Pending',
    'AutomaticActionDecision::Cooldown',
    'AutomaticActionDecision::Quarantined',
    'AutomaticActionDecision::FailClosed',
    'AutomaticActionDecision inspect(',
    'const auto decision = inspect(candidate, now, distance_meters);',
    'PendingActionTracker pending_',
    'quarantine_',
    'reset_activation()')) {
    Assert-ContainsOrdinal $actionEvidence $value 'Automatic action state contract'
}

if ($interactionContract.lifecycle.confirmation -ne
        'exact_actor_or_component_weak_identity_invalidation_or_exact_component_interactable_state_change' -or
    $interactionContract.lifecycle.timeout -ne
        'first_650ms_timeout_cools_down_100ms_then_one_selector_represented_retry_second_timeout_quarantines_exact_component_for_current_activation' -or
    $interactionContract.lifecycle.interaction_owner_change -ne
        'clear_pending_and_attempt_records_and_enforce_1500ms_settle_before_next_scan' -or
    $interactionContract.automatic_action.pending_policy -ne
        'one_global_pending_blocks_all_later_automatic_invocations_and_tracks_the_exact_returned_component' -or
    $interactionContract.automatic_action.confirmation_window_ms -ne 650 -or
    $interactionContract.automatic_action.engine_pulse_ms -ne 25 -or
    $interactionContract.automatic_action.active_scan_ms -ne 25 -or
    $interactionContract.automatic_action.idle_scan_ms -ne 33 -or
    $interactionContract.automatic_action.post_pickup_ms -ne 25 -or
    $interactionContract.automatic_action.timeout_retry -ne $true -or
    $interactionContract.automatic_action.retry_delay_ms -ne 100 -or
    $interactionContract.automatic_action.maximum_attempts -ne 2 -or
    $interactionContract.automatic_action.terminal_timeout_policy -ne
        'quarantine_exact_component_for_current_activation_after_second_unconfirmed_attempt' -or
    $interactionContract.automatic_action.retry_preflight_policy -ne
        'exact_component_cooldown_and_quarantine_are_checked_before_live_action_mapping_or_subsystem_resolution' -or
    $interactionContract.automatic_action.context_reset_policy -ne
        'interaction_owner_change_clears_pending_and_attempt_records_then_enforces_a_1500ms_settle_before_next_scan' -or
    $interactionContract.automatic_action.general_collision_mutation -ne $false -or
    $interactionContract.automatic_action.drop_item_overlap_mutation -ne
        'structured_PAK_authored_DropItemActor_SphereOverlapComp_RelativeScale3D_native_runtime_mutation_disabled' -or
    $interactionContract.automatic_action.drop_item_range_policy -ne
        'one_owned_range_pak_authors_19_class_proven_type7_drop_overlap_spheres_and_native_runtime_multiplier_is_compile_time_disabled') {
    throw 'Interaction metadata does not match the bounded retry, context reset, and exact drop-item range contract.'
}

if ($buildFingerprints.lifecycle.action_pending_policy -ne
        'one_global_pending_action_blocks_later_invocations_and_tracks_the_exact_returned_component' -or
    $buildFingerprints.lifecycle.confirmation_policy -ne
        'exact_actor_or_component_weak_identity_invalidation_or_exact_component_inactive' -or
    $buildFingerprints.lifecycle.confirmation_window_ms -ne 650 -or
    $buildFingerprints.lifecycle.retry_delay_ms -ne 100 -or
    $buildFingerprints.lifecycle.engine_pulse_ms -ne 25 -or
    $buildFingerprints.lifecycle.active_scan_ms -ne 25 -or
    $buildFingerprints.lifecycle.idle_scan_ms -ne 33 -or
    $buildFingerprints.lifecycle.post_pickup_ms -ne 25 -or
    $buildFingerprints.lifecycle.maximum_attempts -ne 2 -or
    $buildFingerprints.lifecycle.timeout_policy -ne
        'first_timeout_cools_down_then_one_selector_represented_retry_second_timeout_quarantines_exact_component' -or
    $buildFingerprints.lifecycle.retry_preflight_policy -ne
        'exact_component_cooldown_and_quarantine_before_live_action_mapping_or_subsystem_resolution' -or
    $buildFingerprints.lifecycle.interaction_owner_change_policy -ne
        'clear_pending_and_attempt_records_then_enforce_1500ms_settle_before_next_scan' -or
    $buildFingerprints.lifecycle.drop_item_range_policy -ne
        'structured_owned_range_pak_authors_19_type7_DropItemActor_overlap_spheres_native_runtime_multiplier_disabled') {
    throw 'Build-fingerprint metadata does not match the corrective action and range lifecycle.'
}

$invokePickup = Get-BracedBlock `
    $adapter `
    'bool invoke_pickup_unsafe(' `
    'Automatic pickup invocation'
foreach ($value in @(
    'const auto component_object_id = weak_object_id(component_weak);',
    'const auto preflight_decision = automatic_action_.inspect(',
    'automatic_action_.pending_candidate() == component_object_id',
    'automatic_action_.cancel(component_object_id)',
    'const auto owner_settle_until = Clock::now() + kWorldSettleDelay;',
    'next_auto_scan_due_ = world_settle_until_;',
    'return reject("interaction_owner_changed");')) {
    Assert-ContainsOrdinal $invokePickup $value `
        'Exact-component retry and interaction-owner settle contract'
}
$preflightIndex = $invokePickup.IndexOf(
    'const auto preflight_decision = automatic_action_.inspect(',
    [StringComparison]::Ordinal)
$actionMappingIndex = $invokePickup.IndexOf(
    'resolve_live_interaction_action(context, &action_resolution,',
    [StringComparison]::Ordinal)
$subsystemIndex = $invokePickup.IndexOf(
    'resolve_enhanced_input_subsystem(',
    [StringComparison]::Ordinal)
if ($preflightIndex -lt 0 -or $actionMappingIndex -lt 0 -or $subsystemIndex -lt 0 -or
    $preflightIndex -ge $actionMappingIndex -or $preflightIndex -ge $subsystemIndex) {
    throw 'Exact-component retry preflight must run before live action mapping and subsystem resolution.'
}

$engineTick = Get-BracedBlock $adapter 'void engine_tick_post(' 'Game-thread scheduler'
$ownerFailureIndex = $engineTick.IndexOf(
    'if (failure_reason == "interaction_owner_changed")',
    [StringComparison]::Ordinal)
$genericBackoffIndex = $engineTick.IndexOf(
    'if (retry_no_candidate)',
    [StringComparison]::Ordinal)
if ($ownerFailureIndex -lt 0 -or $genericBackoffIndex -lt 0 -or
    $ownerFailureIndex -ge $genericBackoffIndex) {
    throw 'Interaction-owner reset must return before generic scheduler backoff can overwrite its settle deadline.'
}

$dropRangeContract = Get-BracedBlock `
    $adapter `
    'void resolve_drop_item_range_contract() noexcept' `
    'Drop-item range reflection contract'
foreach ($value in @(
    'DS_PickupRangeX3_P.pak',
    'DS_PickupRangeX5_P.pak',
    'DS_PickupRangeX10_P.pak',
    'DS_PickupRangeX15_P.pak',
    'DS_PickupRangeX20_P.pak',
    'selected_count != 1',
    'exact_property<FObjectProperty>',
    'exact_property<FFloatProperty>',
    'exact_property<FBoolProperty>',
    'property_range_fits')) {
    Assert-ContainsOrdinal $dropRangeContract $value `
        'Exact fail-closed DropItemActor range contract'
}

$dropRangeApply = Get-BracedBlock `
    $adapter `
    '[[nodiscard]] bool apply_drop_item_range_unsafe(' `
    'Drop-item range application'
foreach ($value in @(
    'overlap_component->GetOuterPrivate() != actor',
    'overlap_component->GetWorld() != actor->GetWorld()',
    'sphere_radius_instance_property_->GetPropertyValueInContainer(',
    'set_sphere_radius_value_property_->SetPropertyValueInContainer(',
    'set_sphere_radius_update_property_->SetPropertyValueInContainer(parameters.data(), true)',
    'overlap_component->ProcessEvent(set_sphere_radius_function_, parameters.data())',
    '// PROCESS_EVENT_RETURNED_SCALAR_ONLY: no UObject access below this line.')) {
    Assert-ContainsOrdinal $dropRangeApply $value `
        'BeginPlay-only DropItemActor overlap-radius application'
}
$dropRangePostEvent = $dropRangeApply.Substring($dropRangeApply.IndexOf(
    '// PROCESS_EVENT_RETURNED_SCALAR_ONLY: no UObject access below this line.',
    [StringComparison]::Ordinal))
foreach ($value in @('overlap_component->', 'actor->', 'FWeakObjectPtr{')) {
    Assert-DoesNotContain $dropRangePostEvent $value `
        'Drop-item post-ProcessEvent scalar-only boundary'
}

foreach ($value in @(
    'class AtomicPhysicalKeyEdge',
    'down_.compare_exchange_strong(expected, true,',
    'down_.load(std::memory_order_acquire)',
    'down_.exchange(false, std::memory_order_acq_rel)')) {
    Assert-ContainsOrdinal $eventGate $value 'Runtime-used atomic physical-key edge contract'
}

$getAsyncKeyStateMatches = [regex]::Matches($adapter, '\bGetAsyncKeyState\s*\(')
if ($getAsyncKeyStateMatches.Count -ne 1) {
    throw 'ExperimentalNested must contain exactly one GetAsyncKeyState call for key-release detection.'
}
$releaseBlock = Get-BracedBlock $adapter 'void release_toggle_key_if_up() noexcept' 'Toggle-key release detector'
foreach ($value in @(
    'toggle_key_edge_.latched()',
    'if ((GetAsyncKeyState(static_cast<int>(toggle_virtual_key_)) & 0x8000) != 0) return;',
    'if (toggle_key_edge_.key_up()) ++key_release_rearms_')) {
    Assert-ContainsOrdinal $releaseBlock $value 'Narrow toggle-key release detector'
}
if ([regex]::Matches($releaseBlock, '\bGetAsyncKeyState\s*\(').Count -ne 1) {
    throw 'GetAsyncKeyState must appear only once and only inside release_toggle_key_if_up.'
}
$releaseLatchLoadIndex = $releaseBlock.IndexOf(
    'toggle_key_edge_.latched()',
    [StringComparison]::Ordinal)
$releaseProbeIndex = $releaseBlock.IndexOf('GetAsyncKeyState(', [StringComparison]::Ordinal)
$releaseLatchClearIndex = $releaseBlock.IndexOf(
    'toggle_key_edge_.key_up()',
    [StringComparison]::Ordinal)
if ($releaseLatchLoadIndex -lt 0 -or $releaseProbeIndex -lt $releaseLatchLoadIndex -or
    $releaseLatchClearIndex -lt $releaseProbeIndex) {
    throw 'The release detector must guard its sole Win32 probe with the atomic latch before rearming.'
}

$onUpdateIndex = $adapter.IndexOf('void on_update() override', [StringComparison]::Ordinal)
$releaseCallIndex = $adapter.IndexOf('release_toggle_key_if_up();', $onUpdateIndex, [StringComparison]::Ordinal)
$keyDrainIndex = $adapter.IndexOf('session_events_.drain_key_events()', $onUpdateIndex, [StringComparison]::Ordinal)
if ($onUpdateIndex -lt 0 -or $releaseCallIndex -lt $onUpdateIndex -or
    $keyDrainIndex -lt 0 -or $releaseCallIndex -gt $keyDrainIndex) {
    throw 'on_update must rearm the physical key edge before draining configured key events.'
}

$keyRegistrationIndex = $adapter.IndexOf(
    'register_keydown_event(static_cast<Input::Key>(*toggle_key)',
    [StringComparison]::Ordinal)
$keyLatchIndex = $adapter.IndexOf(
    'toggle_key_edge_.key_down()',
    $keyRegistrationIndex,
    [StringComparison]::Ordinal)
$keyRecordIndex = $adapter.IndexOf(
    'session_events_.record_key_event()',
    $keyRegistrationIndex,
    [StringComparison]::Ordinal)
if ($keyRegistrationIndex -lt 0 -or $keyLatchIndex -lt $keyRegistrationIndex -or
    $keyRecordIndex -lt 0 -or $keyLatchIndex -gt $keyRecordIndex) {
    throw 'The UE4SS keydown callback must claim the atomic physical-edge latch before recording a key event.'
}

Assert-DoesNotContain $fingerprint 'expected_game_sha256' 'Runtime fingerprint policy'
Assert-ContainsOrdinal $fingerprint 'result.trusted = expected_ue4ss_sha256(result.ue4ss_sha256);' 'UE4SS ABI fingerprint policy'
Assert-ContainsOrdinal $fingerprint 'GetModuleHandleW(L"UE4SS.dll")' 'Loaded UE4SS module identity policy'
Assert-ContainsOrdinal $fingerprint 'nested / "UE4SS.dll"' 'ExperimentalNested path policy'
Assert-ContainsOrdinal $fingerprint 'if (game) result.game_sha256 = *game;' 'Diagnostic-only game hash policy'
Assert-ContainsOrdinal $eventGate 'class SessionToggleEventGate' 'World-generation event gate'
Assert-ContainsOrdinal $eventGate 'enqueue_for_generation(toggle_requests_, source_generation)' 'Toggle generation gate'
Assert-ContainsOrdinal $eventGate 'class AtomicPhysicalKeyEdge' 'Runtime-used atomic physical-key edge'
Assert-ContainsOrdinal $fingerprintHeader 'std::array<std::string_view, 1>' 'Single supported UE4SS fingerprint policy'
Assert-ContainsOrdinal $fingerprintHeader 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1' 'ExperimentalNested fingerprint'
Assert-DoesNotContain $fingerprintHeader '8AC18FBFFC1EF96B0662D4A2D537B3F224C26D65CAABA7989A9404C566102B26' 'ExperimentalNested-only fingerprint policy'
Assert-DoesNotContain $fingerprint 'verify_build_fingerprint(binary_directory, binary_directory)' 'StableRoot fallback policy'
Assert-ContainsOrdinal $cmake 'PROPERTY STRINGS ExperimentalNested)' 'ExperimentalNested-only CMake policy'
Assert-ContainsOrdinal $cmake 'Version 1.3.0 supports ExperimentalNested only' 'Unsupported CMake ABI rejection'
Assert-ContainsOrdinal $nativeBuilder "[ValidateSet('ExperimentalNested')]" 'ExperimentalNested-only native builder policy'

$worldGuardIndex = $adapter.IndexOf('const auto current_world_identity = resolve_viewport_world_identity(engine);', [StringComparison]::Ordinal)
$confirmationIndex = $adapter.IndexOf('poll_pending_action(now);', [StringComparison]::Ordinal)
if ($worldGuardIndex -lt 0 -or $confirmationIndex -lt 0 -or $worldGuardIndex -gt $confirmationIndex) {
    throw 'Stable World identity must be checked before confirmation evidence is polled.'
}

$activeWorldRecheckIndex = $adapter.IndexOf(
    'const auto active_world_identity =',
    $adapter.IndexOf('bool invoke_pickup_unsafe(', [StringComparison]::Ordinal),
    [StringComparison]::Ordinal)
$selectorCallIndex = $adapter.IndexOf('call_native_selector_guarded(', $activeWorldRecheckIndex,
                                      [StringComparison]::Ordinal)
if ($activeWorldRecheckIndex -lt 0 -or $selectorCallIndex -lt 0 -or
    $activeWorldRecheckIndex -gt $selectorCallIndex) {
    throw 'The fresh Pawn World identity must be compared with the active World before the selector call.'
}

$subsystemFinishIndex = $adapter.IndexOf('subsystem_scope.finish();', [StringComparison]::Ordinal)
$foregroundRecheckIndex = $adapter.IndexOf(
    'const auto injection_foreground = foreground_window_state();',
    [StringComparison]::Ordinal)
$beginActionIndex = $adapter.IndexOf('const auto action_decision = begin_pending_action(', $subsystemFinishIndex,
                                    [StringComparison]::Ordinal)
$injectionIndex = $adapter.IndexOf('inject_live_interaction_action_once(', $beginActionIndex,
                                   [StringComparison]::Ordinal)
if ($subsystemFinishIndex -lt 0 -or $foregroundRecheckIndex -lt 0 -or
    $beginActionIndex -lt 0 -or
    $injectionIndex -lt 0 -or
    $foregroundRecheckIndex -lt $subsystemFinishIndex -or
    $foregroundRecheckIndex -gt $beginActionIndex -or
    $beginActionIndex -gt $injectionIndex) {
    throw 'Foreground must be revalidated before the one global pending action is established and injected.'
}

foreach ($value in @(
    'if (named_object(controller, STR("Player")) != local_player)',
    'if (named_object(pawn, STR("Controller")) != controller)',
    'if (!pawn->GetWorld() || controller->GetWorld() != pawn->GetWorld())')) {
    Assert-ContainsOrdinal $adapter $value 'Bidirectional current-player identity contract'
}

foreach ($value in @(
    'kMaxPlayerContextDebugEventsPerActivation = 32',
    'kMaxSelectorDebugEventsPerActivation = 64',
    'kMaxDeferredDebugEventsPerActivation = 32',
    'debug_context_suppressed_',
    'debug_selector_suppressed_',
    'debug_deferred_suppressed_',
    'reason={} pawn=0x{:X}',
    'scalar_only=1 change_only=1')) {
    Assert-ContainsOrdinal $adapter $value 'Bounded scalar debug attribution contract'
}

$pickupBlock = Get-BracedBlock $adapter 'bool invoke_pickup_unsafe(' 'Guarded pickup implementation'
$scalarOnlyMarkerIndex = $pickupBlock.IndexOf(
    '// PROCESS_EVENT_RETURNED_SCALAR_ONLY: no UObject access below this line.',
    [StringComparison]::Ordinal)
if ($scalarOnlyMarkerIndex -lt 0) {
    throw 'The pickup implementation is missing the post-ProcessEvent boundary marker.'
}
$postInjection = $pickupBlock.Substring($scalarOnlyMarkerIndex)
foreach ($value in @('FWeakObjectPtr{', 'object_full_name(', 'context.', 'action_resolution.',
                     '*interact_type', '*interactable', 'enhanced_input_subsystem',
                     'live_actor', 'live_component', 'live_player_input',
                     'live_interaction_action', 'live_subsystem', 'selected.')) {
    Assert-DoesNotContain $postInjection $value 'Post-ProcessEvent scalar-only logging boundary'
}
foreach ($value in @(
    'const auto receiver_identity = pack_weak_identity(receiver_weak);',
    'const auto player_input_identity = pack_weak_identity(player_input_weak);',
    'const auto interaction_action_identity = pack_weak_identity(interaction_action_weak);',
    'const auto invocation_activation_id = action_activation_id_;',
    'const bool pending_unchanged = action_activation_id_ == invocation_activation_id')) {
    Assert-ContainsOrdinal $pickupBlock $value 'Pre-ProcessEvent scalar snapshot and reentrancy contract'
}

$subsystemBlock = Get-BracedBlock $adapter `
    '[[nodiscard]] UObject* resolve_enhanced_input_subsystem(' `
    'Enhanced Input subsystem resolver'
$subsystemProcessEventIndex = $subsystemBlock.IndexOf(
    'subsystem_library_cdo_->ProcessEvent(', [StringComparison]::Ordinal)
$subsystemLifecycleCheckIndex = $subsystemBlock.IndexOf(
    'shutting_down_.load(std::memory_order_acquire)', [StringComparison]::Ordinal)
$subsystemReturnReadIndex = $subsystemBlock.IndexOf(
    'return_property->GetObjectPropertyValue(', [StringComparison]::Ordinal)
if ($subsystemProcessEventIndex -lt 0 -or $subsystemLifecycleCheckIndex -lt $subsystemProcessEventIndex -or
    $subsystemReturnReadIndex -lt $subsystemLifecycleCheckIndex) {
    throw 'Subsystem ProcessEvent must check scalar lifecycle state before interpreting its UObject return.'
}
foreach ($value in @(
    'action_activation_id_ != expected_activation_id',
    'active_world_identity_.load(std::memory_order_acquire) != expected_world_identity',
    'return_property->GetObjectPropertyValue(')) {
    Assert-ContainsOrdinal $subsystemBlock $value 'Subsystem ProcessEvent reentrancy and reflected-object contract'
}
Assert-DoesNotContain $subsystemBlock 'std::memcpy(&subsystem' 'Subsystem reflected-object return contract'

foreach ($value in @(
    'exact_property<FObjectProperty>(',
    'exact_property<FObjectPtrProperty>(',
    'exact_mapping_action_property(',
    'exact_property<FClassProperty>(',
    'write_object_function_parameter(',
    'write_class_function_parameter(',
    'property->SetObjectPropertyValue(address, value);',
    'property->GetObjectPropertyValue(address) == value;')) {
    Assert-ContainsOrdinal $adapter $value 'Exact reflected object-parameter type and accessor contract'
}
Assert-DoesNotContain $adapter 'CastField<FObjectPropertyBase>' 'Broad reflected object-property cast'
Assert-DoesNotContain $adapter 'FObjectPropertyBase* action_property' 'Broad mapped-action property storage'
Assert-ContainsOrdinal $adapter '"enhanced_action_mapping_action_property_class_unsupported"' 'Mapped-action exact storage rejection'
Assert-ContainsOrdinal $adapter 'action_property_storage={}' 'Mapped-action storage diagnostic'
Assert-DoesNotContain $adapter 'write_function_parameter(' 'Untyped reflected parameter writer'
$objectParameterWriter = Get-BracedBlock $adapter `
    '[[nodiscard]] static bool write_object_function_parameter(' `
    'Exact object parameter writer'
$classParameterWriter = Get-BracedBlock $adapter `
    '[[nodiscard]] static bool write_class_function_parameter(' `
    'Exact class parameter writer'
Assert-DoesNotContain $objectParameterWriter 'std::memcpy' 'Object parameter writer raw storage write'
Assert-DoesNotContain $classParameterWriter 'std::memcpy' 'Class parameter writer raw storage write'

if ([regex]::Matches($installerEngine,
        'new OwnedPayloadContract \{ Version = "1\.2\.0"').Count -ne 1) {
    throw 'The installer must contain exactly one evidenced 1.2.0 owned-payload contract.'
}
Assert-ContainsOrdinal $installerEngine '9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6' 'Deployed 1.2.0 upgrade identity'
Assert-ContainsOrdinal $installerEngine '6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1' 'Released 1.3.0 repair identity'
Assert-ContainsOrdinal $installerEngine '4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F' 'Installed 1.3.0 reflection-candidate repair identity'
Assert-ContainsOrdinal $installerEngine '01A6E1358FBFE0B9DB35B4ECAAB2F1F08425265F47D6B1873872AA56D9E002AC' 'Owner-accepted 1.3.0 action-property repair identity'
Assert-ContainsOrdinal $installerEngine '09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B' 'Installed 1.3.0 confirmation-window tuning identity'
Assert-ContainsOrdinal $installerTest '09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B' 'Installed 1.3.0 confirmation-window tuning ownership test'
Assert-ContainsOrdinal $installerEngine '5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08' 'Immediate installed predecessor migration identity'
Assert-ContainsOrdinal $installerTest '5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08' 'Immediate installed predecessor migration test'
foreach ($value in @(
    'private const string OwnershipSchema = "2";',
    'private const string OwnershipId = "8F4282F9-25C4-4EFC-A150-1C4A812C85B4";',
    'MatchesRecordedOwnedPayload(',
    'Plugin SHA-256',
    'Lua SHA-256',
    'Notices SHA-256')) {
    Assert-ContainsOrdinal $installerEngine $value 'Durable recorded ownership contract'
}
Assert-ContainsOrdinal $installerTest 'tampered-recorded-payload' 'Recorded payload tamper rejection test'
Assert-DoesNotContain $installerEngine 'DAEAC64E3FEE280653E300741E5B2FD40CE219C6F243DE32C333F167CE8DCBBD' 'Unproven owned payload allowlist'
Assert-ContainsOrdinal $installerTest "Assert (-not [bool]`$unproven120)" 'Unproven 1.2.0 ownership rejection test'
foreach ($value in @(
    'internal bool CanRepair { get; set; }',
    'internal string InstalledVersion { get; set; }',
    'state.CanRepair = string.Equals(',
    'state.CanUpgrade = !state.CanRepair;',
    'Repair is available.')) {
    Assert-ContainsOrdinal $installerEngine $value 'Installer Repair state contract'
}
foreach ($value in @(
    '_installationState.CanRepair',
    '? "Repair"',
    '? "Upgrade"',
    ': "Install"')) {
    Assert-ContainsOrdinal $installerForm $value 'Installer Repair UI contract'
}
foreach ($value in @(
    "Assert ([bool](Prop `$State 'CanRepair'))",
    "Assert (-not [bool](Prop `$State 'CanUpgrade'))",
    "Assert (-not [bool](Prop `$State 'CanRepair'))",
    'Owned repair preserves config and is backup-free')) {
    Assert-ContainsOrdinal $installerTest $value 'Installer Repair test contract'
}

$enabledGuardIndex = $adapter.IndexOf(
    'if (!automation_enabled_.load(std::memory_order_acquire)) return;',
    [StringComparison]::Ordinal)
$foregroundIndex = $adapter.IndexOf(
    'const auto foreground = foreground_window_state();',
    $enabledGuardIndex,
    [StringComparison]::Ordinal)
if ($enabledGuardIndex -lt 0 -or $foregroundIndex -lt 0) {
    throw 'The enabled scheduler and foreground boundaries are missing.'
}
$schedulerBlock = $adapter.Substring($enabledGuardIndex, $foregroundIndex - $enabledGuardIndex)
if ($schedulerBlock -notmatch 'if\s*\(automatic_action_\.pending\(\)\)\s*\{[\s\S]*?return;[\s\S]*?\}') {
    throw 'The game-thread scheduler must return before selector work while one automatic action is pending.'
}

if ([regex]::Matches($adapter, [regex]::Escape('automatic_action_.reset_activation()')).Count -ne 1) {
    throw 'Automatic action reset must have one lifecycle wrapper implementation.'
}

if ([regex]::Matches($adapter, [regex]::Escape('selector(receiver, output, nullptr)')).Count -ne 1) {
    throw 'ExperimentalNested must contain exactly one native selector callsite.'
}
if ([regex]::Matches($adapter,
        [regex]::Escape('invoke_selector_address_guarded(selector_capability_.address')).Count -ne 1) {
    throw 'ExperimentalNested must source its only native selector call from the resolved capability.'
}
$capabilityGateIndex = $adapter.IndexOf(
    'const bool allowed = selector_capability_.resolved()',
    [StringComparison]::Ordinal)
$enableStoreIndex = $adapter.IndexOf(
    'automation_enabled_.store(true, std::memory_order_release)',
    [StringComparison]::Ordinal)
if ($capabilityGateIndex -lt 0 -or $enableStoreIndex -lt 0 -or
    $capabilityGateIndex -ge $enableStoreIndex) {
    throw 'A resolved process selector capability must gate F9 before automation can be enabled.'
}
$fingerprintGateIndex = $adapter.IndexOf(
    'if (!apply_build_fingerprint_once())',
    [StringComparison]::Ordinal)
$firstReflectionIndex = $adapter.IndexOf(
    'drop_item_class_ = UObjectGlobals::StaticFindObject',
    [StringComparison]::Ordinal)
if ($fingerprintGateIndex -lt 0 -or $firstReflectionIndex -lt 0 -or
    $fingerprintGateIndex -ge $firstReflectionIndex) {
    throw 'The exact UE4SS hash/path gate must run before any reflection or selector-resolution ABI call.'
}
if ([regex]::Matches($adapter, [regex]::Escape('"AUTOMATION_AVAILABLE"')).Count -ne 1) {
    throw 'Automation availability must be published only after ABI and selector capability resolution.'
}
if ([regex]::Matches($adapter, [regex]::Escape('load_game_interaction_binding(path)')).Count -ne 1) {
    throw 'ExperimentalNested must resolve the saved semantic INTERACT binding exactly once per enable path.'
}
if ([regex]::Matches($adapter, [regex]::Escape('register_keydown_event(static_cast<Input::Key>(*toggle_key)')).Count -ne 1) {
    throw 'ExperimentalNested must contain exactly one configuration-derived toggle-key registration.'
}

foreach ($value in @(
    'kNormalGatherInteractType = 2',
    'kTreasureBoxInteractType = 4',
    'kAnimalInteractType = 5',
    'kDropItemInteractType = 7',
    'interact_type == kDropItemInteractType && actor_is_drop_item')) {
    Assert-ContainsOrdinal $types $value 'Closed target-policy contract'
}

if ($config -match '(?im)^\s*(range|radius|multiplier|proxy)[^=]*=' -or
    $lua -match '(?i)proxy|overlap[_ -]?proxy|range[_ -]?multiplier') {
    throw 'Public runtime files must not configure or claim a native range multiplier.'
}
if ($config -notmatch '(?m)^enabled_on_launch=false\r?$' -or
    $config -notmatch '(?m)^automatic_pickup=true\r?$' -or
    $config -notmatch '(?m)^toggle_hotkey=F9\r?$' -or
    $config -notmatch '(?m)^interaction_key=AUTO\r?$' -or
    $config -notmatch '(?m)^interaction_key_fallback=F\r?$' -or
    $config -notmatch '(?m)^debug_logging=false\r?$') {
    throw 'Public runtime defaults do not match the release contract.'
}
if ($lua -notmatch 'DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0') {
    throw 'Lua release marker does not match 1.3.0.'
}

Write-Output 'EXPERIMENTAL_NESTED_SELECTOR_AND_DROP_RANGE_VALIDATION PASSED'
Write-Output 'GAME_HASH_POLICY DIAGNOSTIC_ONLY'
Write-Output 'UE4SS_ABI_POLICY PINNED'
Write-Output 'HOTKEY_SOURCE UE4SS_KEY_CALLBACK'
Write-Output 'HOTKEY_EDGE UE4SS_KEYDOWN_PLUS_WIN32_RELEASE'
Write-Output 'AUTOMATIC_ACTION_PENDING_GLOBAL 1'
Write-Output 'AUTOMATIC_ACTION_TIMEOUT_RETRY BOUNDED_1'
Write-Output 'SELECTOR_CALLS_PER_DUE_SCAN 1'
Write-Output 'OVERLAP_HOOKS 0'
Write-Output 'DROP_ITEM_RANGE_STRUCTURED_PAK_TARGETS 19'
Write-Output 'DROP_ITEM_RANGE_NATIVE_MULTIPLIER 0'
Write-Output 'GENERAL_RANGE_SCANS 0'
Write-Output 'TREASURE_POLICY EXCLUDED'
