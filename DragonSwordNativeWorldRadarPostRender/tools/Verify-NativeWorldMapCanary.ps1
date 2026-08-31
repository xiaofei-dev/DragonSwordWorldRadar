[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Remove-CppComments {
    param([string]$Text)
    $withoutBlocks = [regex]::Replace($Text, '/\*[\s\S]*?\*/', '')
    return [regex]::Replace($withoutBlocks, '//[^\r\n]*', '')
}

function Read-RequiredFile {
    param([string]$RelativePath)
    $path = Join-Path $projectRoot $RelativePath
    Assert-True (Test-Path -LiteralPath $path -PathType Leaf) `
        "Required world-map atlas file is missing: $RelativePath"
    return Get-Content -LiteralPath $path -Raw
}

function Get-IntegralConstant {
    param([string]$Text, [string]$Name)
    $match = [regex]::Match(
        $Text,
        "\b$([regex]::Escape($Name))\s*=\s*(-?\d+)(?:\.0+)?[uUlL]*\s*;")
    Assert-True $match.Success "Required bounded constant is missing: $Name"
    return [int]$match.Groups[1].Value
}

function Get-MainFunction {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?(?:static\s+)?(?:bool|void)\s+' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "Native owner function was not found: $Name"
    return $match.Value
}

function Get-RendererFunction {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^(?:bool|void|WorldMapLayeringRefreshResult)\s+WorldMapUmgRenderer::' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "World-map renderer function was not found: $Name"
    return $match.Value
}

function Assert-MainLifecycleCall {
    param([string]$Main, [string]$FunctionName, [string]$CallPattern)
    $function = Get-MainFunction $Main $FunctionName
    Assert-True ($function -match $CallPattern) `
        "Native owner lifecycle function $FunctionName is missing the required world-map call."
}

$main = Read-RequiredFile 'src\native\main.cpp'
$renderer = Read-RequiredFile 'src\native\world_map_umg_renderer.cpp'
$rendererHeader = Read-RequiredFile 'src\native\world_map_umg_renderer.hpp'
$visibilityHubHeader = Read-RequiredFile 'src\native\radar_visibility_hub.hpp'
$visibilityConfig = Read-RequiredFile 'config\visibility.ini'
$visibilityParser = Read-RequiredFile 'include\dswros\visibility_config.hpp'
$saveReconciler = Read-RequiredFile 'src\native\native_save_reconciler.cpp'
$areaQuestVisibility = Read-RequiredFile 'include\dswros\area_quest_visibility.hpp'
$objectState = Read-RequiredFile 'include\dswros\object_state.hpp'
$renderProjection = Read-RequiredFile 'include\dswros\render_projection.hpp'
$nativeTests = Read-RequiredFile 'tests\native_state_tests.cpp'
$cmake = Read-RequiredFile 'CMakeLists.txt'
$metadata = Read-RequiredFile 'metadata\release.json' | ConvertFrom-Json

$mainCode = Remove-CppComments $main
$rendererCode = Remove-CppComments $renderer
$headerCode = Remove-CppComments $rendererHeader

Assert-True ($metadata.version -eq '2.1.0') `
    'Release metadata version does not match the full world-map atlas milestone.'
Assert-True ($metadata.world_map_umg_renderer.capacity -eq 1785 `
    -and $metadata.world_map_umg_renderer.atlas_texture_size -eq 2048 `
    -and $metadata.world_map_umg_renderer.umg_host_count -eq 2 `
    -and $metadata.world_map_umg_renderer.umg_image_count -eq 2) `
    'Release metadata does not describe the fixed full-atlas capacity and dual-host/dual-image contract.'
$brushAbiMetadata = $metadata.world_map_umg_renderer.texture_brush_abi
Assert-True ($brushAbiMetadata.reflected_parameter_bytes -eq 9 `
    -and $brushAbiMetadata.local_call_buffer_bytes -eq 16 `
    -and $brushAbiMetadata.evidence -match 'SetBrushFromTexture') `
    'Release metadata does not preserve the reflected-span/local-buffer ABI distinction.'
$currentParentMetadata = $metadata.world_map_umg_renderer.current_parent_witness
Assert-True ($currentParentMetadata.source -eq 'current supplied DLayerMap.ArrayIconInfo' `
    -and $currentParentMetadata.candidate_limit -eq 4096 `
    -and ((@($currentParentMetadata.validation_events) -join ',') -eq `
        'attached_host_reuse,F8_suspend,F7_resume,zoom_settle_reparent') `
    -and $currentParentMetadata.retains_array_element_or_widget -eq $false `
    -and $currentParentMetadata.steady_polling -eq $false) `
    'Release metadata does not describe the bounded event-only current-parent witness.'
$attachTimingMetadata = $metadata.world_map_umg_renderer.attach_timing
Assert-True ($attachTimingMetadata.result_log_field -eq 'attach_total_us' `
    -and $attachTimingMetadata.scope -match 'complete attach_once' `
    -and $attachTimingMetadata.scope -match 'texture import' `
    -and $attachTimingMetadata.scope -match 'UMG attachment' `
    -and $attachTimingMetadata.resume_reuse_semantics -match `
        'do not measure resume or reuse duration' `
    -and $attachTimingMetadata.runtime_acceptance -eq 'pending') `
    'Release metadata does not distinguish complete attach timing from resume/reuse correlation.'
Assert-True ($main -match `
        'kVersion\s*=\s*STR\("2\.1\.0"\)' `
    -and $main -match `
        'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0') `
    'Native source version or runtime label does not match 2.1.0.'
Assert-True ($main -match `
        'version=2\.1\.0 runtime_label=\{\} treasure_catalog=') `
    'Native START diagnostics do not match 2.1.0.'
Assert-True ($main -match 'sharp_tangent_six_piece_pointer' `
    -and $main -match 'map100_full_global_task_minigame_dual_atlas_capacity1785' `
    -and $main -match 'world_map_f8=suspend_collapsed' `
    -and $main -match 'world_map_f7=exact_retained_layer_resume_outside_activity' `
    -and $main -match 'world_map_travel=detach' `
    -and $main -match 'activity_suppression=edge_detach_recreate_both_renderers' `
    -and $main -match 'compact_encounter_sizes=30_27' `
    -and $main -match 'area_quest_definition_snapshot=f7_game_db_main_group_numeric_only' `
    -and $main -match 'area_quest_compact=nearby_prerequisite_proven_plus_runtime' `
    -and $main -match 'area_quest_triggerability=fail_closed_main_group_conditions' `
    -and $main -match 'area_quest_world_map=one_shot_main_group_prerequisite_proof_plus_runtime' `
    -and $main -match 'compact_layering=single_proven_viewport_host' `
    -and $main -match 'compact_encounter_style=four_piece_official_reference' `
    -and $main -match 'encounter_end_recovery=exact_destroyed_class_player_to_current_actor' `
    -and $main -match 'compact_attach=event_candidate_plus_one_bounded_startup_catchup_distinct_replacement_rearm' `
    -and $main -match 'world_map_encounter_sizes=44_34' `
    -and $main -match 'world_map_minigame_size=32' `
    -and $main -match 'world_map_edge_coverage=4x4_all_formal_glyphs' `
    -and $main -match 'world_map_encounter_style=official_reference_simplified_contrast' `
    -and $main -match 'world_map_runtime_delta=current_session_only_if_exact_visible_else_set_world_map_image_deferred' `
    -and $main -match 'world_map_readiness=set_world_map_image_one_shot_serial_matched_budget_rearm' `
    -and $main -match 'world_map_minigames=33_fly_40_mole_10_wave_save_filtered' `
    -and $main -match 'area_quest_completion=exact_catalog_dynamic_event_or_exact_task_actor_then_ten_second_exact_id_end_probe_then_three_bounded_positive_only_save_attempts' `
    -and $main -match 'area_quest_monster_alive=unique_bounded_assault_numeric_link' `
    -and $main -match 'area_quest_time_refresh=first_valid_and_world_hour_edge_transactional_runtime_rescan' `
    -and $main -match 'compact_area_quest_style=translucent_charcoal_rounded_brush_thick_dark_frame_three_white_dots' `
    -and $main -match 'compact_transition_hide=first_invalid_position_sample' `
    -and $main -match 'world_map_layering=current_native_icon_canvas_max_z_exact_owned_zoom_event_retained_host_reparent_stable_geometry_plus_bounded_settle_tail_single_full_attach_fallback' `
    -and $main -match 'world_map_replacement=event_driven_exact_set_image_rearm_nonfatal_layer_mismatch') `
    'READY diagnostics do not expose the current coordinate, task, dual-atlas, and lifecycle policy.'

Assert-True ($main -match '#include "world_map_umg_renderer\.hpp"') `
    'The native owner does not include the world-map UMG renderer.'
Assert-True ($main -match '\bWorldMapUmgRenderer\b') `
    'The native owner does not own the world-map UMG renderer.'
Assert-True ($main -match '#include <Unreal/UObjectArray\.hpp>') `
    'The native owner does not include the UObject lifecycle listener API.'
Assert-True ($main -match 'public FUObjectCreateListener') `
    'DLayerMap creation is not captured through the native UObject listener.'
foreach ($requiredListenerToken in @(
        'AddUObjectCreateListener\(this\)',
        'RemoveUObjectCreateListener\(this\)',
        'NotifyUObjectCreated',
        'OnUObjectArrayShutdown',
        'world_map_listener_pending_',
        'world_map_listener_candidate_',
        'capture_created_encounter_actor_guarded',
        'created_encounter_candidates_',
        'consume_created_encounter_candidates')) {
    Assert-True ($main -match $requiredListenerToken) `
        "The bounded DLayerMap listener is missing: $requiredListenerToken"
}
$createListener = Get-MainFunction $mainCode 'NotifyUObjectCreated'
$consumeWorldMapListener = Get-MainFunction `
    $mainCode 'consume_world_map_listener_candidate'
$captureEncounterCandidate = Get-MainFunction `
    $mainCode 'capture_created_encounter_actor_unsafe'
foreach ($forbiddenListenerWork in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'ProcessEvent',
        'append_log', 'std::format', 'fstream', 'filesystem')) {
    Assert-True ($createListener -notmatch $forbiddenListenerWork) `
        "The global UObject create callback contains forbidden work: $forbiddenListenerWork"
}
Assert-True ($mainCode -match `
        '(?ms)^\s{4}void\s+OnUObjectArrayShutdown\s*\([^)]*\)[^{]*\{[\s\S]*?unregister_object_create_listener\s*\(\s*\)') `
    'UObjectArray shutdown does not unregister the shared object-create listener.'
Assert-True ($mainCode -match `
        'world_map_listener_pending_\.exchange\s*\([^;]*std::memory_order_acq_rel') `
    'EngineTick does not consume the fixed listener slot through its atomic pending gate.'
Assert-True ($consumeWorldMapListener -match `
        'if\s*\(!world_map_listener_pending_\.load\(std::memory_order_acquire\)\)\s*\{\s*return;\s*\}[\s\S]*?scoped_lock\s+lock\{world_map_listener_mutex_\}') `
    'The idle game tick must bypass the world-map listener mutex when no create event is pending.'
Assert-True ($createListener -match `
        'scoped_lock\s+lock\{world_map_listener_mutex_\}[\s\S]*?world_map_listener_candidate_\s*=\s*world_map_weak[\s\S]*?world_map_listener_pending_\.store\(' `
    -and $consumeWorldMapListener -match `
        'scoped_lock\s+lock\{world_map_listener_mutex_\}[\s\S]*?world_map_listener_pending_\.exchange\([\s\S]*?weak\s*=\s*world_map_listener_candidate_' `
    -and $consumeWorldMapListener -match `
        'world_map_listener_candidate_(?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})[\s\S]*?world_map_listener_object_index_\s*=\s*-1' `
    -and $mainCode -match `
        'current_layer->GetWorld\(\)\s*!=\s*expected_world') `
    'The DLayerMap candidate and pending flag must be one mutex-protected mailbox without a lost-event window.'
Assert-True ($mainCode -match 'kExpectedEncounterCount\s*=\s*49' `
    -and $mainCode -match 'std::array<FWeakObjectPtr, kExpectedEncounterCount>' `
    -and $mainCode -match `
        'std::array<std::uint64_t, kExpectedEncounterCount>[\s\S]*?encounter_class_name_keys_' `
    -and $captureEncounterCandidate -match `
        '(?:object_class->GetFName\(\)\.ToUnstableInt\(\)|DSNWRPR_CLASS_NAME_KEY\(object_class\))' `
    -and $captureEncounterCandidate -match `
        'std::find\([\s\S]*?encounter_class_name_keys_' `
    -and $captureEncounterCandidate -notmatch `
        'to_string|to_wstring|std::string|encounter_class_indices_|new\s' `
    -and $mainCode -match 'created_encounter_processed_identity_' `
    -and $mainCode -match 'created_encounter_processed_activation_' `
    -and $mainCode -match 'consume_created_encounter_candidates\(\);') `
    'Newly streamed encounter actors are not captured through one fixed exact-class weak slot per catalog entry.'
$consumeCreatedEncounters = Get-MainFunction $mainCode 'consume_created_encounter_candidates'
foreach ($forbiddenCreatedEncounterWork in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'ProcessEvent',
        'append_log', 'fstream', 'filesystem', 'std::vector', 'new ')) {
    Assert-True ($consumeCreatedEncounters -notmatch $forbiddenCreatedEncounterWork) `
        "Created-encounter queue consumption contains forbidden work: $forbiddenCreatedEncounterWork"
}
Assert-True ([regex]::Matches(
        $cmake, 'src/native/world_map_umg_renderer\.cpp').Count -ge 2) `
    'The world-map renderer must be present in the target and strict source flags.'

$markerCapacity = Get-IntegralConstant `
    $rendererHeader 'kWorldMapUmgMarkerCapacity'
$atlasTextureSize = Get-IntegralConstant `
    $rendererHeader 'kWorldMapAtlasTextureSize'
$maxServiceAttempts = Get-IntegralConstant `
    $main 'kWorldMapMaxServiceAttempts'
$treasureMarkerSize = Get-IntegralConstant $renderer 'kTreasureMarkerSize'
$atlasLayerCount = Get-IntegralConstant `
    $rendererHeader 'kWorldMapAtlasLayerCount'
$maxNativeIconCandidates = Get-IntegralConstant `
    $renderer 'kMaxNativeIconCandidates'
Assert-True ($markerCapacity -eq 1785) `
    "World-map marker capacity must equal 1,506 treasures plus 49 encounters, 83 mini-games, and 147 area quests; found $markerCapacity."
Assert-True ($atlasTextureSize -eq 2048) `
    "World-map atlas texture must remain 2048 square; found $atlasTextureSize."
Assert-True ($maxServiceAttempts -eq 3) `
    "World-map readiness must stop after exactly three service attempts; found $maxServiceAttempts."
Assert-True ($treasureMarkerSize -eq 12) `
    "Atlas treasure glyph width must remain 12 reference pixels; found $treasureMarkerSize."
Assert-True ($rendererCode -match 'kBossMarkerSize\s*=\s*44\.0\s*;' `
    -and $rendererCode -match 'kAssaultMarkerSize\s*=\s*34\.0\s*;' `
    -and $rendererCode -match 'kMiniGameMarkerSize\s*=\s*32\.0\s*;' `
    -and $rendererCode -match 'kAreaQuestMarkerSize\s*=\s*26\.0\s*;') `
    'Atlas Boss, Assault, mini-game, and area-quest glyph sizes do not match the approved compact world-map scale.'
Assert-True ($atlasLayerCount -eq 2 `
    -and $rendererCode -match `
        'kRadarMarkerZ\s*=\s*[\s\S]*?std::numeric_limits<std::int32_t>::max\(\)\s*;' `
    -and $rendererCode -match `
        'restack_hosts_unsafe[\s\S]*?remove_from_parent_[\s\S]*?add_child_to_canvas_[\s\S]*?ZOrderParameters\s+host_z\{kRadarMarkerZ\}') `
    "Both radar atlas hosts must be reinserted after native icons at maximum Z with deterministic internal ordering."
Assert-True ($maxNativeIconCandidates -eq 4096) `
    "Native icon traversal bound changed unexpectedly; found $maxNativeIconCandidates."
Assert-True ($rendererHeader -match `
        'WorldMapUmgMarkerArray\s*=\s*[\s\S]*?std::array<WorldMapUmgMarker,\s*kWorldMapUmgMarkerCapacity>') `
    'The full marker snapshot is not represented by the fixed-capacity numeric array.'
Assert-True ($rendererHeader -match 'std::int64_t\s+id\{\}' `
    -and $rendererHeader -match 'double\s+world_x\{\}' `
    -and $rendererHeader -match 'double\s+world_y\{\}' `
    -and $rendererHeader -match 'WorldMapUmgMarkerKind\s+kind' `
    -and $rendererHeader -match 'bool\s+visible\{true\}') `
    'World-map snapshot entries no longer carry complete numeric identity, position, kind, and visibility data.'
Assert-True ($rendererHeader -match '\bSuspended\b' `
    -and $rendererHeader -match '\bvoid\s+suspend\s*\(' `
    -and $rendererHeader -match '\bresume_suspended\s*\(') `
    'The explicit F8 suspend/F7 resume state contract is incomplete.'
$requiredReadyExpression = [regex]::Match(
    $mainCode,
    '(?s)const\s+bool\s+required_runtime_ready\s*=\s*(?<expression>.*?);').Groups['expression'].Value
Assert-True ($requiredReadyExpression.Length -gt 0) `
    'The required-runtime readiness expression was not found.'
foreach ($optionalZoomToken in @(
        'world_map_zoom_function_', 'world_map_zoom_schema_ready_',
        'world_map_zoom_hook_registered_')) {
    Assert-True ($requiredReadyExpression -notmatch `
            [regex]::Escape($optionalZoomToken)) `
        "The exact world-map zoom hook must remain optional: $optionalZoomToken"
}
Assert-True ($mainCode -match `
        'kWorldMapZoomFunction\s*=\s*STR\("/Script/DSClient\.DPanelWorldMap:OnSliderValueChanged"\)' `
    -and $mainCode -match `
        'world_map_panel_layer_property_\s*=\s*CastField<FObjectPropertyBase>\s*\([\s\S]*?GetPropertyByNameInChain\(L"LayerMap"\)' `
    -and $mainCode -match `
        'world_map_zoom_schema_ready_\s*=\s*world_map_panel_class_[\s\S]*?world_map_zoom_function_[\s\S]*?world_map_panel_layer_property_[\s\S]*?GetOffset_Internal\(\)\s*>=\s*0[\s\S]*?GetSize\(\)[\s\S]*?sizeof\(UObject\*\)') `
    'The optional zoom hook is not bound to the exact DPanelWorldMap function and reflected LayerMap object schema.'
Assert-True ([regex]::Matches(
        $mainCode,
        'UObjectGlobals::RegisterHook\s*\(\s*world_map_zoom_function_').Count -eq 1 `
    -and [regex]::Matches(
        $mainCode,
        'UObjectGlobals::UnregisterHook\s*\(\s*world_map_zoom_function_').Count -eq 1 `
    -and $mainCode -match `
        'if\s*\(\s*world_map_zoom_schema_ready_\s*\)\s*\{\s*try\s*\{[\s\S]*?RegisterHook\s*\(\s*world_map_zoom_function_' `
    -and $mainCode -match `
        'world_map_zoom_hook_registered_\s*=\s*false\s*;') `
    'The optional zoom hook must register and unregister exactly once behind its reflected schema gate.'

$zoomPost = Get-MainFunction $mainCode 'world_map_zoom_post'
$zoomPostUnsafe = Get-MainFunction $mainCode 'world_map_zoom_post_unsafe'
$attachedLayerMatches = Get-RendererFunction `
    $rendererCode 'attached_layer_matches'
Assert-True ($rendererHeader -match `
        '\[\[nodiscard\]\]\s+bool\s+attached_layer_matches\s*\(\s*RC::Unreal::UObject\*\s+current_layer\s*\)\s*const\s+noexcept' `
    -and $attachedLayerMatches -match `
        '!current_layer[\s\S]*?state_\s*!=\s*WorldMapUmgRendererState::Attached[\s\S]*?return\s+false' `
    -and [regex]::Matches(
        $attachedLayerMatches,
        'return\s+layer_\.Get\(\)\s*==\s*current_layer').Count -eq 2) `
    'Exact attached-layer ownership must compare the current object against the retained FWeakObjectPtr in both guarded build paths.'
Assert-True ($zoomPost -match '!context\.Context' `
    -and $zoomPost -match '!enabled_' `
    -and $zoomPost -match 'transition_active_' `
    -and $zoomPost -match 'activity_suppressed_' `
    -and $zoomPost -match '!world_map_zoom_schema_ready_' `
    -and $zoomPost -match '!required_runtime_ready_\.load' `
    -and $zoomPost -match 'shutting_down_\.load' `
    -and $zoomPost -match `
        'world_map_zoom_post_unsafe\s*\(\s*context\.Context\s*\)') `
    'The optional zoom callback does not fail closed before reflected LayerMap access.'
Assert-True ($zoomPostUnsafe -match `
        'panel->IsA\s*\(\s*world_map_panel_class_\s*\)' `
    -and $zoomPostUnsafe -match `
        'world_map_umg_renderer_\.state\(\)[\s\S]*?!=\s*dsnwr::WorldMapUmgRendererState::Attached' `
    -and $zoomPostUnsafe -match `
        'world_map_panel_layer_property_[\s\S]*?ContainerPtrToValuePtr<void>\s*\(\s*panel\s*\)' `
    -and $zoomPostUnsafe -match `
        'world_map_panel_layer_property_[\s\S]*?GetObjectPropertyValue\s*\(\s*layer_value\s*\)' `
    -and $zoomPostUnsafe -match `
        'retained_layer\s*=\s*world_map_layer_candidate_\.Get\(\)' `
    -and $zoomPostUnsafe -match `
        '!current_layer\s*\|\|\s*current_layer\s*!=\s*retained_layer' `
    -and $zoomPostUnsafe -match `
        '!world_map_umg_renderer_\.attached_layer_matches\s*\(\s*current_layer\s*\)' `
    -and $zoomPostUnsafe -match `
        'arm_world_map_layering_refresh\s*\(\s*Clock::now\(\)\s*,\s*WorldMapLayeringTrigger::ZoomChanged\s*\)') `
    'Zoom layering work must accept only the exact retained candidate that the renderer itself owns.'
$zoomOwnershipIndex = $zoomPostUnsafe.IndexOf(
    'world_map_umg_renderer_.attached_layer_matches(',
    [StringComparison]::Ordinal)
$zoomArmIndex = $zoomPostUnsafe.IndexOf(
    'arm_world_map_layering_refresh(', [StringComparison]::Ordinal)
Assert-True ($zoomOwnershipIndex -ge 0 `
    -and $zoomArmIndex -gt $zoomOwnershipIndex) `
    'The zoom callback can arm a settle tail before exact renderer layer ownership is proven.'

$settleDelayBlock = [regex]::Match(
    $mainCode,
    '(?s)constexpr\s+std::array\s+kWorldMapLayeringSettleDelays\s*\{(?<values>.*?)\};')
Assert-True $settleDelayBlock.Success `
    'The bounded world-map layering settle schedule was not found.'
$settleDelays = @([regex]::Matches(
        $settleDelayBlock.Groups['values'].Value,
        'std::chrono::milliseconds\s*\{\s*(\d+)\s*\}') |
    ForEach-Object { [int]$_.Groups[1].Value })
Assert-True ($settleDelays.Count -eq 5 `
    -and (($settleDelays -join ',') -eq '100,250,500,1000,1250')) `
    "Layering geometry samples must remain the bounded 100/250/500/1000/1250 ms schedule; found $($settleDelays -join ',')."

$armLayeringRefresh = Get-MainFunction `
    $mainCode 'arm_world_map_layering_refresh'
Assert-True ($armLayeringRefresh -match `
        'world_map_layering_refresh_pending_\s*=\s*true' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_started_\s*=\s*now' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_attempt_\s*=\s*0' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_due_\s*=\s*now[\s\S]*?\+\s*kWorldMapLayeringSettleDelays\.front\(\)' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_serial_\s*=\s*world_map_candidate_serial_' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_trigger_\s*=\s*trigger') `
    'A layering event does not arm one serial-bound five-deadline settle tail from its event timestamp.'
Assert-True ([regex]::Matches(
        $mainCode, '\barm_world_map_layering_refresh\s*\(').Count -eq 6 `
    -and $mainCode -match `
        'WorldMapLayeringTrigger::SetWorldMapImage' `
    -and $mainCode -match 'WorldMapLayeringTrigger::ZoomChanged' `
    -and $mainCode -match 'WorldMapLayeringTrigger::F7Resume' `
    -and $mainCode -match 'WorldMapLayeringTrigger::Attach') `
    'Layering settle work must be armed only by attach, SetWorldMapImage, exact zoom, or F7 resume events.'

$serviceLayeringRefresh = Get-MainFunction `
    $mainCode 'service_world_map_layering_refresh'
$worldMapZoomPostUnsafe = Get-MainFunction `
    $mainCode 'world_map_zoom_post_unsafe'
Assert-True ($worldMapZoomPostUnsafe -match `
        'attached_layer_matches\s*\([\s\S]*?current_layer[\s\S]*?arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::ZoomChanged' `
    -and $worldMapZoomPostUnsafe -notmatch `
        'world_map_layering_parent_rebuild_consumed_\s*=\s*false') `
    'Wheel zoom must restart only the finite settle tail and must never re-enable the per-candidate full-attach fallback.'
$pendingGateIndex = $serviceLayeringRefresh.IndexOf(
    'if (!world_map_layering_refresh_pending_', [StringComparison]::Ordinal)
$refreshCallIndex = $serviceLayeringRefresh.IndexOf(
    'world_map_umg_renderer_.refresh_layering(',
    [StringComparison]::Ordinal)
$attemptBoundsIndex = $serviceLayeringRefresh.IndexOf(
    'if (attempt_index >= kWorldMapLayeringSettleDelays.size())',
    [StringComparison]::Ordinal)
Assert-True ($pendingGateIndex -ge 0 `
    -and $attemptBoundsIndex -gt $pendingGateIndex `
    -and $refreshCallIndex -gt $pendingGateIndex `
    -and $refreshCallIndex -gt $attemptBoundsIndex `
    -and $serviceLayeringRefresh -match `
        '!world_map_layering_refresh_pending_[\s\S]*?now\s*<\s*world_map_layering_refresh_due_[\s\S]*?return' `
    -and $serviceLayeringRefresh -match `
        '!enabled_[\s\S]*?transition_active_[\s\S]*?activity_suppressed_[\s\S]*?world_map_layering_refresh_serial_[\s\S]*?!=\s*world_map_candidate_serial_[\s\S]*?WorldMapUmgRendererState::Attached[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false[\s\S]*?return' `
    -and $serviceLayeringRefresh -match `
        'attempt_index\s*>=\s*kWorldMapLayeringSettleDelays\.size\(\)[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false[\s\S]*?return' `
    -and $serviceLayeringRefresh -notmatch `
        'while\s*\(attempt_index\s*\+\s*1U\s*<\s*kWorldMapLayeringSettleDelays\.size\(\)' `
    -and $serviceLayeringRefresh -notmatch `
        'world_map_layering_refresh_attempt_\s*=\s*static_cast<std::uint32_t>\(attempt_index\)' `
    -and $serviceLayeringRefresh -match `
        'current_layer\s*=\s*world_map_layer_candidate_\.Get\(\)[\s\S]*?final_attempt[\s\S]*?world_map_umg_renderer_\.refresh_layering\([\s\S]*?current_layer\s*,\s*final_attempt\)' `
    -and $serviceLayeringRefresh -match `
        'restacked\s*=\s*refresh_result[\s\S]*?WorldMapLayeringRefreshResult::Restacked[\s\S]*?reparented\s*=\s*refresh_result[\s\S]*?WorldMapLayeringRefreshResult::Reparented[\s\S]*?retry_later\s*=\s*refresh_result[\s\S]*?WorldMapLayeringRefreshResult::RetryLater[\s\S]*?parent_changed\s*=\s*refresh_result[\s\S]*?WorldMapLayeringRefreshResult::ParentChanged' `
    -and $serviceLayeringRefresh -match `
        'final_attempt\s*=\s*attempt_index\s*\+\s*1U[\s\S]*?>=\s*kWorldMapLayeringSettleDelays\.size\(\)[\s\S]*?refresh_layering\([\s\S]*?current_layer\s*,\s*final_attempt\)[\s\S]*?parent_rebuild_allowed\s*=\s*parent_changed[\s\S]*?reparent_geometry_stability_result\(\)[\s\S]*?==\s*dswros::WorldMapGeometryStabilityResult::Stable[\s\S]*?&&\s*final_attempt[\s\S]*?!world_map_layering_parent_rebuild_consumed_' `
    -and $serviceLayeringRefresh -match `
        'if\s*\(parent_changed\)[\s\S]*?if\s*\(parent_rebuild_allowed\)[\s\S]*?world_map_layering_parent_rebuild_consumed_\s*=\s*true[\s\S]*?world_map_umg_renderer_\.begin_activation\(\)[\s\S]*?reset_world_map_runtime\(true\)[\s\S]*?else\s+if\s*\(final_attempt\)[\s\S]*?else[\s\S]*?\+\+world_map_layering_refresh_attempt_[\s\S]*?return' `
    -and $serviceLayeringRefresh -match `
        'if\s*\(!restacked\s*&&\s*!reparented\s*&&\s*!retry_later\)[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false[\s\S]*?return' `
    -and $serviceLayeringRefresh -match `
        'attempt_index\s*\+\s*1U[\s\S]*?>=\s*kWorldMapLayeringSettleDelays\.size\(\)[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false[\s\S]*?return' `
    -and $serviceLayeringRefresh -match `
        '\+\+world_map_layering_refresh_attempt_[\s\S]*?world_map_layering_refresh_due_\s*=\s*world_map_layering_refresh_started_[\s\S]*?\+\s*kWorldMapLayeringSettleDelays\s*\[[\s\S]*?world_map_layering_refresh_attempt_\s*\]') `
    'The settle-tail service is not serial-bound, one-observation-per-pass, mutation-gated, five-state fail-closed, and capped at the five absolute event-relative deadlines.'
Assert-True ([regex]::Matches(
        $serviceLayeringRefresh,
        'world_map_umg_renderer_\.refresh_layering\s*\(\s*current_layer\s*,\s*final_attempt\s*\)').Count -eq 1 `
    -and [regex]::Matches(
        $mainCode, '\bservice_world_map_layering_refresh\s*\(').Count -eq 2) `
    'The finite settle service must contain one guarded refresh call whose first four due passes cannot permit tree mutation.'
foreach ($forbiddenLayeringPoll in @(
        'FindFirstOf', 'FindAllOf', 'StaticFindObject', 'ArrayIconInfo',
        'MapOverlay', 'sleep_for', 'for (',
        'arm_world_map_layering_refresh(')) {
    Assert-True ($serviceLayeringRefresh -notmatch `
            [regex]::Escape($forbiddenLayeringPoll)) `
        "The due-pass service contains steady discovery, rearming, or unrelated work: $forbiddenLayeringPoll"
}

$restackHosts = Get-RendererFunction $rendererCode 'restack_hosts_unsafe'
$refreshLayering = Get-RendererFunction $rendererCode 'refresh_layering'
Assert-True ($rendererHeader -match `
        'enum\s+class\s+WorldMapLayeringRefreshResult\s*:\s*std::uint32_t\s*\{\s*Restacked\s*,\s*Reparented\s*,\s*RetryLater\s*,\s*ParentChanged\s*,\s*Faulted\s*,?\s*\}' `
    -and $rendererHeader -match `
        '\[\[nodiscard\]\]\s+WorldMapLayeringRefreshResult\s+refresh_layering\s*\(\s*RC::Unreal::UObject\*\s+current_layer\s*,\s*bool\s+allow_tree_mutation\s*\)\s*noexcept' `
    -and $rendererHeader -match `
        '\[\[nodiscard\]\]\s+WorldMapLayeringRefreshResult\s+restack_hosts_unsafe\s*\(\s*RC::Unreal::UObject\*\s+current_layer\s*,\s*bool\s+allow_tree_mutation\s*\)' `
    -and $rendererCode -notmatch `
        '\brestack_hosts_unsafe\s*\(\s*\)') `
    'Every retained-host restack must receive the event-current DLayerMap and report one of the five explicit outcomes.'
Assert-True ($refreshLayering -match `
        'state_\s*!=\s*WorldMapUmgRendererState::Attached[\s\S]*?return\s+WorldMapLayeringRefreshResult::RetryLater' `
    -and [regex]::Matches(
        $refreshLayering,
        'restack_hosts_unsafe\s*\(\s*current_layer\s*,\s*allow_tree_mutation\s*\)').Count -eq 2 `
    -and [regex]::Matches(
        $refreshLayering,
        'result\s*==\s*WorldMapLayeringRefreshResult::RetryLater[\s\S]*?result\s*==\s*WorldMapLayeringRefreshResult::ParentChanged[\s\S]*?return\s+result').Count -eq 2 `
    -and [regex]::Matches(
        $refreshLayering,
        'result\s*!=\s*WorldMapLayeringRefreshResult::Restacked[\s\S]*?result\s*!=\s*WorldMapLayeringRefreshResult::Reparented[\s\S]*?!validate_host_unsafe\s*\(\s*current_layer\s*\)[\s\S]*?state_\s*=\s*WorldMapUmgRendererState::Faulted[\s\S]*?return\s+WorldMapLayeringRefreshResult::Faulted').Count -eq 2) `
    'Layering refresh must preserve restack/reparent/retry outcomes and fault closed after same-layer host validation.'
$restackLayerMismatchIndex = $restackHosts.IndexOf(
    'if (!current_layer || current_layer != layer_.Get())',
    [StringComparison]::Ordinal)
$restackPayloadValidationIndex = $restackHosts.IndexOf(
    'if (!validate_host_payload_unsafe(current_layer, owning_player))',
    [StringComparison]::Ordinal)
Assert-True ($restackHosts -match `
        'if\s*\(\s*!current_layer\s*\|\|\s*current_layer\s*!=\s*layer_\.Get\(\)\s*\)\s*\{\s*return\s+WorldMapLayeringRefreshResult::RetryLater\s*;\s*\}' `
    -and $restackLayerMismatchIndex -ge 0 `
    -and $restackPayloadValidationIndex -gt $restackLayerMismatchIndex) `
    'A replacement-layer mismatch must return RetryLater before retained host payload validation can fault the old renderer session.'
Assert-True ($restackHosts -match `
        '!validate_host_payload_unsafe\s*\(\s*current_layer\s*,\s*owning_player\s*\)[\s\S]*?return\s+WorldMapLayeringRefreshResult::Faulted' `
    -and [regex]::Matches(
        $restackHosts, '\bfind_native_icon_template\s*\(').Count -eq 1 `
    -and [regex]::Matches(
        $restackHosts, '\bcurrent_layer_witnesses_native_parent\s*\(').Count -eq 1 `
    -and $restackHosts -match `
        'witnessed_parent\s*=\s*native_parent_\.Get\(\)[\s\S]*?retained_parent_witnessed[\s\S]*?current_layer_witnesses_native_parent[\s\S]*?if\s*\(\s*!retained_parent_witnessed\s*\)[\s\S]*?!find_native_icon_template' `
    -and $restackHosts -match `
        '!find_native_icon_template\s*\(\s*current_layer\s*,\s*owning_player[\s\S]*?return\s+WorldMapLayeringRefreshResult::RetryLater' `
    -and $restackHosts -match `
        'witnessed_parent\s*=\s*native_icon_template\.parent_canvas' `
    -and $restackHosts -match `
        '!witnessed_parent[\s\S]*?!witnessed_parent->IsA\(canvas_panel_class_\)[\s\S]*?return\s+WorldMapLayeringRefreshResult::RetryLater' `
    -and $restackHosts -match `
        'FWeakObjectPtr\s+witnessed_parent_identity(?:\s*=\s*witnessed_parent|\{\}[\s\S]*?witnessed_parent_identity\s*=\s*witnessed_parent)' `
    -and $restackHosts -match `
        'last_layering_previous_parent_index_\s*=\s*native_parent_\.ObjectIndex' `
    -and $restackHosts -match `
        'last_layering_previous_parent_serial_\s*=\s*native_parent_\.ObjectSerialNumber' `
    -and $restackHosts -match `
        'last_layering_current_parent_index_\s*=\s*witnessed_parent_identity\.ObjectIndex' `
    -and $restackHosts -match `
        'last_layering_current_parent_serial_\s*=\s*witnessed_parent_identity\.ObjectSerialNumber' `
    -and $restackHosts -match `
        'last_layering_parent_changed_\s*=[\s\S]*?previous_parent_index_[\s\S]*?!=\s*last_layering_current_parent_index_[\s\S]*?\|\|[\s\S]*?previous_parent_serial_[\s\S]*?!=\s*last_layering_current_parent_serial_' `
    -and $restackHosts -match `
        'read_live_player_canvas_anchor\s*\([\s\S]*?current_anchor_x[\s\S]*?current_anchor_y[\s\S]*?world_map_geometry_maximum_delta\s*\([\s\S]*?last_layering_geometry_changed_[\s\S]*?if\s*\(last_layering_parent_changed_\s*\|\|\s*last_layering_geometry_changed_\)[\s\S]*?observe_world_map_geometry_sample\s*\([\s\S]*?WorldMapGeometryStabilityResult::Stable[\s\S]*?!allow_tree_mutation[\s\S]*?rebase_world_map_atlas_placement\s*\([\s\S]*?atlas_left_[\s\S]*?atlas_top_[\s\S]*?retained_geometry[\s\S]*?current_geometry' `
    -and $restackHosts -match `
        'native_parent\s*=\s*native_parent_\.Get\(\)' `
    -and $restackHosts -match `
        'native_parent_\s*=\s*witnessed_parent[\s\S]*?atlas_left_\s*=\s*rebased->left[\s\S]*?atlas_top_\s*=\s*rebased->top[\s\S]*?return\s+WorldMapLayeringRefreshResult::Reparented') `
    'Each refresh must prove the retained native Canvas from the current layer before bounded fallback discovery, and rebase hosts only from fresh parent-local geometry.'
$attachmentSpaceGuardIndex = $restackHosts.IndexOf(
    'if (last_layering_parent_changed_ || last_layering_geometry_changed_)',
    [StringComparison]::Ordinal)
$freshParentGeometryIndex = $restackHosts.IndexOf(
    'read_live_player_canvas_anchor(', [StringComparison]::Ordinal)
$stableParentGeometryIndex = $restackHosts.IndexOf(
    'reparent_geometry_stability_result_', [StringComparison]::Ordinal)
$rebaseParentGeometryIndex = $restackHosts.IndexOf(
    'rebase_world_map_atlas_placement(', [StringComparison]::Ordinal)
$firstHostRemovalIndex = $restackHosts.IndexOf(
    'host->ProcessEvent(remove_from_parent_', [StringComparison]::Ordinal)
$changedMutationGuardIndex = $restackHosts.IndexOf(
    'if (!allow_tree_mutation)', $stableParentGeometryIndex,
    [StringComparison]::Ordinal)
$sameMutationGuardIndex = $restackHosts.IndexOf(
    'if (!allow_tree_mutation)', $firstHostRemovalIndex + 1,
    [StringComparison]::Ordinal)
$secondHostRemovalIndex = $restackHosts.IndexOf(
    'host->ProcessEvent(remove_from_parent_', $firstHostRemovalIndex + 1,
    [StringComparison]::Ordinal)
Assert-True ($attachmentSpaceGuardIndex -ge 0 `
    -and $freshParentGeometryIndex -ge 0 `
    -and $freshParentGeometryIndex -lt $attachmentSpaceGuardIndex `
    -and $stableParentGeometryIndex -gt $attachmentSpaceGuardIndex `
    -and $rebaseParentGeometryIndex -gt $stableParentGeometryIndex `
    -and $changedMutationGuardIndex -gt $stableParentGeometryIndex `
    -and $changedMutationGuardIndex -lt $rebaseParentGeometryIndex `
    -and $firstHostRemovalIndex -gt $rebaseParentGeometryIndex `
    -and $firstHostRemovalIndex -gt $attachmentSpaceGuardIndex `
    -and $sameMutationGuardIndex -gt $firstHostRemovalIndex `
    -and $secondHostRemovalIndex -gt $sameMutationGuardIndex `
    -and $restackHosts -match `
        'WorldMapGeometryStabilityResult::Stable[\s\S]*?if\s*\(\s*!allow_tree_mutation\s*\)\s*\{\s*return\s+WorldMapLayeringRefreshResult::ParentChanged[\s\S]*?rebase_world_map_atlas_placement[\s\S]*?host->ProcessEvent\s*\(\s*remove_from_parent_' `
    -and $restackHosts -match `
        'reset_reparent_geometry_stability_sample\(\)[\s\S]*?if\s*\(\s*!allow_tree_mutation\s*\)\s*\{\s*return\s+WorldMapLayeringRefreshResult::RetryLater[\s\S]*?host->ProcessEvent\s*\(\s*remove_from_parent_' `
    -and $renderProjection -match `
        'rebase_world_map_atlas_placement\([\s\S]*?parent_width[\s\S]*?kWorldMapGeometryStabilityTolerance[\s\S]*?return\s+std::nullopt') `
    'A changed native Canvas must prove two stable parent-local geometry samples; only the final sample may mutate hosts, and an extent change must request a fresh atlas.'
$restackLayerLoops = [regex]::Matches(
    $restackHosts,
    'for\s*\(\s*std::size_t\s+layer_index\s*=\s*0\s*;[\s\S]*?layer_index\s*<\s*kWorldMapAtlasLayerCount\s*;[\s\S]*?\+\+layer_index\s*\)')
Assert-True ($atlasLayerCount -eq 2 `
    -and $restackLayerLoops.Count -eq 4 `
    -and [regex]::Matches(
        $restackHosts,
        'host->ProcessEvent\s*\(\s*remove_from_parent_\s*,\s*nullptr\s*\)').Count -eq 2 `
    -and [regex]::Matches(
        $restackHosts,
        '(?:native_parent|witnessed_parent)->ProcessEvent\s*\(\s*add_child_to_canvas_\s*,\s*&add_host\s*\)').Count -eq 2 `
    -and $restackHosts -match `
        'AddChildParameters\s+add_host\s*\{\s*host\s*\}' `
    -and $restackHosts -match `
        'native_parent_slots_\[layer_index\](?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})' `
    -and $restackHosts -match `
        'native_parent_slots_\[layer_index\]\s*=\s*native_slot') `
    'A same-parent restack or changed-parent reparent must move only the two retained atlas hosts, without mutating native icon children.'
Assert-True ($restackHosts -notmatch '(?i)MapOverlay|map_overlay' `
    -and $restackHosts -notmatch `
        '\b(?:build_rle_tga_atlas|NewObject|StaticFindObject|CreateWidget|ImportFileAsTexture2D|clear_children_)\b' `
    -and $rendererCode -notmatch '\bAddChildToOverlay\b' `
    -and $rendererCode -notmatch `
        'map_overlay\s*->\s*ProcessEvent\s*\(\s*add_child') `
    'Layering refresh must retain atlas geometry and never parent radar hosts to the outer MapOverlay.'
$serviceWorldMapAtlas = Get-MainFunction $mainCode 'service_world_map_atlas'
$resetWorldMapRuntime = Get-MainFunction $mainCode 'reset_world_map_runtime'
Assert-True ($serviceWorldMapAtlas -match `
        'reuse_layering_result\s*=\s*world_map_umg_renderer_\.refresh_layering\([\s\S]*?current_layer\s*,\s*false\)' `
    -and $serviceWorldMapAtlas -match `
        'WorldMapLayeringRefreshResult::Restacked[\s\S]*?WorldMapLayeringRefreshResult::Reparented' `
    -and $serviceWorldMapAtlas -match `
        'WorldMapLayeringRefreshResult::RetryLater[\s\S]*?WorldMapLayeringRefreshResult::ParentChanged[\s\S]*?arm_world_map_layering_refresh\s*\(\s*now\s*,\s*WorldMapLayeringTrigger::SetWorldMapImage\s*\)' `
    -and $serviceWorldMapAtlas -match `
        'WorldMapLayeringRefreshResult::RetryLater[\s\S]*?\|\|\s*reuse_layering_result[\s\S]*?WorldMapLayeringRefreshResult::ParentChanged[\s\S]*?\?\s*"bounded_settle_tail"\s*:\s*"none"' `
    -and $serviceWorldMapAtlas -match `
        'if\s*\(attached\)[\s\S]*?world_map_layering_parent_rebuild_consumed_\s*=\s*false[\s\S]*?arm_world_map_layering_refresh\s*\(\s*now\s*,\s*WorldMapLayeringTrigger::Attach\s*\)' `
    -and $serviceWorldMapAtlas -notmatch `
        'parent_rebuild_allowed|world_map_umg_renderer_\.begin_activation\(\)[\s\S]*?reset_world_map_runtime\(true\)' `
    -and $resetWorldMapRuntime -match `
        'world_map_marker_snapshot_built_\s*=\s*false' `
    -and $resetWorldMapRuntime -match `
        'world_map_session_pending_\s*=\s*preserve_candidate\s*&&\s*world_map_candidate_available_') `
    'Same-live-layer reuse must remain observation-only, log both deferred results accurately, and reset the next stable extent-change allowance only after a successful fresh attachment.'
Assert-True ($mainCode -match 'WORLD_MAP_LAYERING_RESTACKED' `
    -and $mainCode -match 'WORLD_MAP_LAYERING_REPARENTED' `
    -and $mainCode -match 'bounded_native_canvas_settle_tail' `
    -and $mainCode -match `
        'attached_layer_restack\s*=\s*same_layer[\s\S]*?WorldMapUmgRendererState::Attached[\s\S]*?attached_layer_matches\s*\(\s*current_layer\s*\)[\s\S]*?if\s*\(attached_layer_restack\)[\s\S]*?arm_world_map_layering_refresh\s*\(\s*Clock::now\(\)\s*,\s*WorldMapLayeringTrigger::SetWorldMapImage\s*\)') `
    'SetWorldMapImage must arm the bounded settle tail only when the renderer owns the exact retained candidate.'
Assert-True ($rendererHeader -match `
        'initialize\s*\(\s*std::filesystem::path\s+atlas_cache_path\s*\)') `
    'The atlas cache path is not supplied once during renderer initialization.'

Assert-True ($rendererHeader -match '\bFWeakObjectPtr\b') `
    'Runtime world-map widget identities must be retained weakly.'
Assert-True ($headerCode -notmatch `
        '(?m)^\s*(?:RC::Unreal::)?UObject\s*\*\s*[A-Za-z_]\w*_\s*(?:\{\})?\s*;') `
    'The world-map renderer retains a raw UObject member across calls.'
Assert-True ($headerCode -notmatch `
        'std::array\s*<\s*(?:RC::Unreal::)?UObject\s*\*') `
    'The world-map renderer retains raw UObject pointers in a fixed container.'
Assert-True ($headerCode -notmatch '\b(?:TObjectPtr|TWeakObjectPtr)\s*<') `
    'Runtime world-map identities must use the audited UE4SS FWeakObjectPtr type only.'
foreach ($requiredWeakIdentity in @(
        'layer_', 'retainer_box_', 'native_parent_')) {
    Assert-True ($rendererHeader -match `
            "FWeakObjectPtr\s+$([regex]::Escape($requiredWeakIdentity))") `
        "The shared atlas weak identity is missing: $requiredWeakIdentity"
}
foreach ($requiredLayerIdentity in @(
        'hosts_', 'widget_trees_', 'root_panels_', 'native_parent_slots_',
        'atlas_images_', 'atlas_image_slots_', 'atlas_textures_')) {
    Assert-True ($rendererHeader -match `
            "FWeakObjectPtr,\s*kWorldMapAtlasLayerCount>\s*$([regex]::Escape($requiredLayerIdentity))") `
        "The dual-atlas weak identity array is missing: $requiredLayerIdentity"
}
Assert-True ($headerCode -notmatch '\bpieces_\b|marker_canvases_' `
    -and $rendererCode -notmatch 'kWorldMapUmgMarkerPieceCount') `
    'The rejected per-marker host/piece pool returned.'

foreach ($requiredApi in @(
        '/Script/DSClient\.DMapPointIconUserWidget',
        '/Script/UMG\.Image',
        'UMG\.Image:SetBrushFromTexture',
        'KismetRenderingLibrary:ImportFileAsTexture2D',
        'CanvasPanel:AddChildToCanvas',
        'Widget:SetVisibility',
        'Widget:RemoveFromParent')) {
    Assert-True ($renderer -match $requiredApi) `
        "The atlas renderer is missing required reflected API: $requiredApi"
}
Assert-True ($rendererCode -match `
        'require_parameters\s*\(\s*1U\s*<<\s*9U\s*,\s*set_brush_from_texture_\s*,\s*9\s*\)' `
    -and $rendererCode -match `
        'require_parameters\s*\(\s*1U\s*<<\s*10U\s*,\s*import_file_as_texture_\s*,\s*32\s*\)') `
    'Texture brush/import ABI gates do not match the audited 9/32-byte reflected parameter spans.'
Assert-True ([regex]::Matches(
        $rendererCode,
        'UObjectGlobals::NewObject<UObject>\s*\(\s*trees\[layer_index\]\s*,\s*image_class_\s*\)').Count -eq 1) `
    'The renderer must use one bounded loop call site to allocate the two atlas UImages.'
Assert-True ([regex]::Matches(
        $rendererCode,
        'blueprint_library->ProcessEvent\s*\(\s*create_widget_').Count -eq 1) `
    'The renderer must use one bounded loop call site to create the two game-native atlas hosts.'
Assert-True ([regex]::Matches(
        $rendererCode,
        'rendering_library->ProcessEvent\s*\(\s*import_file_as_texture_').Count -eq 1) `
    'The renderer must use one bounded loop call site to import the two textures in the explicit attachment path.'
Assert-True ($rendererCode -notmatch 'NewObject<UObject>\s*\([^;]*border' `
    -and $rendererCode -notmatch 'marker_piece_style') `
    'Expanded-map marker UObjects or four-piece glyph creation returned.'

foreach ($requiredAtlasHelper in @(
        'calculate_atlas_bounds', 'atlas_input_fingerprint',
        'build_rle_tga_atlas', 'write_rle_tga', 'draw_chest_glyph',
        'draw_atlas_polygon', 'draw_boss_silhouette',
        'draw_boss_glyph', 'draw_assault_glyph',
        'draw_area_quest_glyph',
        'draw_marker_glyph')) {
    Assert-True ($rendererCode -match "\b$requiredAtlasHelper\s*\(") `
        "The event-only atlas pipeline is missing: $requiredAtlasHelper"
}
Assert-True ($rendererCode -match `
        'std::vector<std::uint32_t>\s+pixels\s*\(\s*pixel_count\s*,\s*kTransparent\s*\)') `
    'The explicit atlas build does not start from one transparent fixed-size pixel surface.'
Assert-True ($rendererCode -match `
        'for\s*\([^)]*index\s*=\s*0[^)]*index\s*<\s*marker_count[\s\S]*?draw_marker_glyph') `
    'The atlas builder does not rasterize every visible marker in the supplied snapshot.'
$atlasBuild = [regex]::Match(
    $rendererCode,
    '(?ms)^\[\[nodiscard\]\]\s+AtlasBuildResult\s+build_rle_tga_atlas\s*\([^;]*?\)\s*noexcept\s*\{(?:(?!^\}).)*^\}').Value
Assert-True ($atlasBuild.Length -gt 0) `
    'The explicit atlas build function was not found.'
Assert-True ([regex]::Matches(
        $atlasBuild, '\bdraw_marker_glyph\s*\(').Count -eq 2 `
    -and $atlasBuild -match `
        'marker_belongs_to_layer\(markers\[index\]\.kind, layer\)' `
    -and $atlasBuild -match `
        'kind\s*==\s*WorldMapUmgMarkerKind::Treasure[\s\S]*?draw_marker_glyph[\s\S]*?kind\s*!=\s*WorldMapUmgMarkerKind::Treasure[\s\S]*?draw_marker_glyph') `
    'The layered atlas does not filter categories and deterministically draw background treasure last.'
Assert-True ($rendererCode -match 'header\[2\]\s*=\s*10U' `
    -and $rendererCode -match 'header\[16\]\s*=\s*32U' `
    -and $rendererCode -match 'header\[17\]\s*=\s*0x28U') `
    'The cache artifact is no longer a 32-bit top-left-origin RLE true-color TGA.'
Assert-True ($rendererCode -match 'run\s*<\s*128U' `
    -and $rendererCode -match '0x80U\s*\|\s*\(run\s*-\s*1U\)' `
    -and $rendererCode -match 'raw_count\s*<\s*128U') `
    'RLE and raw TGA packet lengths are not bounded to the format limit.'
$atlasCache = [regex]::Match(
    $rendererCode,
    '(?ms)^struct\s+AtlasFileCache\s*\{(?:(?!^\};).)*^\};').Value
Assert-True ($atlasCache.Length -gt 0) 'The process-local atlas file cache was not found.'
foreach ($cacheField in @(
        'path_hash', 'input_fingerprint', 'file_bytes',
        'visible_marker_count', 'valid')) {
    Assert-True ($atlasCache -match "\b$cacheField\b") `
        "The atlas file cache is missing numeric field: $cacheField"
}
Assert-True ($atlasCache -notmatch 'UObject|FWeakObjectPtr|time_point|Clock|filesystem::path') `
    'The atlas file cache must retain only numeric fingerprint and size state.'
Assert-True ($rendererCode -match `
        'std::array<AtlasFileCache,\s*kWorldMapAtlasLayerCount>\s*&\s*atlas_file_caches') `
    'The two atlas layers do not own independent numeric file-cache entries.'
Assert-True ([regex]::Matches(
        $rendererCode, '\bbuild_rle_tga_atlas\s*\(').Count -eq 2) `
    'Atlas construction must appear only in its definition and one explicit attach call.'
Assert-True ($main -match `
        'world_map_umg_renderer_\.initialize\s*\(\s*mod_directory\(\)\s*/\s*"runtime"\s*/\s*"cache"\s*/\s*"world-map-treasure-atlas\.tga"\s*\)') `
    'The atlas renderer does not receive the fixed runtime cache path at initialization.'

Assert-True ($rendererCode -match 'ArrayIconInfo' `
    -and $rendererCode -match 'FScriptArrayHelper' `
    -and $rendererCode -match 'IconWidget' `
    -and $rendererCode -match 'find_native_icon_template') `
    'The exact native map-icon parent discovery path is incomplete.'
Assert-True ($rendererCode -match `
        'count\s*<=\s*0\s*\|\|\s*count\s*>\s*kMaxNativeIconCandidates') `
    'The reflected ArrayIconInfo length is not checked against its fixed cap.'
Assert-True ($rendererCode -match 'slot\s*->\s*IsA\s*\(\s*canvas_panel_slot_class' `
    -and $rendererCode -match 'parent\s*->\s*IsA\s*\(\s*canvas_panel_class' `
    -and $rendererCode -match '\bcontent\s*!=\s*icon\b' `
    -and $rendererCode -match 'owning_player\.return_value\s*!=\s*expected_owning_player') `
    'The runtime native-icon template is not gated to the exact slot, parent, content, and player.'
Assert-True ($rendererCode -notmatch '\breinterpret_cast\b[^\r\n]*(?:0x4A0|0x38)' `
    -and $rendererCode -notmatch '\b0x(?:4A0|38)\b') `
    'ArrayIconInfo or IconWidget access regressed to stale hard-coded offsets.'
Assert-True ($rendererCode -notmatch '\bAddChildToOverlay\b' `
    -and $rendererCode -notmatch `
        'map_overlay\s*->\s*ProcessEvent\s*\(\s*add_child') `
    'The rejected MapOverlay host path returned.'

$parentWitness = [regex]::Match(
    $rendererCode,
    '(?ms)^\[\[nodiscard\]\]\s+bool\s+current_layer_witnesses_native_parent\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
Assert-True ($parentWitness.Length -gt 0) `
    'The bounded current-layer native-parent witness helper was not found.'
foreach ($requiredWitnessRelation in @(
        'GetPropertyByNameInChain(L"ArrayIconInfo")',
        'FScriptArrayHelper icons(array_property, array_value)',
        'count <= 0 || count > kMaxNativeIconCandidates',
        'point_panel->IsA(canvas_panel_class)',
        'slot->IsA(canvas_panel_slot_class)',
        'read_object_property(slot, L"Parent") != expected_parent',
        'read_object_property(slot, L"Content") != icon',
        'owning_player.return_value == expected_owning_player')) {
    Assert-True ($parentWitness -match [regex]::Escape($requiredWitnessRelation)) `
        "Current-layer native-parent witness is missing: $requiredWitnessRelation"
}
foreach ($forbiddenWitnessWork in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'NewObject',
        'filesystem', 'fstream', 'ofstream', 'sleep_for', 'retry_deadline',
        '0x4A0', '0x38')) {
    Assert-True ($parentWitness -notmatch [regex]::Escape($forbiddenWitnessWork)) `
        "Current-layer parent validation contains forbidden retained, discovery, file, or scheduled work: $forbiddenWitnessWork"
}
Assert-True ($parentWitness -notmatch `
        '(?m)\bstatic\s+(?:[^;\r\n]*UObject|FWeakObjectPtr|FScriptArrayHelper)') `
    'Current-layer parent validation retains runtime reflection state across events.'
Assert-True ([regex]::Matches(
        $rendererCode, '\bcurrent_layer_witnesses_native_parent\s*\(').Count -eq 3) `
    'Current-layer parent witness must appear only in its definition, exact host validation, and retained-parent refresh selection.'

$validateHost = Get-RendererFunction $rendererCode 'validate_host_unsafe'
foreach ($requiredRelation in @(
        'current_layer != layer_.Get()',
        'read_object_property(current_layer, L"RetainerBox") != retainer',
        'read_object_property(host, L"WidgetTree") != tree',
        'read_object_property(host, L"Panel_Point") != root_panel',
        'read_object_property(host, L"Slot") != native_slot',
        'read_object_property(native_slot, L"Parent") != native_parent',
        'read_object_property(native_slot, L"Content") != host',
        'read_object_property(image, L"Slot") != image_slot',
        'read_object_property(image_slot, L"Parent") != root_panel',
        'read_object_property(image_slot, L"Content") != image')) {
    Assert-True ($validateHost -match [regex]::Escape($requiredRelation)) `
        "Exact atlas host validation is missing: $requiredRelation"
}
Assert-True ($validateHost -match `
        'read_struct_object_property\s*\(\s*image\s*,\s*L"Brush"\s*,\s*L"ResourceObject"\s*\)\s*!=\s*texture') `
    'Exact atlas host validation does not prove the retained texture resource.'
Assert-True ($validateHost -match `
        'host->ProcessEvent\s*\(\s*get_owning_player_\s*,\s*&host_owner\s*\)' `
    -and $validateHost -match `
        'current_layer_witnesses_native_parent\s*\(\s*current_layer\s*,\s*host_owner\.return_value\s*,\s*native_parent') `
    'Exact validation does not require a current ArrayIconInfo witness for the retained native parent and owning player.'

Assert-True ($rendererHeader -match `
        '\[\[nodiscard\]\]\s+std::uint64_t\s+attach_elapsed_us\s*\(\s*\)\s+const\s+noexcept\s*\{\s*return\s+attach_elapsed_us_\s*;\s*\}' `
    -and $rendererHeader -match `
        'std::uint64_t\s+attach_elapsed_us_\s*\{\s*\}\s*;') `
    'The renderer does not expose and retain the complete attach elapsed-time diagnostic.'
$initializeRenderer = Get-RendererFunction $rendererCode 'initialize'
Assert-True ($initializeRenderer -match 'attach_elapsed_us_\s*=\s*0') `
    'Renderer initialization does not reset the complete attach elapsed-time diagnostic.'
$attachOnce = Get-RendererFunction $rendererCode 'attach_once'
Assert-True ([regex]::Matches(
        $attachOnce, 'attach_elapsed_us_\s*=').Count -eq 2) `
    'Attach timing must cover both invalid-input completion and the guarded attach result exactly once.'
$attachTimerStartIndex = $attachOnce.IndexOf(
    'const auto attach_started = std::chrono::steady_clock::now()')
$attachGuardedIndex = $attachOnce.IndexOf('attach_guarded(')
$attachTimerFinishIndex = $attachOnce.LastIndexOf('attach_elapsed_us_ =')
Assert-True ($attachTimerStartIndex -ge 0 `
    -and $attachGuardedIndex -gt $attachTimerStartIndex `
    -and $attachTimerFinishIndex -gt $attachGuardedIndex `
    -and $attachOnce -match `
        'duration_cast<std::chrono::microseconds>\s*\(\s*std::chrono::steady_clock::now\(\)\s*-\s*attach_started\s*\)') `
    'attach_elapsed_us does not measure the complete guarded attach transaction.'
Assert-True ([regex]::Matches(
        $mainCode, 'attach_total_us=\{\}').Count -eq 3 `
    -and [regex]::Matches(
        $mainCode, 'world_map_umg_renderer_\.attach_elapsed_us\s*\(\s*\)').Count -eq 3) `
    'Complete attach timing must be logged exactly for resume, retained-host reuse, and attach result.'

$attach = Get-RendererFunction $rendererCode 'attach_unsafe'
Assert-True ($attach -match `
        'marker_count\s*>\s*kWorldMapUmgMarkerCapacity' `
    -or $rendererCode -match `
        'marker_count\s*>\s*kWorldMapUmgMarkerCapacity') `
    'The full snapshot is not bounded before attachment.'
Assert-True ($rendererCode -match `
        'Widget:GetCachedGeometry' `
    -and $rendererCode -match `
        'CanvasPanelSlot:GetAlignment' `
    -and $rendererCode -match `
        'SlateBlueprintLibrary:GetLocalSize' `
    -and $rendererCode -match `
        'SlateBlueprintLibrary:LocalToAbsolute' `
    -and $rendererCode -match `
        'SlateBlueprintLibrary:AbsoluteToLocal' `
    -and $rendererCode -match `
        'read_live_player_canvas_anchor\s*\([\s\S]*?read_object_property\(slot,\s*L"Content"\)\s*!=\s*player_icon[\s\S]*?player_icon->ProcessEvent\s*\(\s*get_cached_geometry[\s\S]*?expected_native_parent->ProcessEvent\s*\(\s*get_cached_geometry[\s\S]*?local_to_absolute[\s\S]*?absolute_to_local[\s\S]*?validate_world_map_canvas_anchor' `
    -and $rendererCode -match `
        'class\s+GeometryCallParameters[\s\S]*?InitializeValue_InContainer[\s\S]*?DestroyValue_InContainer' `
    -and $attach -match `
        'read_live_player_canvas_anchor\([\s\S]*?player_icon,\s*native_icon_template\.parent_canvas[\s\S]*?last_attach_failure_\s*=\s*24[\s\S]*?return\s+false' `
    -and $attach -match `
        'player_canvas_anchor_x_\s*=\s*player_map_x[\s\S]*?player_canvas_anchor_y_\s*=\s*player_map_y' `
    -and $attach -notmatch `
        'player_map_[xy]\s*=\s*ui_size\s*(?:\*\s*0\.5|/\s*2)' `
    -and $attach -match `
        'project_world_map_point\([\s\S]*?player_map_x,\s*player_map_y[\s\S]*?marker\.world_x[\s\S]*?marker\.world_y[\s\S]*?parent_width\s*,\s*parent_height' `
    -and $renderProjection -match `
        'target\.x\s*-\s*player\.x[\s\S]*?parent_width[\s\S]*?target\.y\s*-\s*player\.y[\s\S]*?parent_height') `
    'World-map projection must convert the player alignment pivot through live Slate geometry into the selected native Canvas and fail closed without a centered fallback.'
Assert-True ($renderProjection -match `
        'world_map_attach_failure_retryable\s*\([\s\S]*?failure\s*>=\s*3U[\s\S]*?failure\s*<=\s*9U[\s\S]*?failure\s*==\s*24U' `
    -and $renderProjection -match `
        'world_map_attach_attempt_is_terminal\s*\([\s\S]*?!world_map_attach_failure_retryable\(failure\)' `
    -and $rendererHeader -match `
        'retryable_not_ready\(\)[\s\S]*?world_map_attach_failure_retryable\([\s\S]*?last_attach_failure_' `
    -and $attachOnce -match `
        'world_map_attach_attempt_is_terminal\([\s\S]*?attached,\s*last_attach_failure_\)[\s\S]*?attach_attempted_\s*=\s*true' `
    -and $nativeTests -match `
        'world_map_attach_attempt_is_terminal\(false,\s*24U\)[\s\S]*?retry latch open' `
    -and $main -match `
        'WORLD_MAP_ATLAS_ATTACHED[\s\S]*?player_anchor_source=\{\}[\s\S]*?player_anchor_x=\{:\.3f\}[\s\S]*?player_anchor_y=\{:\.3f\}[\s\S]*?native_parent_width=\{:\.3f\}[\s\S]*?native_parent_height=\{:\.3f\}' `
    -and $main -match '"slate_geometry"') `
    'World-map Slate-geometry readiness must share the retry policy without latching failure 24, and diagnostics must record the exact converted anchor and parent extent.'
Assert-True ($nativeTests -match `
        'validate_world_map_canvas_anchor\(\s*1940\.0,\s*720\.0,\s*3440\.0,\s*1440\.0' `
    -and $nativeTests -match `
        'validate_world_map_canvas_anchor\(\s*1280\.0,\s*900\.0,\s*2560\.0,\s*1600\.0' `
    -and $nativeTests -match `
        'project_world_map_point\(\s*3145\.620,\s*2559\.683' `
    -and $nativeTests -match `
        'near\(twenty_one_nine_offset->x,\s*3529\.620\)[\s\S]*?near\(twenty_one_nine_offset->y,\s*1791\.683\)' `
    -and $nativeTests -match `
        '3000-to-3840 parent-local extent change must request a fresh atlas') `
    'Native tests must pin the logged 3000-square 16:9 and 3840-square 21:9 parent-local coordinate bases, plus the 16:10 anchor guard.'
$geometryBeginActivation = Get-RendererFunction `
    $rendererCode 'begin_activation'
$geometryBeginMapSession = Get-RendererFunction `
    $rendererCode 'begin_map_session'
$geometryResetRuntimeHandles = Get-RendererFunction `
    $rendererCode 'reset_runtime_handles'
Assert-True ($renderProjection -match `
        'struct\s+WorldMapGeometrySample\s*\{\s*double\s+player_canvas_x[\s\S]*?double\s+player_canvas_y[\s\S]*?double\s+parent_width[\s\S]*?double\s+parent_height' `
    -and $renderProjection -match `
        'kWorldMapGeometryStabilityTolerance\s*=\s*0\.5' `
    -and $renderProjection -match `
        'world_map_geometry_maximum_delta\s*\([\s\S]*?retained\.parent_width\s*<=\s*0\.0[\s\S]*?current\.parent_height\s*<=\s*0\.0[\s\S]*?std::max' `
    -and $renderProjection -match `
        'observe_world_map_geometry_sample\s*\([\s\S]*?if\s*\(!retained_valid\)[\s\S]*?WorldMapGeometryStabilityResult::Seeded[\s\S]*?maximum_delta\s*<=\s*kWorldMapGeometryStabilityTolerance[\s\S]*?WorldMapGeometryStabilityResult::Stable[\s\S]*?WorldMapGeometryStabilityResult::Replaced' `
    -and $attach -match `
        'read_live_player_canvas_anchor\([\s\S]*?observe_world_map_geometry_sample\([\s\S]*?WorldMapGeometryStabilityResult::Stable[\s\S]*?last_attach_failure_\s*=\s*24[\s\S]*?project_world_map_point' `
    -and [regex]::Matches(
        $rendererCode,
        'observe_world_map_geometry_sample\s*\(').Count -eq 2 `
    -and $rendererHeader -match `
        'bool\s+geometry_sample_valid_[\s\S]*?WorldMapGeometrySample\s+geometry_sample_[\s\S]*?WorldMapGeometryStabilityResult\s+geometry_stability_result_[\s\S]*?double\s+geometry_sample_max_delta_' `
    -and $rendererHeader -match `
        'bool\s+reparent_geometry_sample_valid_[\s\S]*?WorldMapGeometrySample\s+reparent_geometry_sample_[\s\S]*?WorldMapGeometryStabilityResult[\s\S]*?reparent_geometry_stability_result_[\s\S]*?double\s+reparent_geometry_sample_max_delta_[\s\S]*?reparent_geometry_parent_index_[\s\S]*?reparent_geometry_parent_serial_' `
    -and $restackHosts -match `
        'reparent_geometry_parent_index_[\s\S]*?last_layering_current_parent_index_[\s\S]*?reparent_geometry_parent_serial_[\s\S]*?last_layering_current_parent_serial_[\s\S]*?reset_reparent_geometry_stability_sample\(\)[\s\S]*?observe_world_map_geometry_sample\([\s\S]*?WorldMapGeometryStabilityResult::Stable[\s\S]*?return\s+WorldMapLayeringRefreshResult::ParentChanged' `
    -and $initializeRenderer -match `
        'reset_geometry_stability_sample\(\)' `
    -and $geometryBeginActivation -match `
        'reset_geometry_stability_sample\(\)' `
    -and $geometryBeginMapSession -match `
        'reset_geometry_stability_sample\(\)[\s\S]*?detach_guarded\(\)' `
    -and $geometryResetRuntimeHandles -match `
        'reset_geometry_stability_sample\(\)' `
    -and $serviceWorldMapAtlas -match `
        'world_map_renderer_session_started_[\s\S]*?begin_map_session\(\)' `
    -and $main -match `
        'kWorldMapMaxServiceAttempts\s*=\s*3U' `
    -and $main -match `
        'kWorldMapServiceRetryDelay\s*=\s*std::chrono::milliseconds\{150\}' `
    -and $main -match `
        'geometry_stability=\{\}[\s\S]*?geometry_sample_max_delta=\{:\.3f\}' `
    -and $main -match `
        'parent_changed=\{\}[\s\S]*?geometry_changed=\{\}[\s\S]*?reparent_geometry_stability=\{\}[\s\S]*?reparent_geometry_sample_max_delta=\{:\.3f\}[\s\S]*?reparent_anchor_delta_x=\{:\.3f\}[\s\S]*?reparent_anchor_delta_y=\{:\.3f\}' `
    -and $nativeTests -match `
        'logged 3000-to-3840 parent-local reflow' `
    -and $nativeTests -match `
        'WorldMapGeometryStabilityResult::Seeded[\s\S]*?WorldMapGeometryStabilityResult::Replaced[\s\S]*?WorldMapGeometryStabilityResult::Stable') `
    'World-map attachment, parent migration, and same-parent aspect reflow must require bounded numeric-only Slate geometry proof, reset it at lifecycle boundaries, and expose stabilization diagnostics without new polling.'
Assert-True ($attach -match `
        'layered_marker_count\s*!=\s*visible_marker_count') `
    'Atlas attachment does not require exact visible-marker raster coverage.'
Assert-True ($attach -match `
        'SetBrushFromTextureParameters\s+brush\{[\s\S]*?atlas_textures\[layer_index\],\s*false\}' `
    -and $attach -match `
        'read_struct_object_property\s*\([\s\S]*?atlas_images\[layer_index\],[\s\S]*?L"Brush",\s*L"ResourceObject"\)\s*!=\s*atlas_textures\[layer_index\]') `
    'Each image brush is not verified against its imported texture.'
Assert-True ($attach -match `
        'native_parent->ProcessEvent\s*\(\s*add_child_to_canvas_' `
    -and $attach -match `
        'read_object_property\s*\(\s*native_slot,\s*L"Parent"\s*\)\s*!=\s*native_parent' `
    -and $attach -match `
        'read_object_property\s*\(\s*native_slot,\s*L"Content"\s*\)\s*!=\s*hosts\[layer_index\]') `
    'The bounded dual live parent mutations are not followed by exact slot validation.'
$publishIndex = $attach.IndexOf('layer_ = current_layer')
$parentMutationIndex = $attach.IndexOf(
    'native_parent->ProcessEvent(add_child_to_canvas_')
Assert-True ($publishIndex -ge 0 -and $parentMutationIndex -gt $publishIndex) `
    'Weak transaction handles must be published before either live parent-tree mutation.'

Assert-True ($renderer -match `
        'L"/Game/Design/CaptureMinimap/Data/WorldMapData_100\.WorldMapData_100"' `
    -and $renderer -match `
        'L"/Game/Design/CaptureMinimap/Data/WorldMapData_200\.WorldMapData_200"') `
    'World-map data lookup must use canonical classless UObject paths.'
Assert-True ($renderer -notmatch `
        '/Script/DSClient\.DWorldMapData''/Game/Design/CaptureMinimap/Data/WorldMapData_') `
    'StaticFindObject cannot consume the rejected export-text map-data path.'
foreach ($requiredCacheToken in @(
        'cached_map_id_', 'cached_map_dimensions_', 'cached_map_ui_size_',
        'map_data_cache_hit_count', 'last_map_data_source')) {
    Assert-True ($renderer + $rendererHeader -match $requiredCacheToken) `
        "The numeric world-map data cache contract is missing: $requiredCacheToken"
}
$resetRuntimeHandles = Get-RendererFunction $rendererCode 'reset_runtime_handles'
foreach ($preservedCacheField in @(
        'cached_map_id_', 'cached_map_dimensions_', 'cached_map_ui_size_')) {
    Assert-True ($resetRuntimeHandles -notmatch $preservedCacheField) `
        "Detach must preserve the pure numeric map-data cache: $preservedCacheField"
}
Assert-True ($rendererCode -match `
        'valid_cached_map_data[\s\S]*?cached_map_id\s*==\s*detected_map_id[\s\S]*?std::isfinite\s*\(\s*cached_dimensions\s*\)[\s\S]*?std::isfinite\s*\(\s*cached_ui_size\s*\)') `
    'The numeric cache fallback is not gated by exact map ID and finite scalars.'

$selection = Get-MainFunction $mainCode 'collect_world_map_marker_snapshot'
Assert-True ($visibilityConfig -match '(?m)^\[radar\]$' `
    -and $visibilityConfig -match '(?m)^bird_eggs=true$' `
    -and $visibilityConfig -match '(?m)^\[map\]$' `
    -and $visibilityConfig -match '(?m)^\[modes\]$' `
    -and $visibilityConfig -match '(?m)^assault=available$' `
    -and $visibilityParser -match 'kMaximumVisibilityConfigBytes\s*=\s*4096U' `
    -and $visibilityHubHeader -match `
        'AreaQuests,[\s\S]*?BirdEggs,[\s\S]*?Count' `
    -and $visibilityHubHeader -match `
        'kRadarVisibilityWorldCategories\s*=\s*0x3EU' `
    -and $selection -notmatch '\bBirdEgg\b' `
    -and $rendererHeader -notmatch '\bBirdEgg\b' `
    -and $rendererCode -notmatch '\bBirdEgg\b') `
    'Bird eggs are compact-only bit 0x40 and must never enter either world-map atlas.'
foreach ($requiredSelection in @(
        'world_map_umg_markers_.fill({})',
        'for (std::size_t index = 0; index < render_catalog_size_; ++index)',
        'compact_eligibility_[index] == 0',
        'dswros::CompactRenderModel::kCompactMapId',
        'entry.id', 'entry.position.x', 'entry.position.y',
        'world_map_treasure_tone(entry.kind)',
        'for (const EncounterSpec& spec : encounter_catalog_)',
        'encounter_visible_for_selected_mode(',
        'WorldMapUmgMarkerKind::Boss',
        'WorldMapUmgMarkerKind::Assault',
        'mini_game_eligibility_[index] == 0',
        'world_map_mini_game_kind(spec.kind)',
        'WorldMapUmgMarkerKind::AreaQuest',
        '!area_quest_visible_for_selected_mode(index)',
        '++world_map_marker_count_')) {
    Assert-True ($selection -match [regex]::Escape($requiredSelection)) `
        "Full eligible map-100 selection is missing: $requiredSelection"
}
Assert-True ($selection -match 'for\s*\(\s*std::size_t index = 0;\s*index < area_quest_catalog_\.size\(\);\s*\+\+index\s*\)') `
    'Full eligible map-100 selection does not iterate the complete area-quest catalog.'
Assert-True ($selection -match 'for\s*\(\s*std::size_t index = 0;\s*index < mini_game_catalog_\.size\(\);\s*\+\+index\s*\)') `
    'Full eligible map-100 selection does not iterate the complete mini-game catalog.'
Assert-True ($selection -match `
        'encounter_visible_for_selected_mode\([\s\S]*?spec,\s*now_unix_seconds\)' `
    -and $selection -notmatch `
        'encounter_available\(|encounter_visible_for_selected_mode_at_hour\(') `
    'Expanded-map Assault selection must use the presentation-only AVAILABLE/ALL helper and must not inline or weaken strict encounter availability.'
Assert-True ($mainCode -match 'world_map_mini_game_kind[\s\S]*?WorldMapUmgMarkerKind::Fly' `
    -and $mainCode -match 'world_map_mini_game_kind[\s\S]*?WorldMapUmgMarkerKind::Mole' `
    -and $mainCode -match 'world_map_mini_game_kind[\s\S]*?WorldMapUmgMarkerKind::Wave') `
    'Mini-game kinds are not mapped into the expanded-map snapshot.'
Assert-True ($selection -match `
        'world_map_marker_count_\s*>=\s*world_map_umg_markers_\.size\s*\(\s*\)') `
    'Full eligible map-100 selection is not bounded by the fixed output array.'
foreach ($forbiddenSelectionWork in @(
        'compact_render_model_.refresh', 'kWorldMapCanaryRadius',
        'std::sort', 'std::partial_sort', 'std::vector', 'new ',
        'UObject', 'ProcessEvent', 'filesystem', 'fstream')) {
    Assert-True ($selection -notmatch [regex]::Escape($forbiddenSelectionWork)) `
        "Full world-map selection contains forbidden truncation or dynamic work: $forbiddenSelectionWork"
}
Assert-True ($mainCode -match `
        'dsnwr::kWorldMapUmgMarkerCapacity[\s\S]*?==\s*1506U\s*\+\s*kExpectedEncounterCount\s*\+\s*kExpectedMiniGameCount\s*\+\s*kExpectedAreaQuestCount') `
    'The owner does not compile-time bind atlas capacity to treasures, encounters, 83 mini-games, and 147 area quests.'
Assert-True ($main -match 'world_count\s*!=\s*1506') `
    'The render-catalog loader no longer validates exactly 1,506 map-100 records.'
Assert-True ($mainCode -match `
        'std::array<dswros::CompactTreasureCatalogEntry,\s*kMaximumTreasureCatalogEntries>\s*render_catalog_entries_') `
    'The full selection source catalog is not retained in a fixed numeric array.'
Assert-True ($mainCode -notmatch '\bworld_map_selected_markers_\b') `
    'The removed two-nearest compact selection buffer returned.'
$service = Get-MainFunction $mainCode 'service_world_map_atlas'
Assert-True ($service -match 'collect_world_map_marker_snapshot\s*\(\s*\)' `
    -and $service -match '!encounter_state_ready_' `
    -and $service -match '!area_quest_state_ready_' `
    -and $service -match `
        'attach_once\s*\([\s\S]*?world_map_umg_markers_[\s\S]*?world_map_marker_count_') `
    'The event service does not attach the complete fixed snapshot and exact count.'
Assert-True ($rendererHeader -match 'WorldMapUmgMarkerKind[\s\S]*?AreaQuest' `
    -and $rendererCode -match 'kOfficialWhite' `
    -and $rendererCode -match 'kOfficialCyan' `
    -and $rendererCode -match 'kAreaQuestBubble' `
    -and $rendererCode -match 'draw_area_quest_glyph' `
    -and $rendererCode -match 'draw_atlas_polygon' `
    -and $rendererCode -notmatch 'kAreaQuestGold') `
    'Official-reference encounter or dialogue area-quest atlas styling is incomplete.'
Assert-True ($rendererHeader -match 'WorldMapUmgMarkerKind[\s\S]*?Fly[\s\S]*?Mole[\s\S]*?Wave' `
    -and $rendererCode -match 'draw_fly_glyph' `
    -and $rendererCode -match 'draw_mole_glyph' `
    -and $rendererCode -match 'draw_wave_glyph' `
    -and $rendererCode -match 'draw_atlas_polygon_aa' `
    -and $rendererCode -match 'sample_axis\s*=\s*4' `
    -and $rendererCode -match 'curve_steps\s*=\s*5' `
    -and $rendererCode -match 'kFlyWing\s*=\s*0xFF9ADBFFU' `
    -and $rendererCode -match 'kHammer\s*=\s*0xFFD29152U' `
    -and $rendererCode -match 'kWave\s*=\s*0xFF325BE0U') `
    'Expanded-map Fly, Mole, and Wave glyph parity is incomplete.'
Assert-True ($rendererCode -match 'kAreaQuestDark\s*=\s*0x84232A2EU' `
    -and $rendererCode -match 'kAtlasStyleRevision\s*=\s*48U' `
    -and $rendererCode -match 'hash_u64\(hash, kAtlasStyleRevision\)') `
    'Translucent area-quest styling must invalidate the event-only atlas cache.'
Assert-True ($rendererCode -match `
        '(?s)void\s+draw_atlas_rectangle\s*\([^}]+draw_atlas_polygon_aa\(' `
    -and $rendererCode -match `
        '(?s)void\s+draw_atlas_diamond\s*\([^}]+draw_atlas_polygon_aa\(' `
    -and $rendererCode -notmatch `
        '(?m)^\s{4,}draw_atlas_polygon\s*\(pixels') `
    'Every production expanded-map rectangle, diamond, and polygon must use bounded edge coverage.'
Assert-True ($areaQuestVisibility -match 'area_quest_world_map_visible' `
    -and $areaQuestVisibility -match 'AreaQuestEligibilityProof::Eligible' `
    -and $areaQuestVisibility -match 'completion_observed' `
    -and $areaQuestVisibility -match 'area_quest_completion_transition' `
    -and $areaQuestVisibility -match 'current_state\s*==\s*AreaQuestState::End' `
    -and $areaQuestVisibility -match 'completed_in_snapshot' `
    -and $mainCode -match 'area_quest_save_completion_' `
    -and $mainCode -match 'completion\.quest_id' `
    -and $saveReconciler -match 'FROM tb_dynamic_quest_complete' `
    -and $saveReconciler -match 'user_identity_ambiguous' `
    -and $saveReconciler -match '!quest_fields->completion_identity_ambiguous' `
    -and $mainCode -match 'dynamic_completion_user_dbid=' `
    -and $mainCode -match 'dynamic_completion_identity_ambiguous=' `
    -and $mainCode -match 'DGameSingleton' `
    -and $mainCode -match 'GameDBTableManager' `
    -and $mainCode -match 'GameDBArray' `
    -and $mainCode -match 'DDynamicQuestDataTable' `
    -and $mainCode -match 'DynamicQuestMainMap' `
    -and $mainCode -match 'DynamicQuestGroupMap' `
    -and $mainCode -match 'capture_area_quest_definitions_guarded' `
    -and $mainCode -match 'evaluate_area_quest_definition' `
    -and $mainCode -match 'area_quest_static_proofs_' `
    -and $saveReconciler -notmatch 'tb_dynamic_quest_group') `
    'World-map area quests are not fail-closed across Main/Group prerequisite proof and the one-shot completion snapshot.'
$requestAreaQuestScan = Get-MainFunction $mainCode 'request_area_quest_scan'
$processOneAreaQuest = Get-MainFunction $mainCode 'process_one_area_quest'
$finishAreaQuestScan = Get-MainFunction $mainCode 'finish_area_quest_scan'
Assert-True ($areaQuestVisibility -match `
        'area_quest_active_sample_may_rearm[\s\S]*?scan_start_completion_revision\s*==\s*current_completion_revision[\s\S]*?&&\s*inactive_state_observed_after_completion' `
    -and $areaQuestVisibility -match `
        'area_quest_inactive_sample_arms_reactivation[\s\S]*?state\s*==\s*AreaQuestState::None[\s\S]*?\|\|\s*state\s*==\s*AreaQuestState::End' `
    -and $requestAreaQuestScan -match `
        'area_quest_scan_start_completion_revisions_\s*=\s*area_quest_exact_completion_revisions_' `
    -and $requestAreaQuestScan -match `
        'area_quest_scan_repeatable_reactivation_armed_\s*=\s*area_quest_repeatable_reactivation_armed_' `
    -and $processOneAreaQuest -match `
        'area_quest_active_sample_may_rearm\(' `
    -and $processOneAreaQuest -match `
        'area_quest_scan_start_completion_revisions_\[index\]' `
    -and $processOneAreaQuest -match `
        'area_quest_exact_completion_revisions_\[index\]' `
    -and $processOneAreaQuest -match `
        'area_quest_scan_repeatable_reactivation_armed_\[index\]' `
    -and $processOneAreaQuest -match `
        'preserve_exact_completion\s*=\s*!scan_revision_is_current\s*\|\|\s*\(decoded_active\s*&&\s*area_quest_scan_completion_observed_\[index\]\s*&&\s*!active_sample_may_rearm\)' `
    -and $processOneAreaQuest -match `
        'area_quest_inactive_sample_arms_reactivation\(' `
    -and $processOneAreaQuest -match `
        'area_quest_scan_repeatable_reactivation_armed_\[index\]\s*=\s*true' `
    -and $processOneAreaQuest -match `
        'area_quest_scan_repeatable_reactivation_armed_\[index\]\s*=\s*false' `
    -and $processOneAreaQuest -match `
        'area_quest_scan_completion_observed_\[index\][\s\S]*?&&\s*active_sample_may_rearm\)\s*\{\s*area_quest_scan_completion_observed_\[index\]\s*=\s*false;\s*area_quest_scan_repeatable_reactivation_armed_\[index\]\s*=\s*false' `
    -and $processOneAreaQuest -match `
        'area_quest_scan_completion_observed_\[index\][\s\S]*?&&\s*dswros::area_quest_inactive_sample_arms_reactivation\(\s*decoded_state\)\)\s*\{[\s\S]*?area_quest_scan_repeatable_reactivation_armed_\[index\]\s*=\s*true' `
    -and $finishAreaQuestScan -match `
        'area_quest_repeatable_reactivation_armed_\s*=\s*area_quest_scan_repeatable_reactivation_armed_' `
    -and $nativeTests -match `
        'post-completion stale progress must remain hidden before an inactive boundary' `
    -and $nativeTests -match `
        'inactive boundary then later active state may rearm a repeatable task' `
    -and $nativeTests -match `
        'only a proven inactive NONE or END sample may arm repeatable reactivation') `
    'World-map exact completion is not fenced from stale active samples until a later NONE/END boundary proves repeatable reactivation.'
$captureTaskClassMap = Get-MainFunction `
    $mainCode 'capture_area_quest_task_class_map_unsafe'
Assert-True ($mainCode -match 'kDynamicQuestTaskUseType\s*=\s*4' `
    -and $captureTaskClassMap -match 'current_game_instance\(engine\)' `
    -and $captureTaskClassMap -match 'TaskActorClassContainer' `
    -and $captureTaskClassMap -match 'DynamicQuestTaskList' `
    -and $captureTaskClassMap -match 'TaskUseType' `
    -and $captureTaskClassMap -match 'UseQuestList' `
    -and $captureTaskClassMap -match 'CreateTaskClass' `
    -and $captureTaskClassMap -match `
        'CastField<FClassProperty>\(' `
    -and $captureTaskClassMap -match `
        'task_use_type\s*!=\s*kDynamicQuestTaskUseType' `
    -and $captureTaskClassMap -match 'task_class->GetFullName\(\)') `
    'The task-ID map must reflect TaskActorClassContainer/DynamicQuestTaskList and filter Quest_Dynamic TaskUseType=4 by full task class name.'
Assert-True ($captureTaskClassMap -match `
        'candidates\.size\(\)\s*!=\s*area_quest_catalog_\.size\(\)' `
    -and $captureTaskClassMap -match `
        'std::count\(covered\.begin\(\),\s*covered\.end\(\),\s*true\)' `
    -and $captureTaskClassMap -match `
        '!catalog_ambiguous_classes\.empty\(\)' `
    -and $captureTaskClassMap -match `
        'catalog_indices_by_id\.contains\(quest_id\)' `
    -and $captureTaskClassMap -match 'duplicate_catalog_id' `
    -and $captureTaskClassMap -match `
        'area_quest_task_class_indices_\s*=\s*std::move\(candidates\)' `
    -and $captureTaskClassMap -match `
        'area_quest_task_class_map_ready_\s*=\s*true') `
    'The task-ID map must fail closed unless every one of the 147 catalog IDs has one exact full-class binding.'

$resetTaskClassCapture = Get-MainFunction `
    $mainCode 'reset_area_quest_task_class_capture'
$resetTaskClassRuntime = Get-MainFunction `
    $mainCode 'reset_area_quest_task_class_runtime'
Assert-True ($resetTaskClassCapture -match `
        'area_quest_task_class_indices_\.clear\(\)' `
    -and $resetTaskClassCapture -match `
        'area_quest_task_class_map_ready_\s*=\s*false' `
    -and $resetTaskClassCapture -match `
        'area_quest_task_class_map_capture_pending_\s*=\s*capture_pending' `
    -and $resetTaskClassCapture -match `
        'area_quest_task_class_map_attempt_count_\s*=\s*0' `
    -and $resetTaskClassCapture -match `
        'area_quest_task_class_map_retry_after_\s*=\s*Clock::time_point\{\}' `
    -and $resetTaskClassCapture -match `
        'area_quest_task_class_row_count_\s*=\s*0' `
    -and $resetTaskClassCapture -match `
        'area_quest_dynamic_task_class_row_count_\s*=\s*0' `
    -and $resetTaskClassCapture -match `
        'area_quest_task_class_ambiguity_count_\s*=\s*0') `
    'The mapping-only reset does not clear every task-class capture result and bounded-retry field.'
Assert-True ($resetTaskClassRuntime -match `
        'reset_area_quest_task_class_capture\(capture_pending\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_exact_completion_bits_' `
    -and $resetTaskClassRuntime -match `
        'word\.store\(\s*0,\s*std::memory_order_release\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_unmapped_completion_requests_\.store\(\s*0' `
    -and $resetTaskClassRuntime -match `
        'area_quest_exact_completion_revisions_\.fill\(0\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_scan_start_completion_revisions_\.fill\(0\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_repeatable_reactivation_armed_\.fill\(false\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_scan_repeatable_reactivation_armed_\.fill\(false\)') `
    'The full lifecycle reset does not own exact-completion bits, revision fences, and repeatable-reactivation state.'
$activateLifecycle = Get-MainFunction $mainCode 'activate'
$transitionEndLifecycle = Get-MainFunction $mainCode 'transition_end'
$disableLifecycle = Get-MainFunction $mainCode 'disable'
$transitionBeginLifecycle = Get-MainFunction $mainCode 'transition_begin'
$disableForMainMenuLifecycle = Get-MainFunction `
    $mainCode 'disable_for_main_menu_owner_boundary'
$worldMapImagePostLifecycle = Get-MainFunction $mainCode 'world_map_image_post'
Assert-True ($worldMapImagePostLifecycle -match `
        '!required_runtime_ready_\.load\(std::memory_order_acquire\)' `
    -and $worldMapImagePostLifecycle -match `
        'shutting_down_\.load\(std::memory_order_acquire\)') `
    'The world-map image hook must fail closed before touching a UObject when required runtime initialization is unavailable or shutdown has begun.'
Assert-True ($transitionBeginLifecycle -match `
        '!required_runtime_ready_\.load\(std::memory_order_acquire\)' `
    -and $transitionEndLifecycle -match `
        '!required_runtime_ready_\.load\(std::memory_order_acquire\)' `
    -and $transitionEndLifecycle -match `
        'object_world_guarded\([\s\S]*?consume_world_map_listener_candidate\(current_world\)[\s\S]*?consume_compact_listener_candidate\(current_world\)' `
    -and $transitionEndLifecycle -match `
        'else\s*\{[\s\S]*?clear_world_map_listener_candidate\(\)[\s\S]*?clear_compact_listener_candidate\(\)' `
    -and $transitionBeginLifecycle -notmatch `
        'clear_world_map_listener_candidate\(\)|clear_compact_listener_candidate\(\)') `
    'Travel must preserve only weak listener mailboxes until transition end validates each candidate against the exact new GameMode world.'
$titleBoundaryIndex = $transitionEndLifecycle.IndexOf(
    'if (is_main_menu_world_identity(current_world_key_))',
    [StringComparison]::Ordinal)
$disabledBoundaryIndex = $transitionEndLifecycle.IndexOf(
    'if (!enabled_ || main_menu_activation_latched_)',
    [StringComparison]::Ordinal)
$worldMapBeginIndex = $transitionEndLifecycle.IndexOf(
    'world_map_umg_renderer_.begin_activation();',
    [StringComparison]::Ordinal)
Assert-True ($titleBoundaryIndex -ge 0 `
    -and $disabledBoundaryIndex -gt $titleBoundaryIndex `
    -and $worldMapBeginIndex -gt $disabledBoundaryIndex `
    -and $transitionEndLifecycle -match `
        'is_main_menu_world_identity\(current_world_key_\)[\s\S]*?disable_for_main_menu_owner_boundary\("transition-end"\)[\s\S]*?return' `
    -and $disableForMainMenuLifecycle -match `
        'disable\(\)[\s\S]*?world_map_umg_renderer_\.detach\(\)[\s\S]*?reset_world_map_runtime\(false\)[\s\S]*?clear_world_map_listener_candidate\(\)' `
    -and $disableForMainMenuLifecycle -match `
        'runtime_opened_treasure_ids_\.clear\(\)[\s\S]*?area_quest_world_map_eligibility_\.fill\(0\)[\s\S]*?baseline_world_key_\.clear\(\)' `
    -and $disableForMainMenuLifecycle -notmatch `
        'world_map_umg_renderer_\.suspend\(\)') `
    'The exact TitleMap owner boundary must detach and discard the prior-save world-map host and numeric visibility state before any new-world activation can begin.'
Assert-True ($activateLifecycle -match `
        'reset_area_quest_task_class_runtime\(\s*area_quest_state_provider_ready_[\s\S]*?area_quest_catalog_\.size\(\)\s*==\s*kExpectedAreaQuestCount\s*\)') `
    'Task-class capture and numeric witnesses are not reset on a fresh F7 activation.'
Assert-True ($transitionEndLifecycle -match `
        'reset_area_quest_task_class_runtime\(\s*area_quest_state_provider_ready_[\s\S]*?area_quest_catalog_\.size\(\)\s*==\s*kExpectedAreaQuestCount\s*,\s*false\s*\)') `
    'Travel end must rearm task-class capture while preserving bounded numeric completion witnesses.'
Assert-True ($disableLifecycle -match `
        'reset_area_quest_task_class_runtime\(\s*false\s*\)') `
    'F8 must clear task-class state and activation-local completion witnesses.'
Assert-True ($transitionBeginLifecycle -match `
        'reset_area_quest_task_class_runtime\(\s*false\s*,\s*false\s*\)') `
    'Travel begin must clear world-local task-class state while preserving bounded numeric completion witnesses.'
$applySaveResult = (Get-MainFunction `
    $mainCode 'request_or_apply_save_reconcile') + "`n" +
    (Get-MainFunction `
        $mainCode 'apply_full_save_reconcile_result') + "`n" +
    (Get-MainFunction `
        $mainCode 'apply_completion_confirmation_result')
$activateBody = Get-MainFunction $mainCode 'activate'
$disableBody = Get-MainFunction $mainCode 'disable'
Assert-True ($applySaveResult -notmatch `
        'completed_dynamic_quest_ids_\.clear\(\)' `
    -and $applySaveResult -match `
        'completion\.complete_count\s*>\s*0[\s\S]*?completed_dynamic_quest_ids_\.insert\(' `
    -and $applySaveResult -match `
        'merge_encounter_cooldown\(\s*current_next_available,\s*next_available\)' `
    -and $applySaveResult -match `
        'rebuild_area_quest_world_map_eligibility\(\)[\s\S]*?refresh_world_map_atlas_for_runtime_delta\(') `
    'A delayed save snapshot must preserve newer runtime state and retire any already attached stale atlas.'
Assert-True ($activateBody -match `
        'completed_dynamic_quest_ids_\.clear\(\)' `
    -and $disableBody -match `
        'completed_dynamic_quest_ids_\.clear\(\)') `
    'Activation and disable lifecycle owners must still clear activation-local dynamic-quest completion state.'

$taskCompletePost = Get-MainFunction $mainCode 'task_complete_post'
$taskCompleteUnsafe = Get-MainFunction $mainCode 'task_complete_post_unsafe'
$consumeExactCompletions = Get-MainFunction `
    $mainCode 'consume_exact_area_quest_completions'
$applyExactCompletion = Get-MainFunction `
    $mainCode 'apply_exact_area_quest_completion'
$latchExactCompletion = Get-MainFunction `
    $mainCode 'latch_exact_area_quest_completion_numeric'
$serviceTaskClassMap = Get-MainFunction `
    $mainCode 'service_area_quest_task_class_map'
$engineTickUnsafe = Get-MainFunction $mainCode 'engine_tick_unsafe'
$transitionBegin = Get-MainFunction $mainCode 'transition_begin'
$transitionPreserveBranch = [regex]::Match(
    $consumeExactCompletions,
    '(?ms)if\s*\(preserve_for_transition[\s\S]*?^\s{16}\}\s*else\s*\{').Value
Assert-True ($transitionBegin -match `
        'consume_exact_area_quest_completions\(true\);[\s\S]*?reset_area_quest_task_class_runtime\(false,\s*false\)' `
    -and $transitionPreserveBranch -match `
        'latch_exact_area_quest_completion_numeric\(index\)' `
    -and $transitionPreserveBranch -notmatch `
        'apply_exact_area_quest_completion|world_map_umg_renderer_|compact_umg_renderer_|reset_world_map_runtime|rebuild_area_quest_world_map_eligibility' `
    -and $latchExactCompletion -notmatch `
        'world_map_umg_renderer_|compact_umg_renderer_|reset_world_map_runtime|rebuild_area_quest_world_map_eligibility') `
    'Travel must drain same-frame exact-completion bits into numeric state before reset without touching a renderer during teardown.'
$explicitF7TaskClassRearm = [regex]::Match(
    $engineTickUnsafe,
    '(?ms)if\s*\(enabled_[\s\S]*?AREA_QUEST_TASK_CLASS_MAP_REARMED[\s\S]*?rearm_world_map_from_f7').Value
Assert-True ($explicitF7TaskClassRearm -match `
        'reset_area_quest_task_class_capture\(true\)' `
    -and $explicitF7TaskClassRearm -notmatch `
        'reset_area_quest_task_class_runtime\(') `
    'Explicit F7 must retry only task-class mapping and preserve exact completion bits and revision fences.'
Assert-True ($mainCode -match '/Script/DS\.DETTaskBaseActor:OnRecvCompleteQuest' `
    -and $mainCode -match 'AREA_QUEST_EXACT_COMPLETION_OBSERVED' `
    -and $taskCompletePost -match `
        'area_quest_rescan_requests_\.fetch_add\(\s*1' `
    -and $taskCompletePost -match 'task_complete_post_unsafe\(context\.Context\)' `
    -and $taskCompleteUnsafe -match 'task_class->GetFullName\(\)' `
    -and $taskCompleteUnsafe -match `
        'area_quest_task_class_indices_\.find\(class_name\)' `
    -and $taskCompleteUnsafe -match `
        'area_quest_exact_completion_bits_\[word\]\.fetch_or\(' `
    -and $consumeExactCompletions -match `
        'area_quest_exact_completion_bits_\[word_index\]\.exchange\(' `
    -and $consumeExactCompletions -match `
        'apply_exact_area_quest_completion\(' `
    -and $applyExactCompletion -match `
        'latch_exact_area_quest_completion_numeric\(index\)' `
    -and $latchExactCompletion -match `
        '\+\+area_quest_exact_completion_revisions_\[index\]' `
    -and $latchExactCompletion -match `
        'area_quest_states_\[index\]\s*=\s*dswros::AreaQuestState::End' `
    -and $applyExactCompletion -match `
        'area_quest_world_map_eligibility_' `
    -and $latchExactCompletion -match `
        'area_quest_repeatable_reactivation_armed_\[index\]\s*=\s*false' `
    -and $latchExactCompletion -match `
        'area_quest_scan_repeatable_reactivation_armed_\[index\]\s*=\s*false' `
    -and $latchExactCompletion -match 'compact_rebind_dirty_\s*=\s*true' `
    -and $applyExactCompletion -match `
        'refresh_world_map_atlas_for_runtime_delta\(\s*"area_quest_exact_completion"' `
    -and $mainCode -match `
        'request_area_quest_scan\("quest_state_event_debounced"\)') `
    'Mapped task completion must hide the exact task immediately while every completion still requests the generic debounced transactional refresh.'
Assert-True ($mainCode -match `
        'kAreaQuestTaskClassMapMaxAttempts\s*=\s*2' `
    -and $serviceTaskClassMap -match `
        'area_quest_task_class_map_attempt_count_\s*>=\s*kAreaQuestTaskClassMapMaxAttempts' `
    -and $serviceTaskClassMap -match `
        'area_quest_task_class_map_retry_after_\s*=\s*now\s*\+\s*kAreaQuestTaskClassMapRetryDelay' `
    -and $engineTickUnsafe -match `
        'AREA_QUEST_TASK_CLASS_MAP_REARMED' `
    -and $engineTickUnsafe -match `
        'if\s*\(now\s*<\s*stable_after_\)\s*return;[\s\S]*?if\s*\(!activity_suppressed_\)\s*\{\s*service_area_quest_task_class_map\(engine,\s*now\);\s*\}') `
    'Task-class capture must have one bounded retry and explicit F7 rearm without steady-state polling.'
Assert-True ($mainCode -notmatch `
        'area_quest_exact_completion_(?:candidate|unambiguous|transition|baseline|rescan_pending)' `
    -and $areaQuestVisibility -notmatch `
        'area_quest_exact_completion_transition|candidate_count\s*==\s*1') `
    'The removed dev53 ID-less candidate/unambiguous inference path is still present.'
$consumeEncounterCandidates = Get-MainFunction `
    $mainCode 'consume_created_encounter_candidates'
Assert-True ($mainCode -notmatch '\bFindAllOf\s*\(' `
    -and $mainCode -match 'kEncounterObservationRadius\s*=\s*10000\.0' `
    -and $mainCode -match `
        'kEncounterCandidateProbeBudgetPerControlTick\s*=\s*8' `
    -and $mainCode -match `
        'std::array<FWeakObjectPtr,\s*kExpectedEncounterCount>[\s\S]*?created_encounter_candidates_' `
    -and $consumeEncounterCandidates -match `
        'kEncounterObservationRadius\s*\*\s*kEncounterObservationRadius' `
    -and $consumeEncounterCandidates -match `
        'candidates\[index\]\.Get\(\)' `
    -and $consumeEncounterCandidates -match `
        'if\s*\(activity_suppressed_\s*\|\|\s*!position_valid_\)' `
    -and $consumeEncounterCandidates -match `
        'position_queries\s*<\s*kEncounterCandidateProbeBudgetPerControlTick' `
    -and $consumeEncounterCandidates -match `
        'read_actor_position\(actor,\s*&actor_position\)[\s\S]*?distance_squared\(player_,\s*actor_position\)' `
    -and $consumeEncounterCandidates -match `
        'observe_encounter\([\s\S]*?candidates\[index\][\s\S]*?actor_position' `
    -and $consumeEncounterCandidates -notmatch `
        'actor_begin_unsafe|FindAllOf|FindFirstOf|StaticFindObject|std::vector|\bnew\b') `
    'Encounter catch-up must use fixed create-event weak slots and at most eight current-actor position queries with no suppressed work or UObject enumeration.'
Assert-True ($mainCode -notmatch `
        'SharedPublisher|CreateFileMappingW|MapViewOfFile|publisher_|DIAGNOSTIC_MAPPING_DISABLED') `
    'The retired external diagnostic shared mapping must not remain in either native renderer path.'
$unregisterCallbacks = Get-MainFunction $main 'unregister_callbacks'
Assert-True ($mainCode -match 'RegisterHook\(\s*task_complete_function_' `
    -and $mainCode -match 'task_complete_hook_registered_\s*=\s*true' `
    -and $unregisterCallbacks -match 'UnregisterHook\(\s*task_complete_function_' `
    -and $unregisterCallbacks -match 'task_complete_hook_registered_\s*=\s*false') `
    'The task-completion hook does not have symmetric registration and shutdown ownership.'
$captureAreaQuestDefinitions = Get-MainFunction `
    $mainCode 'capture_area_quest_definitions_unsafe'
$evaluateAreaQuestCondition = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}\[\[nodiscard\]\]\s+dswros::AreaQuestEligibilityProof\s+evaluate_area_quest_condition\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($mainCode -match 'MonsterAlive\s*=\s*19' `
    -and $evaluateAreaQuestCondition.Length -gt 0 `
    -and $captureAreaQuestDefinitions -match `
        'kMonsterEncounterLinkRadius\s*=\s*15000\.0' `
    -and $captureAreaQuestDefinitions -match `
        'condition\.type\s*!=[\s\S]*?DynamicQuestConditionType::MonsterAlive\)[\s\S]*?\|\|\s*condition\.value1\s*!=\s*0[\s\S]*?return;' `
    -and $captureAreaQuestDefinitions -match `
        'for\s*\(const EncounterSpec& encounter\s*:\s*encounter_catalog_\)' `
    -and $captureAreaQuestDefinitions -match `
        'encounter\.kind\s*!=\s*EncounterKind::Assault[\s\S]*?continue;' `
    -and $captureAreaQuestDefinitions -match `
        'distance\s*>\s*monster_link_radius_squared[\s\S]*?continue;' `
    -and $captureAreaQuestDefinitions -match `
        '\+\+candidate_count' `
    -and $captureAreaQuestDefinitions -match `
        'nearest\s*&&\s*candidate_count\s*==\s*1U[\s\S]*?condition\.linked_encounter_id\s*=\s*nearest->id' `
    -and $captureAreaQuestDefinitions -match `
        'candidate_count\s*>\s*1U[\s\S]*?area_quest_definition_monster_ambiguous_count_' `
    -and [regex]::Matches(
        $captureAreaQuestDefinitions,
        'condition\.linked_encounter_id\s*=\s*nearest->id').Count -eq 1 `
    -and $evaluateAreaQuestCondition -match `
        'DynamicQuestConditionType::MonsterAlive[\s\S]*?encounter_available\(\*encounter,\s*unix_seconds\(\)\)' `
    -and $evaluateAreaQuestCondition -notmatch `
        'encounter_visible_for_(?:selected_mode|display_mode)' `
    -and $mainCode -match 'area_quest_definition_monster_link_count_' `
    -and $mainCode -match 'RUNTIME_VISIBILITY_EDGE' `
    -and $mainCode -match 'world_hour\s*!=\s*previous_world_hour' `
    -and $mainCode -notmatch 'kMonsterEncounterLinkRadius\s*>\s*15000\.0') `
    'MONSTER_ALIVE linking must accept only value1=0 and exactly one Assault inside 150 metres; zero or ambiguous candidates must remain fail closed.'
$runtimeVisibilityService = Get-MainFunction `
    $mainCode 'service_runtime_visibility_edges'
Assert-True ($runtimeVisibilityService -match `
        'previous_world_hour\s*=\s*last_area_quest_world_hour_' `
    -and $runtimeVisibilityService -match `
        'world_hour_changed\s*=\s*world_hour\s*!=\s*previous_world_hour' `
    -and $runtimeVisibilityService -match `
        'last_area_quest_world_hour_\s*=\s*world_hour' `
    -and $runtimeVisibilityService -match `
        'world_hour\s*>=\s*0\s*&&\s*area_quest_state_provider_ready_[\s\S]*?area_quest_time_rescan_scheduled_\s*=\s*true' `
    -and $runtimeVisibilityService -notmatch `
        'previous_world_hour\s*>=\s*0' `
    -and $activateBody -match `
        'last_area_quest_world_hour_\s*=\s*-1' `
    -and $engineTickUnsafe -match `
        'area_quest_time_rescan_scheduled_[\s\S]*?!area_quest_scan_pending_[\s\S]*?area_quest_time_rescan_scheduled_\s*=\s*false[\s\S]*?request_area_quest_scan\("world_hour_edge"\)' `
    -and $transitionBegin -match `
        'area_quest_time_rescan_scheduled_\s*=\s*false' `
    -and $mainCode -notmatch 'area_quest_time_rescan_requests_') `
    'The first valid world-hour sample and each later displayed-hour edge must coalesce exactly one transactional area-quest refresh.'
$recomputeCooldownEdge = Get-MainFunction `
    $mainCode 'recompute_next_encounter_cooldown_edge'
$establishVisibilityBaseline = Get-MainFunction `
    $mainCode 'establish_encounter_visibility_baseline'
$retireAtlasForRuntimeDelta = Get-MainFunction `
    $mainCode 'refresh_world_map_atlas_for_runtime_delta'
$encounterVisibilityMask = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?std::uint64_t\s+encounter_visibility_mask\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($encounterVisibilityMask.Length -gt 0 `
    -and $mainCode -match 'kExpectedEncounterCount\s*=\s*49' `
    -and $encounterVisibilityMask -match `
        'std::min\(\s*encounter_catalog_\.size\(\),\s*kExpectedEncounterCount\)' `
    -and $encounterVisibilityMask -match `
        'encounter_visible_for_selected_mode_at_hour\([\s\S]*?spec,\s*now_unix_seconds,\s*world_hour\)' `
    -and $encounterVisibilityMask -match `
        'mask\s*\|=\s*std::uint64_t\{1\}\s*<<\s*index' `
    -and [regex]::Matches(
        $mainCode,
        'encounter_visible_for_selected_mode\(').Count -eq 3 `
    -and [regex]::Matches(
        $mainCode,
        'encounter_visible_for_selected_mode_at_hour\(').Count -eq 2 `
    -and [regex]::Matches(
        $mainCode,
        'dswros::encounter_visible_for_display_mode\(').Count -eq 2 `
    -and $objectState -match `
        'encounter_visible_for_display_mode[\s\S]*?is_assault\s*&&\s*show_all_assaults[\s\S]*?encounter_available_now' `
    -and $nativeTests -match `
        'AVAILABLE Assault display must preserve the live time window' `
    -and $nativeTests -match `
        'ALL Assault display must expose an out-of-window static record' `
    -and $nativeTests -match `
        'ALL Assault display must include cooling-down Assaults' `
    -and $nativeTests -match `
        'ALL Assault display must expose the static catalog before save state is ready' `
    -and $nativeTests -match `
        'ALL Assault display must not bypass a Boss time condition' `
    -and $nativeTests -match `
        'AVAILABLE Assault display must fail closed before state is ready') `
    'Encounter visibility must remain one fixed 49-bit presentation mask, with Assault ALL exposing only the static Assault catalog while AVAILABLE, Boss, completion, and cooldown authority remain unchanged.'
Assert-True ([regex]::Matches(
        $mainCode,
        'encounter_next_available_unix_seconds_\.clear\(\)').Count -eq 1 `
    -and $disableForMainMenuLifecycle -match `
        'encounter_next_available_unix_seconds_\.clear\(\)' `
    -and $objectState -match `
        'merge_encounter_cooldown[\s\S]*?current_next_available_unix_seconds\s*<\s*candidate_next_available_unix_seconds' `
    -and $nativeTests -match `
        'an older save snapshot must not roll back a newer runtime cooldown' `
    -and $nativeTests -match `
        'a newer save snapshot must advance the effective cooldown') `
    'Ordinary lifecycle and delayed save reconciliation must preserve the maximum runtime encounter cooldown; only the exact TitleMap save-owner boundary may clear it.'
Assert-True ($recomputeCooldownEdge -match `
        'next_encounter_cooldown_edge_unix_seconds_\s*=\s*0' `
    -and $recomputeCooldownEdge -match `
        'next_available\s*<=\s*now_unix_seconds' `
    -and $recomputeCooldownEdge -match `
        'next_encounter_cooldown_edge_unix_seconds_\s*==\s*0[\s\S]*?next_available\s*<\s*next_encounter_cooldown_edge_unix_seconds_' `
    -and $establishVisibilityBaseline -match `
        'encounter_visibility_mask_\s*=\s*encounter_visibility_mask\(' `
    -and $establishVisibilityBaseline -match `
        'recompute_next_encounter_cooldown_edge\(now_unix_seconds\)') `
    'The bounded encounter baseline must retain only the earliest future cooldown edge.'
$runtimeEdgeCallIndex = $engineTickUnsafe.IndexOf(
    'service_runtime_visibility_edges(now);',
    [StringComparison]::Ordinal)
$positionBranchIndex = $engineTickUnsafe.IndexOf(
    'if (now >= next_position_)',
    [StringComparison]::Ordinal)
Assert-True ($mainCode -match `
        'kMinimapScaleSampleInterval\s*=\s*std::chrono::seconds\{1\}' `
    -and $runtimeVisibilityService -match `
        'next_runtime_visibility_edge_probe_\s*=\s*now\s*\+\s*kMinimapScaleSampleInterval' `
    -and $runtimeVisibilityService -match `
        'cooldown_edge_reached\s*=\s*encounter_state_ready_[\s\S]*?next_encounter_cooldown_edge_unix_seconds_\s*>\s*0[\s\S]*?now_unix_seconds\s*>=\s*next_encounter_cooldown_edge_unix_seconds_' `
    -and $runtimeVisibilityService -match `
        'if\s*\(!world_hour_changed\s*&&\s*!cooldown_edge_reached[\s\S]*?&&\s*encounter_visibility_mask_valid_\)\s*\{\s*return;' `
    -and $runtimeVisibilityService -match `
        'previous_encounter_visibility[\s\S]*?!=\s*encounter_visibility_mask_' `
    -and $runtimeVisibilityService -match `
        '\(world_hour_changed\s*\|\|\s*cooldown_edge_reached\)[\s\S]*?rebuild_area_quest_world_map_eligibility\(' `
    -and $runtimeVisibilityService -match `
        'if\s*\(!encounter_visibility_changed\s*&&\s*!area_quest_visibility_changed\)\s*\{\s*return;' `
    -and $runtimeVisibilityService -match `
        'compact_rebind_dirty_\s*=\s*true;[\s\S]*?refresh_world_map_atlas_for_runtime_delta\(' `
    -and $runtimeEdgeCallIndex -ge 0 `
    -and $positionBranchIndex -gt $runtimeEdgeCallIndex `
    -and $engineTickUnsafe -match `
        'if\s*\(now\s*>=\s*next_activity_probe_\)\s*\{\s*next_activity_probe_\s*=\s*now\s*\+\s*kDiscoveryInterval;\s*probe_activity_context_guarded\(engine\);\s*if\s*\(!enabled_\)\s*\{\s*return;\s*\}\s*apply_compact_suppression\(\);\s*service_runtime_visibility_edges\(now\);\s*\}' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'service_runtime_visibility_edges\(now\)').Count -eq 1) `
    'Runtime encounter edges must use one 1 Hz scalar service inside the 250 ms control branch and outside the 16 ms position branch.'
foreach ($forbiddenRuntimeEdgeWork in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'ProcessEvent',
        'UObject', 'read_player_position', 'service_world_map_atlas',
        'update_compact_pool', 'std::vector', 'new ')) {
    Assert-True ($runtimeVisibilityService -notmatch `
            [regex]::Escape($forbiddenRuntimeEdgeWork)) `
        "Runtime visibility edge service contains forbidden recurring work: $forbiddenRuntimeEdgeWork"
}
Assert-True ($retireAtlasForRuntimeDelta -match `
        'world_map_marker_snapshot_built_\s*=\s*false' `
    -and $retireAtlasForRuntimeDelta -match `
        'if\s*\(attached\s*&&\s*visibly_open\)[\s\S]*?begin_activation\(\);[\s\S]*?reset_world_map_runtime\(true\)' `
    -and $retireAtlasForRuntimeDelta -match `
        'else\s+if\s*\(attached\)[\s\S]*?world_map_session_pending_\s*=\s*false[\s\S]*?deferred_until_set_world_map_image' `
    -and $retireAtlasForRuntimeDelta -notmatch `
        'collect_world_map_marker_snapshot|attach_once|service_world_map_atlas|FindFirstOf|FindAllOf' `
    -and $mainCode -match 'Clock::time_point\s+next_runtime_visibility_edge_probe_' `
    -and $mainCode -match 'std::int64_t\s+next_encounter_cooldown_edge_unix_seconds_' `
    -and $mainCode -match 'std::uint64_t\s+encounter_visibility_mask_' `
    -and $mainCode -match 'bool\s+encounter_visibility_mask_valid_') `
    'A direct encounter or linked-task edge must rebuild only an exact visible session and defer a hidden retained layer.'
$markTreasureOpened = Get-MainFunction $mainCode 'mark_treasure_opened'
Assert-True ($markTreasureOpened -match `
        'runtime_opened_treasure_ids_\.insert\(id\)\.second;\s*if\s*\(!newly_recorded\)\s*\{\s*return\s+false;\s*\}\s*bool\s+compact_visibility_changed' `
    -and $markTreasureOpened -match `
        'compact_rebind_dirty_\s*=\s*compact_rebind_dirty_\s*\|\|\s*compact_visibility_changed' `
    -and $markTreasureOpened -notmatch `
        'compact_rebind_dirty_\s*=\s*true' `
    -and $markTreasureOpened -match `
        'render_catalog_entries_\[index\]\.map_id[\s\S]*?==\s*dswros::CompactRenderModel::kCompactMapId' `
    -and $markTreasureOpened -match `
        'mini_game_catalog_\[index\]\.map_id[\s\S]*?==\s*dswros::CompactRenderModel::kCompactMapId' `
    -and $markTreasureOpened -match `
        'if\s*\(!world_map_visibility_changed\)\s*\{\s*return\s+compact_visibility_changed;\s*\}' `
    -and $markTreasureOpened -match `
        'refresh_world_map_atlas_for_runtime_delta\(\s*"treasure_opened",\s*id\)' `
    -and $markTreasureOpened -notmatch `
        'service_world_map_atlas|collect_world_map_marker_snapshot|attach_once|FindFirstOf|FindAllOf') `
    'Treasure and mini-game completion must reject duplicate events before catalog matching, dirty compact state only on a real visibility delta, and invalidate map-100 atlas state without rebuilding from gameplay sampling.'
Assert-True ($service -match 'world_map_session_pending_' `
    -and $service -match 'world_map_service_attempts_\s*<\s*kWorldMapMaxServiceAttempts' `
    -and $service -match 'world_map_session_pending_\s*=\s*retry') `
    'The atlas attachment service lost its bounded one-shot session gate.'

$markEncounter = Get-MainFunction $mainCode 'mark_encounter_defeated'
$applyEncounterDefeat = Get-MainFunction `
    $mainCode 'apply_encounter_defeat_state'
$observeEncounter = Get-MainFunction $mainCode 'observe_encounter'
$probeObservedObjects = Get-MainFunction $mainCode 'probe_observed_objects'
$recoverEncounterEnd = Get-MainFunction `
    $mainCode 'recover_unobserved_encounter_end'
$encounterDeathPreUnsafe = Get-MainFunction `
    $mainCode 'encounter_death_pre_unsafe'
$consumePendingEncounterDeaths = Get-MainFunction `
    $mainCode 'consume_pending_encounter_deaths'
$ordinaryEncounterRendererBranch = [regex]::Match(
    $applyEncounterDefeat,
    '(?ms)if\s*\(!lifecycle_boundary\)\s*\{(?<body>.*?)^\s{8}\}\s*^\s{8}try\s*\{')
Assert-True $ordinaryEncounterRendererBranch.Success `
    'The ordinary encounter-defeat renderer branch was not found.'
$ordinaryEncounterRendererBody =
    $ordinaryEncounterRendererBranch.Groups['body'].Value
$compactEncounterDirtyIndex = $applyEncounterDefeat.IndexOf(
    'compact_rebind_dirty_ = true;', [StringComparison]::Ordinal)
$ordinaryEncounterBranchIndex = $applyEncounterDefeat.IndexOf(
    'if (!lifecycle_boundary)', [StringComparison]::Ordinal)
Assert-True ($markEncounter -match `
        'encounter_time_condition_matches\(\*spec\)' `
    -and $markEncounter -match `
        'apply_encounter_defeat_state\s*\(\s*\*spec\s*,\s*false\s*,\s*"bounded_disappearance_or_destroyed_end"\s*\)') `
    'Ordinary bounded encounter completion no longer delegates to the shared non-lifecycle defeat transaction.'
Assert-True ($consumeEncounterCandidates -match `
        'encounter_available\([\s\S]*?encounter_catalog_\[index\],\s*now_unix_seconds\)' `
    -and $observeEncounter -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $probeObservedObjects -match `
        'encounter_available\([\s\S]*?\*current_encounter,\s*now_unix_seconds\)' `
    -and $recoverEncounterEnd -match `
        'encounter_available\(\*spec,\s*now_unix_seconds\)' `
    -and $encounterDeathPreUnsafe -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and ($consumeEncounterCandidates + $observeEncounter `
        + $probeObservedObjects + $recoverEncounterEnd `
        + $encounterDeathPreUnsafe + $evaluateAreaQuestCondition) -notmatch `
        'encounter_visible_for_(?:selected_mode|display_mode)') `
    'Assault ALL must remain a rendering-only policy; encounter binding, observation, death/recovery, and MONSTER_ALIVE proof must keep strict current availability.'
Assert-True ($applyEncounterDefeat -match `
        'encounter_cooldown_write_allowed\(' `
    -and $applyEncounterDefeat -match 'active_future_cooldown' `
    -and $applyEncounterDefeat -match 'return\s+false' `
    -and $applyEncounterDefeat -match `
        'encounter_next_available_unix_seconds_\[spec\.id\][\s\S]*?now_unix_seconds\s*\+\s*cooldown_seconds' `
    -and $applyEncounterDefeat -match `
        'establish_encounter_visibility_baseline\(now_unix_seconds\)' `
    -and $applyEncounterDefeat -match `
        'if\s*\(area_quest_state_ready_\)[\s\S]*?rebuild_area_quest_world_map_eligibility\(\)' `
    -and $compactEncounterDirtyIndex -ge 0 `
    -and $ordinaryEncounterBranchIndex -gt $compactEncounterDirtyIndex `
    -and $ordinaryEncounterRendererBody -match `
        'world_map_umg_renderer_\.state\(\)[\s\S]*?WorldMapUmgRendererState::Attached' `
    -and $ordinaryEncounterRendererBody -match `
        'refresh_world_map_atlas_for_runtime_delta\s*\(\s*"encounter_defeated"\s*,\s*spec\.id\s*\)' `
    -and [regex]::Matches(
        $applyEncounterDefeat,
        'world_map_umg_renderer_').Count -eq [regex]::Matches(
        $ordinaryEncounterRendererBody,
        'world_map_umg_renderer_').Count `
    -and [regex]::Matches(
        $applyEncounterDefeat,
        'refresh_world_map_atlas_for_runtime_delta\s*\(').Count -eq 1 `
    -and [regex]::Matches(
        $ordinaryEncounterRendererBody,
        'refresh_world_map_atlas_for_runtime_delta\s*\(').Count -eq 1) `
    'An ordinary confirmed encounter defeat must always dirty compact selection and invalidate the current or deferred atlas through the shared transaction.'
Assert-True ($consumePendingEncounterDeaths -match `
        'bool\s+lifecycle_boundary\s*=\s*false\)\s+noexcept' `
    -and $consumePendingEncounterDeaths -match `
        'PendingEncounterDeathConsumptionContext\s+context[\s\S]*?lifecycle_boundary' `
    -and $consumePendingEncounterDeaths -match `
        'apply_encounter_defeat_state\s*\(\s*spec\s*,\s*lifecycle_boundary\s*,\s*evidence\s*\)' `
    -and $consumePendingEncounterDeaths -match `
        'if\s*\(!lifecycle_boundary\)[\s\S]*?clear_processed_encounter_identity\(spec\.id\)' `
    -and $consumePendingEncounterDeaths -match `
        'pending_encounter_death_mask_\.fetch_and\s*\(\s*~bit\s*,\s*std::memory_order_acq_rel\s*\)' `
    -and $consumePendingEncounterDeaths -match `
        'pending_encounter_death_process_mask_\.fetch_and\s*\(\s*~bit\s*,\s*std::memory_order_acq_rel\s*\)' `
    -and $applyEncounterDefeat -match `
        'lifecycle_boundary\s*\?\s*"deferred_by_lifecycle"' `
    -and $applyEncounterDefeat -match `
        'lifecycle_boundary\s*\?\s*"next_valid_lifecycle"' `
    -and $consumePendingEncounterDeaths -notmatch `
        'world_map_umg_renderer_|compact_umg_renderer_|refresh_world_map_atlas_for_runtime_delta|service_world_map_atlas|collect_world_map_marker_snapshot|attach_once|FindFirstOf|FindAllOf|ProcessEvent') `
    'Lifecycle-boundary encounter consumption must close the accepted numeric bit while explicitly skipping renderer and atlas work.'
Assert-True ($applyEncounterDefeat -notmatch `
        'collect_world_map_marker_snapshot|attach_once|service_world_map_atlas|FindFirstOf|FindAllOf' `
    -and $ordinaryEncounterRendererBody -notmatch `
        'collect_world_map_marker_snapshot|attach_once|service_world_map_atlas|FindFirstOf|FindAllOf') `
    'Encounter defeat must not rebuild or discover the expanded map during combat.'

$activityEdge = Get-MainFunction $mainCode 'apply_activity_suppression_edge'
Assert-True ($activityEdge -match 'world_map_umg_renderer_\.detach\s*\(\s*\)' `
    -and $activityEdge -match 'world_map_activation_catch_up\s*\(\s*\)' `
    -and $activityEdge -match `
        'if\s*\(activity_suppressed_\)\s*\{[\s\S]*?compact_umg_renderer_\.detach\s*\(\s*\);\s*reset_compact_pool_runtime\s*\(\s*\)' `
    -and $activityEdge -match `
        'else\s+if\s*\(enabled_\s*&&\s*!transition_active_\)\s*\{[\s\S]*?compact_umg_renderer_\.begin_activation\s*\(\s*\);\s*reset_compact_pool_runtime\s*\(\s*\)' `
    -and $activityEdge -notmatch 'apply_compact_suppression\s*\(\s*\)') `
    'Activity suppression does not detach both old-world renderers and rearm current-world hosts.'
foreach ($forbiddenActivityWork in @(
        'FindAllOf', 'std::vector', 'sleep_for', 'retry_deadline',
        'RegisterHook', 'NotifyOnNewObject')) {
    Assert-True ($activityEdge -notmatch [regex]::Escape($forbiddenActivityWork)) `
        "Activity suppression edge contains forbidden recurring or dynamic work: $forbiddenActivityWork"
}
$resumeAfterF7 = Get-MainFunction $mainCode 'resume_suspended_world_map_after_f7'
Assert-True ($resumeAfterF7 -match 'activity_suppressed_') `
    'F7 retained-host resume is not fail-closed while an activity is suppressed.'
foreach ($forbiddenTiming in @(
        'Sleep(', 'sleep_for', 'retry_deadline',
        'retry_timer', 'milliseconds{')) {
    Assert-True ($service -notmatch [regex]::Escape($forbiddenTiming)) `
        "The atlas service contains forbidden scheduled work: $forbiddenTiming"
}
Assert-True ($mainCode -match `
        'kWorldMapServiceRetryDelay\s*=\s*std::chrono::milliseconds\{150\}' `
    -and $mainCode -match `
        'kWorldMapMaxServiceAttempts\s*=\s*3U' `
    -and $service -match `
        'now\s*<\s*world_map_service_retry_after_' `
    -and ([regex]::Matches(
        $service,
        'world_map_service_attempts_\s*<\s*kWorldMapMaxServiceAttempts').Count -ge 3) `
    -and ([regex]::Matches(
        $service,
        'now\s*\+\s*kWorldMapServiceRetryDelay').Count -eq 3) `
    -and $service -match `
        'world_map_session_pending_\s*=\s*retry' `
    -and $service -match `
        'world_map_service_retry_after_\s*=\s*retry[\s\S]*?Clock::time_point\{\}') `
    'World-map soft-not-ready recovery must remain a delayed three-attempt current-session transaction.'
foreach ($forbiddenDiscovery in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject',
        'RegisterHook', 'NotifyOnNewObject', 'fstream', 'filesystem')) {
    Assert-True ($service -notmatch [regex]::Escape($forbiddenDiscovery)) `
        "The one-shot atlas service contains forbidden discovery or file work: $forbiddenDiscovery"
}
Assert-True ([regex]::Matches(
        $service, 'world_map_umg_renderer_\.attach_elapsed_us\s*\(\s*\)').Count -eq 2) `
    'The event service must report complete attach timing for retained-host reuse and explicit attach completion.'

Assert-MainLifecycleCall $main 'shutdown_for_process_lifetime' `
    'world_map_umg_renderer_\.detach\s*\(\s*\)'
Assert-MainLifecycleCall $main 'disable' `
    'world_map_umg_renderer_\.suspend\s*\(\s*\)'
Assert-MainLifecycleCall $main 'transition_begin' `
    'world_map_umg_renderer_\.detach\s*\(\s*\)'
Assert-MainLifecycleCall $main 'activate' `
    'world_map_umg_renderer_\.begin_activation\s*\(\s*\)'
Assert-MainLifecycleCall $main 'transition_end' `
    'world_map_umg_renderer_\.begin_activation\s*\(\s*\)'
$disable = Get-MainFunction $mainCode 'disable'
Assert-True ($disable -notmatch 'world_map_umg_renderer_\.detach\s*\(') `
    'F8 must suspend the retained atlas host instead of removing it.'
$activate = Get-MainFunction $mainCode 'activate'
$catchUpIndex = $activate.IndexOf('world_map_activation_catch_up()')
$resumeIndex = $activate.IndexOf('resume_suspended_world_map_after_f7()')
Assert-True ($catchUpIndex -ge 0 -and $resumeIndex -gt $catchUpIndex) `
    'F7 must run its one-shot layer catch-up before exact suspended-host resume.'
$resumeOwner = Get-MainFunction $mainCode 'resume_suspended_world_map_after_f7'
Assert-True ($resumeOwner -match 'WorldMapUmgRendererState::Suspended' `
    -and $resumeOwner -match 'world_map_candidate_available_' `
    -and $resumeOwner -match 'resume_suspended\s*\(\s*current_layer\s*\)' `
    -and $resumeOwner -match 'world_map_serviced_serial_\s*=\s*world_map_candidate_serial_' `
    -and $resumeOwner -match 'world_map_renderer_session_started_\s*=\s*true' `
    -and $resumeOwner -match 'world_map_session_pending_\s*=\s*false' `
    -and [regex]::Matches(
        $resumeOwner, 'world_map_umg_renderer_\.attach_elapsed_us\s*\(\s*\)').Count -eq 1) `
    'F7 exact resume does not atomically consume the retained world-map session.'
foreach ($forbiddenResumeOwnerWork in @(
        'collect_world_map_marker_snapshot', 'attach_once',
        'build_rle_tga_atlas', 'ImportFileAsTexture2D',
        'save_reconciler_', 'request_or_apply_save_reconcile')) {
    Assert-True ($resumeOwner -notmatch [regex]::Escape($forbiddenResumeOwnerWork)) `
        "F7 retained-host resume contains forbidden rebuild or save work: $forbiddenResumeOwnerWork"
}

$beginActivation = Get-RendererFunction $rendererCode 'begin_activation'
Assert-True ($beginActivation -match 'WorldMapUmgRendererState::Suspended' `
    -and $beginActivation -match 'activation_active_\s*=\s*true' `
    -and $beginActivation -match 'return;') `
    'A normal F7 activation does not preserve a suspended exact host for resume.'
$suspend = Get-RendererFunction $rendererCode 'suspend_guarded'
Assert-True ($suspend -match 'validate_host_unsafe\s*\(\s*layer_\.Get\(\)\s*\)' `
    -and $suspend -match `
        'for\s*\(\s*auto&\s+host\s*:\s*hosts_\s*\)' `
    -and $suspend -match `
        'set_visibility\s*\(\s*host\.Get\(\)\s*,\s*set_visibility_\s*,\s*kCollapsed\s*\)' `
    -and $suspend -match 'state_\s*=\s*WorldMapUmgRendererState::Suspended') `
    'F8 does not validate then collapse both exact retained hosts.'
$resumeRenderer = Get-RendererFunction $rendererCode 'resume_suspended_guarded'
Assert-True ($resumeRenderer -match 'validate_host_unsafe\s*\(\s*current_layer\s*\)' `
    -and $resumeRenderer -match `
        'layer_index\s*<\s*kWorldMapAtlasLayerCount' `
    -and $resumeRenderer -match `
        'atlas_images_\[layer_index\]\.Get\(\)' `
    -and $resumeRenderer -match 'hosts_\[layer_index\]\.Get\(\)' `
    -and $resumeRenderer -match 'state_\s*=\s*WorldMapUmgRendererState::Attached') `
    'F7 does not validate and restore both exact retained atlas graphs.'
foreach ($lifecyclePath in @($suspend, $resumeRenderer)) {
    foreach ($forbidden in @(
            'build_rle_tga_atlas', 'write_rle_tga',
            'import_file_as_texture_', 'create_widget_', 'NewObject',
            'ofstream', 'filesystem', 'FindFirstOf', 'StaticFindObject')) {
        Assert-True ($lifecyclePath -notmatch [regex]::Escape($forbidden)) `
            "Suspend/resume contains forbidden rebuild, file, allocation, or lookup work: $forbidden"
    }
}
Assert-True ($suspend -match 'detach_unsafe\s*\(\s*\)' `
    -and $resumeRenderer -match 'detach_unsafe\s*\(\s*\)') `
    'Invalid suspended or resumed host identity does not detach fail closed.'
$detach = Get-RendererFunction $rendererCode 'detach_unsafe'
Assert-True ($detach -match `
        'host->ProcessEvent\s*\(\s*remove_from_parent_\s*,\s*nullptr\s*\)' `
    -and $detach -match 'reset_runtime_handles\s*\(\s*\)' `
    -and $detach -match 'WorldMapUmgRendererState::Suspended') `
    'Travel/map replacement cleanup does not remove both attached or suspended hosts.'
foreach ($weakReset in @(
        'layer_', 'retainer_box_', 'native_parent_')) {
    Assert-True ($resetRuntimeHandles -match `
            "$([regex]::Escape($weakReset))(?:\.Reset\s*\(\s*\)|\s*=\s*FWeakObjectPtr\{\})") `
        "Detach does not clear weak atlas identity: $weakReset"
}
foreach ($weakArrayReset in @(
        'hosts_', 'widget_trees_', 'root_panels_', 'native_parent_slots_',
        'atlas_images_', 'atlas_image_slots_', 'atlas_textures_')) {
    Assert-True ($resetRuntimeHandles -match `
            "$([regex]::Escape($weakArrayReset))\[layer_index\](?:\.Reset\s*\(\s*\)|\s*=\s*FWeakObjectPtr\{\})") `
        "Detach does not clear dual-atlas weak identity: $weakArrayReset"
}
Assert-True ($rendererCode -match `
        '(?s)detach_guarded\s*\([^)]*\).*?EXCEPTION_EXECUTE_HANDLER.*?reset_runtime_handles\s*\(\s*\).*?state_\s*=\s*WorldMapUmgRendererState::Faulted') `
    'A detach fault must clear weak handles and leave the renderer Faulted.'
Assert-True ($beginActivation -match `
        'faults_before_detach\s*=\s*fault_count_[\s\S]*?detach_guarded\s*\(\s*\)[\s\S]*?state_\s*==\s*WorldMapUmgRendererState::Faulted[\s\S]*?fault_count_\s*==\s*faults_before_detach[\s\S]*?abi_failure_mask_\s*==\s*0[\s\S]*?state_\s*=\s*WorldMapUmgRendererState::Ready') `
    'A previous runtime-only world-map fault is not recoverable on one explicit activation after weak-handle cleanup and ABI validation.'

Assert-True ($rendererCode -notmatch '\bFindAllOf\s*\(') `
    'The world-map renderer must not enumerate UObjects.'
Assert-True ($rendererCode -notmatch '\bFindFirstOf\s*\(') `
    'World-map object discovery must remain owned by the bounded native owner path.'
$worldMapDataLookups = [regex]::Matches(
    $mainCode, 'FindFirstOf\s*\([^;\r\n]*DWorldMapData').Count
$layerCatchUpLookups = [regex]::Matches(
    $mainCode, 'FindFirstOf\s*\([^;\r\n]*DLayerMap').Count
Assert-True ($worldMapDataLookups -le 1) `
    "DWorldMapData may be resolved once only; found $worldMapDataLookups owner lookups."
Assert-True ($layerCatchUpLookups -le 1) `
    "DLayerMap may have one activation catch-up lookup only; found $layerCatchUpLookups lookups."
Assert-True ($mainCode -notmatch `
        '(?i)world_map[A-Za-z0-9_]*(?:lookup|catch_up)[A-Za-z0-9_]*(?:interval|retry|timer|after|deadline|next)') `
    'A recurring or timed world-map lookup mechanism is present.'
$combinedCode = $mainCode + "`n" + $rendererCode
Assert-True ($combinedCode -notmatch '\bOnPaint\b') `
    'The native world-map path must not register or depend on an OnPaint hook.'
$steadyRendererPaths = [regex]::Matches(
    $rendererCode,
    '(?ms)^(?:bool|void)\s+WorldMapUmgRenderer::(?:update|tick|refresh)(?:_guarded|_unsafe)?\s*\(')
Assert-True ($steadyRendererPaths.Count -eq 0) `
    'The atlas renderer must not own a recurring update, tick, or refresh path.'
$captureWorldMapCandidate = Get-MainFunction `
    $mainCode 'capture_world_map_candidate_unsafe'
$sameLayerBudgetGuardIndex = $captureWorldMapCandidate.IndexOf(
    'if (same_layer', [StringComparison]::Ordinal)
$freshCandidatePublicationIndex = $captureWorldMapCandidate.IndexOf(
    'world_map_layer_candidate_ = current_layer;',
    [StringComparison]::Ordinal)
$freshCandidateTail = if ($freshCandidatePublicationIndex -ge 0) {
    $captureWorldMapCandidate.Substring($freshCandidatePublicationIndex)
} else {
    ''
}
Assert-True ($captureWorldMapCandidate -match `
        'retained\s*=\s*world_map_layer_candidate_\.Get\(\)' `
    -and $captureWorldMapCandidate -match `
        'same_layer\s*=\s*world_map_candidate_available_[\s\S]*?retained\s*==\s*current_layer' `
    -and $captureWorldMapCandidate -match `
        'retry_budget_consumed\s*=\s*world_map_service_attempts_\s*>\s*0[\s\S]*?world_map_serviced_serial_\s*==\s*world_map_candidate_serial_' `
    -and $captureWorldMapCandidate -match `
        'if\s*\(same_layer[\s\S]*?retry_budget_consumed\)\)[\s\S]*?return;' `
    -and $sameLayerBudgetGuardIndex -ge 0 `
    -and $freshCandidatePublicationIndex -gt $sameLayerBudgetGuardIndex `
    -and [regex]::Matches(
        $captureWorldMapCandidate,
        'world_map_service_attempts_\s*=\s*0').Count -eq 0) `
    'Repeated callbacks for the same live world-map layer may not reset or rearm an exhausted three-attempt budget.'
Assert-True ($captureWorldMapCandidate -match `
        'renderer_ready_for_candidate_rearm\s*=[\s\S]*?WorldMapUmgRendererState::Ready[\s\S]*?\|\|[\s\S]*?WorldMapUmgRendererState::Attached[\s\S]*?!world_map_umg_renderer_\.attached_layer_matches\s*\(\s*current_layer\s*\)' `
    -and $captureWorldMapCandidate -match `
        'set_image_rearm_allowed\s*=\s*same_layer[\s\S]*?set_world_map_image_event[\s\S]*?!world_map_set_image_rearm_consumed_[\s\S]*?world_map_serviced_serial_\s*==\s*world_map_candidate_serial_[\s\S]*?world_map_service_attempts_\s*>\s*0[\s\S]*?renderer_ready_for_candidate_rearm' `
    -and $captureWorldMapCandidate -match `
        'retryable_not_ready\(\)[\s\S]*?world_map_service_attempts_[\s\S]*?>=\s*kWorldMapMaxServiceAttempts' `
    -and $captureWorldMapCandidate -match `
        'if\s*\(set_image_rearm_allowed\)[\s\S]*?world_map_umg_renderer_\.begin_activation\(\);[\s\S]*?reset_world_map_runtime\(true\);[\s\S]*?world_map_set_image_rearm_consumed_\s*=\s*true;[\s\S]*?return' `
    -and [regex]::Matches(
        $captureWorldMapCandidate,
        'world_map_umg_renderer_\.begin_activation\(\)').Count -eq 1 `
    -and [regex]::Matches(
        $captureWorldMapCandidate,
        'reset_world_map_runtime\(true\)').Count -eq 1) `
    'SetWorldMapImage must provide one consumed serial-matched rearm for either Ready or an Attached renderer that owns a different layer.'
Assert-True ($captureWorldMapCandidate -match `
        'world_map_layer_candidate_\s*=\s*current_layer[\s\S]*?world_map_candidate_available_\s*=\s*true[\s\S]*?\+\+world_map_candidate_serial_[\s\S]*?world_map_session_pending_\s*=\s*true') `
    'A genuinely fresh DLayerMap event does not publish a new one-shot atlas session token.'
Assert-True ($freshCandidatePublicationIndex -ge 0 `
    -and $freshCandidateTail -notmatch `
        'world_map_umg_renderer_\.begin_activation\s*\(' `
    -and $freshCandidateTail -notmatch `
        'reset_world_map_runtime\s*\(') `
    'Raw distinct DLayerMap publication must not detach or reset the old session before the exact SetWorldMapImage readiness edge.'

Write-Host 'Native world-map atlas source gates passed.'
