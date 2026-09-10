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
        "Required world-map file is missing: $RelativePath"
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

function Get-MillisecondConstant {
    param([string]$Text, [string]$Name)
    $match = [regex]::Match(
        $Text,
        ("\b$([regex]::Escape($Name))\s*=\s*" +
            'std::chrono::milliseconds\s*[\{\(]\s*(\d+)\s*[\}\)]\s*;'))
    Assert-True $match.Success `
        "Required millisecond constant is missing: $Name"
    return [int]$match.Groups[1].Value
}

function Get-RequiredPatternIndex {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Description
    )
    $match = [regex]::Match($Text, $Pattern)
    Assert-True $match.Success "Required code shape is missing: $Description"
    return $match.Index
}

function Get-RendererFunction {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^(?:\[\[nodiscard\]\]\s*)?(?:bool|void|WorldMapLayeringRefreshResult|std::optional<[^>]+>)\s+WorldMapUmgRenderer::' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "World-map renderer function was not found: $Name"
    return $match.Value
}

function Get-ProjectionFunction {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^(?:\[\[nodiscard\]\]\s*)?inline\s+(?:bool|void|std::optional<[^>]+>)\s*' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "Projection function was not found: $Name"
    return $match.Value
}

function Get-FreeRendererFunction {
    param([string]$Text, [string]$ReturnType, [string]$Name)
    $pattern =
        '(?ms)^(?:\[\[nodiscard\]\]\s*)?' +
        [regex]::Escape($ReturnType) + '\s+' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "World-map renderer helper was not found: $Name"
    return $match.Value
}

function Get-MainMethod {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?(?:bool|void)\s+' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "World-map main state method was not found: $Name"
    return $match.Value
}

$renderer = Read-RequiredFile 'src\native\world_map_umg_renderer.cpp'
$rendererHeader = Read-RequiredFile 'src\native\world_map_umg_renderer.hpp'
$main = Read-RequiredFile 'src\native\main.cpp'
$renderProjection = Read-RequiredFile 'include\dswros\render_projection.hpp'
$worldMapSessionPolicy = Read-RequiredFile `
    'include\dswros\world_map_session_policy.hpp'
$nativeTests = Remove-CppComments `
    (Read-RequiredFile 'tests\native_state_tests.cpp')
$metadata = Read-RequiredFile 'metadata\release.json' | ConvertFrom-Json

$rendererCode = Remove-CppComments $renderer
$headerCode = Remove-CppComments $rendererHeader
$mainCode = Remove-CppComments $main
$projectionCode = Remove-CppComments $renderProjection
$sessionPolicyCode = Remove-CppComments $worldMapSessionPolicy
$markerCapacity = Get-IntegralConstant `
    $rendererHeader 'kWorldMapUmgMarkerCapacity'
$atlasTextureSize = Get-IntegralConstant `
    $rendererHeader 'kWorldMapAtlasTextureSize'
$atlasLayerCount = Get-IntegralConstant `
    $rendererHeader 'kWorldMapAtlasLayerCount'
$maxWorldMapServiceAttempts = Get-IntegralConstant `
    $mainCode 'kWorldMapMaxServiceAttempts'
$maxWorldMapReadinessAttempts = Get-IntegralConstant `
    $mainCode 'kWorldMapMaxReadinessAttempts'
$worldMapServiceRetryDelayMs = Get-MillisecondConstant `
    $mainCode 'kWorldMapServiceRetryDelay'
$worldMapOpenVisibilityGraceMs = Get-MillisecondConstant `
    $mainCode 'kWorldMapOpenVisibilityGrace'

Assert-True ($markerCapacity -ge 4096) `
    "World-map marker capacity regressed below 4,096; found $markerCapacity."
Assert-True ($atlasTextureSize -eq 2048) `
    "World-map atlas texture must remain exactly 2,048 square; found $atlasTextureSize."
Assert-True ($atlasLayerCount -eq 2) `
    "World-map renderer must retain exactly two atlas layers; found $atlasLayerCount."
Assert-True ($metadata.world_map_umg_renderer.capacity -ge 4096 `
    -and $metadata.world_map_umg_renderer.atlas_texture_size -eq 2048 `
    -and $metadata.world_map_umg_renderer.umg_host_count -eq 2 `
    -and $metadata.world_map_umg_renderer.umg_image_count -eq 2) `
    'Release metadata no longer describes the bounded dual-atlas renderer.'

# The expanded atlas is a native-Canvas child. Pan, zoom, DPI, clipping, and
# RetainerBox behavior must therefore be inherited from the game hierarchy;
# the renderer may not invoke the retired viewport-projection chain.
Assert-True ($rendererCode -notmatch `
        '\b(?:calculate_world_map_viewport_placement|world_map_viewport_transform_changed)\s*\(') `
    'The world-map renderer returned to the delayed independent-viewport projection chain.'
Assert-True ($projectionCode -match `
        'retain_world_map_atlas_placement\s*\([\s\S]*?retained_geometry\.parent_width\s*-\s*current_geometry\.parent_width[\s\S]*?retained_geometry\.parent_height\s*-\s*current_geometry\.parent_height[\s\S]*?return\s+retained\s*;' `
    -and $projectionCode -notmatch `
        'retained\.left\s*\+\s*\(\s*current_geometry\.player_canvas_x' `
    -and $projectionCode -notmatch `
        'retained\.top\s*\+\s*\(\s*current_geometry\.player_canvas_y') `
    'Same-parent placement retention must reject extent changes and never apply PlayerIcon anchor deltas.'
foreach ($requiredPlacementTest in @(
        'same-anchor world-map validation must preserve atlas bounds',
        '5C632820 same-parent zoom anchors must never move the retained inner atlas slot',
        'a 3000-to-3840 parent-local extent change must request a fresh atlas instead of scaling marker glyphs',
        'world-map placement retention must reject invalid retained bounds or anchors',
        'the CD41 outer-atlas regression fixture must reproduce the logged 3191.521 parent width',
        'the first CD41-style extent drift must retain the visible payload and only the second matching successful sample may request rebuild',
        'a cleared extent-stability sample must not reuse a prior observation to request rebuild')) {
    Assert-True ($nativeTests.Contains($requiredPlacementTest)) `
        "Native immutable-placement regression coverage is missing: $requiredPlacementTest"
}

# Host visibility is a four-input policy with an explicit write cache. Keep
# the policy pure so all combinations and transient-transform outcomes can be
# pinned without a live UObject graph.
Assert-True ($projectionCode -match `
        'struct\s+WorldMapHostVisibilityInputs\s*\{[\s\S]*?bool\s+content_intent\s*\{\s*\}\s*;[\s\S]*?bool\s+runtime_allowed\s*\{\s*\}\s*;[\s\S]*?bool\s+attached\s*\{\s*\}\s*;[\s\S]*?bool\s+transform_ready\s*\{\s*\}\s*;' `
    -and $projectionCode -match `
        'world_map_host_visibility_target\s*\([^)]*\)\s*noexcept\s*\{[\s\S]*?return\s+inputs\.content_intent\s*&&\s*inputs\.runtime_allowed\s*&&\s*inputs\.attached\s*&&\s*inputs\.transform_ready\s*;' `
    -and $projectionCode -match `
        'world_map_host_visibility_write_required\s*\([^)]*\)\s*noexcept\s*\{[\s\S]*?return\s+!applied_visible\s*\|\|\s*\*applied_visible\s*!=\s*target_visible\s*;') `
    'Host visibility must remain a pure content/runtime/attachment/transform conjunction with optional-cache deduplication.'
Assert-True ($projectionCode -match `
        'struct\s+WorldMapTransformVisibilityPolicy\s*\{[\s\S]*?bool\s+retain_host\s*\{\s*\}\s*;[\s\S]*?bool\s+transform_ready\s*\{\s*\}\s*;[\s\S]*?bool\s+force_collapsed\s*\{\s*\}\s*;' `
    -and $projectionCode -match `
        'WorldMapTransformObservationFailureAction::RetryHidden\s*:[\s\S]*?return\s*\{\s*true\s*,\s*false\s*,\s*true\s*\}\s*;' `
    -and $projectionCode -match `
        'WorldMapTransformObservationFailureAction::RetainLastValid\s*:[\s\S]*?return\s*\{\s*true\s*,\s*true\s*,\s*false\s*\}\s*;' `
    -and $projectionCode -match `
        'WorldMapTransformObservationFailureAction::Fault[\s\S]*?return\s*\{\s*false\s*,\s*false\s*,\s*true\s*\}\s*;') `
    'Transient transform policy must retain-and-hide before first proof, retain the last valid transform, and release only on a hard fault.'
foreach ($requiredVisibilityTest in @(
        'WorldMapHostVisibilityInputs',
        'world_map_host_visibility_target',
        'world_map_host_visibility_write_required',
        'world_map_transform_visibility_policy',
        'Action::RetryHidden',
        'Action::RetainLastValid',
        'Action::Fault',
        'world-map hosts must be visible only when content intent, runtime visibility, attachment, and transform readiness are all true',
        'world-map host visibility writes must publish unknown, edge, and reverse-edge states exactly once',
        'repeated RetryLater must collapse once without clearing durable intent, then repeated Retained must restore once without detaching')) {
    Assert-True ($nativeTests.Contains($requiredVisibilityTest)) `
        "Native visibility-policy regression coverage is missing: $requiredVisibilityTest"
}

# The atlas hosts are full-stretch native-Canvas children. Their inner Image
# Canvas slots own atlas placement, while the game hierarchy owns all steady-
# state pan, zoom, DPI, clipping, and RetainerBox transforms.
foreach ($forbiddenViewportApi in @(
        'UserWidget:AddToViewport', 'add_to_viewport_',
        'Widget:SetAlignmentInViewport', 'set_alignment_in_viewport_',
        'Widget:SetPositionInViewport', 'set_position_in_viewport_',
        'Widget:SetDesiredSizeInViewport', 'set_desired_size_in_viewport_',
        'Widget:ForceLayoutPrepass', 'force_layout_prepass_')) {
    Assert-True ($rendererCode -notmatch [regex]::Escape($forbiddenViewportApi)) `
        "The native-child renderer contains a forbidden viewport/layout API: $forbiddenViewportApi"
}
Assert-True ($rendererCode -match `
        'constexpr\s+std::int32_t\s+kRadarMarkerZ\s*=\s*(?:std::)?numeric_limits<std::int32_t>::max\s*\(\s*\)\s*;' `
    -and $rendererCode -notmatch `
        "kRadarMarkerZ\s*=\s*2'000'000'000") `
    'Native Canvas atlas slots must use the maximum CanvasPanelSlot Z order.'
Assert-True ($rendererCode -match 'CanvasPanel:AddChildToCanvas' `
    -and $rendererCode -match 'CanvasPanelSlot:SetAnchors' `
    -and $rendererCode -match 'CanvasPanelSlot:SetOffsets' `
    -and $rendererCode -match 'CanvasPanelSlot:SetAutoSize' `
    -and $rendererCode -match 'Widget:SetRenderTranslation' `
    -and $rendererCode -match 'RetainerBox:RequestRender' `
    -and $headerCode -match `
        'native_parent_slots_\s*\{\s*\}\s*;') `
    'The hybrid renderer is missing native Canvas ownership, fixed-slot, zero-image-translation, Retainer, or retained-slot ABI.'

# Reflected FGeometry values are non-trivial UE values. Their parameter
# containers must be initialized and destroyed through reflection rather than
# treated as raw byte buffers whose lifetime happens to work in one build.
Assert-True ($rendererCode -match `
        'class\s+GeometryCallParameters[\s\S]*?InitializeValue_InContainer[\s\S]*?~GeometryCallParameters\s*\([^)]*\)[\s\S]*?DestroyValue_InContainer' `
    -and $rendererCode -match `
        'GeometryCallParameters\s*\(\s*const\s+GeometryCallParameters\s*&\s*\)\s*=\s*delete' `
    -and $rendererCode -match `
        'operator\s*=\s*\(\s*const\s+GeometryCallParameters\s*&\s*\)\s*=\s*delete') `
    'FGeometry ProcessEvent buffers must own a balanced reflected initialize/destroy lifecycle and remain non-copyable.'

# Visibility is fail-closed. Content intent is durable across host lifecycles;
# runtime permission, transform readiness, and the applied-write cache are not.
foreach ($defaultCollapsedState in @(
        '\bcontent_visibility_intent_\s*\{\s*(?:false)?\s*\}\s*;',
        '\bruntime_visibility_allowed_\s*\{\s*(?:false)?\s*\}\s*;',
        '\btransform_ready_\s*\{\s*(?:false)?\s*\}\s*;',
        'std::optional\s*<\s*bool\s*>\s+applied_host_visibility_\s*\{\s*\}\s*;')) {
    Assert-True ($headerCode -match $defaultCollapsedState) `
        "The four-gate host visibility state is missing its fail-closed default: $defaultCollapsedState"
}
Assert-True (($headerCode + "`n" + $rendererCode) -notmatch `
        '\b(?:map_visible_requested_|set_map_visible)\b') `
    'The legacy single visibility-request state or setter returned.'
$configureFullStretchSlot = Get-FreeRendererFunction `
    $rendererCode 'bool' 'configure_full_stretch_canvas_slot'
$configureOverlaySlot = Get-FreeRendererFunction `
    $rendererCode 'bool' 'configure_full_stretch_overlay_slot'
Assert-True ($rendererCode -match `
        'struct\s+CanvasSlotLayoutReflectionSchema[\s\S]*?anchors_parameter[\s\S]*?offsets_parameter[\s\S]*?auto_size_parameter' `
    -and $rendererCode -match `
        'resolve_canvas_slot_layout_schema\s*\([\s\S]*?CastField<FStructProperty>[\s\S]*?CastField<FBoolProperty>' `
    -and $configureFullStretchSlot -match `
        'write_vector_property\s*\(\s*schema\.anchors_minimum\s*,\s*anchors_value\s*,\s*0(?:\.0+)?\s*,\s*0(?:\.0+)?\s*\)' `
    -and $configureFullStretchSlot -match `
        'write_vector_property\s*\(\s*schema\.anchors_maximum\s*,\s*anchors_value\s*,\s*1(?:\.0+)?\s*,\s*1(?:\.0+)?\s*\)' `
    -and [regex]::Matches(
        $configureFullStretchSlot,
        'write_numeric_property\s*\(\s*schema\.offset_(?:left|top|right|bottom)\s*,\s*offsets_value\s*,\s*0(?:\.0+)?\s*\)').Count -eq 4 `
    -and $configureFullStretchSlot -match `
        'SetPropertyValue\s*\(\s*auto_size_value\s*,\s*false\s*\)' `
    -and $configureFullStretchSlot -match `
        'VectorParameters\s+alignment\s*\{\s*\{\s*0(?:\.0+)?\s*,\s*0(?:\.0+)?\s*\}\s*\}' `
    -and $configureFullStretchSlot -notmatch `
        '(?:set_slot_position_|set_slot_size_|set_render_translation_|set_z_order|ForceLayoutPrepass)' `
    -and $configureOverlaySlot -match `
        'configure_full_stretch_canvas_slot\s*\(' `
    -and $configureOverlaySlot -match `
        'ZOrderParameters\s+z_order\s*\{\s*kRadarMarkerZ\s*\}') `
    'Both the cloned internal root and each native host slot must be full-stretch; the outer host additionally owns maximum Z.'
Assert-True ($configureOverlaySlot -notmatch `
        '(?:set_slot_position_|set_slot_size_|set_render_translation_|ForceLayoutPrepass)' `
    -and $rendererCode -notmatch 'configure_atlas_canvas_slot') `
    'The native host slot helper must never carry atlas coordinates, size feedback, a render transform, or a forced layout pass.'

$attach = Get-RendererFunction $rendererCode 'attach_unsafe'
# Flight readiness must be confined to fresh attachment; retained zoom/layout
# validation continues to use the independent extent sampler below.
Assert-True ($attach -match '(?s)read_live_player_canvas_anchor.*?WorldMapProjectionSample.*?observe_world_map_projection_sample.*?WorldMapGeometryStabilityResult::Stable.*?project_world_map_point' `
    -and $attach -notmatch '\bobserve_world_map_geometry_sample\s*\(' `
    -and ([regex]::Matches($rendererCode, '\bobserve_world_map_projection_sample\s*\(')).Count -eq 1) `
    'Motion-compensated readiness must gate fresh projection only, never retained atlas placement.'
foreach ($identityExpression in @(
        'numeric_identity(current_layer)', 'numeric_identity(native_parent)',
        'numeric_identity(player_icon)', 'numeric_identity(expected_owning_player)')) {
    Assert-True ($attach.Contains($identityExpression)) `
        "Attachment projection is missing a weak identity boundary: $identityExpression"
}
Assert-True ($mainCode.Contains('WORLD_MAP_ATTACH_PROJECTION') `
    -and $mainCode.Contains('policy=motion_compensated_origin') `
    -and $mainCode.Contains('origin_delta=') `
    -and $mainCode -match 'player_\s*=\s*position;\s*position_valid_\s*=\s*true;\s*request_or_apply_save_reconcile\(\);\s*service_world_map_atlas\(engine\)') `
    'Attach diagnostics and current-tick player sampling must accompany the motion-compensated gate.'
$refreshNativeParent = Get-RendererFunction `
    $rendererCode 'refresh_native_parent_unsafe'
$refreshLayering = Get-RendererFunction $rendererCode 'refresh_layering'
$detachUnsafe = Get-RendererFunction $rendererCode 'detach_unsafe'
$nativeIconClassLookup = Get-FreeRendererFunction `
    $rendererCode 'NativeIconClassLookupResult' 'find_native_icon_class_witness'
$worldMapRenderParent = Get-FreeRendererFunction `
    $rendererCode 'UObject*' 'resolve_world_map_render_parent'

Assert-True ($nativeIconClassLookup -match `
        '(?s)count\s*<\s*0.*?InvalidSchema' `
    -and $nativeIconClassLookup -match `
        '(?s)count\s*==\s*0.*?NotReady' `
    -and $nativeIconClassLookup -match `
        'GetPropertyByNameInChain\(L"Panel_Point"\)' `
    -and $nativeIconClassLookup -match `
        'GetPropertyByNameInChain\(L"Slot"\)' `
    -and $nativeIconClassLookup -match `
        'GetPropertyByNameInChain\(L"Parent"\)' `
    -and $nativeIconClassLookup -match `
        'GetPropertyByNameInChain\(L"Content"\)' `
    -and $nativeIconClassLookup -notmatch `
        'read_object_property\(\s*(?:icon|slot)') `
    'Native icon-class lookup must distinguish an empty/not-ready list from corrupt counts and missing or type-invalid nested widget ABI fields.'
Assert-True ($worldMapRenderParent -match `
        'read_object_property\(\s*layer\s*,\s*L"FogAbovePanel"\s*\)' `
    -and $worldMapRenderParent -match `
        'parent->IsA\(\s*canvas_panel_class\s*\)' `
    -and $worldMapRenderParent -notmatch 'ArrayIconInfo') `
    'The world-map render owner must resolve the named FogAbovePanel directly, independent of zoom-rebuilt ArrayIconInfo ordering.'

# The outer native slot and cloned root are full-stretch. The Mod-owned Image
# Canvas slot owns the parent-local atlas rectangle, while the Image itself has
# an explicit zero render translation.
Assert-True ($attach -match `
        'root_panels\s*\[\s*layer_index\s*\]\s*->ProcessEvent\s*\(\s*add_child_to_canvas_' `
    -and $attach -match `
        'set_slot_vector\s*\(\s*atlas_image_slots\s*\[\s*layer_index\s*\]\s*,\s*set_slot_position_\s*,\s*atlas_bounds\.left\s*,\s*atlas_bounds\.top\s*\)' `
    -and $attach -match `
        'set_slot_vector\s*\(\s*atlas_image_slots\s*\[\s*layer_index\s*\]\s*,\s*set_slot_size_\s*,\s*atlas_bounds\.width\s*,\s*atlas_bounds\.height\s*\)' `
    -and $attach -match `
        'VectorParameters\s+zero_translation\s*\{\s*\{\s*0(?:\.0+)?\s*,\s*0(?:\.0+)?\s*\}\s*\}[\s\S]*?atlas_images\s*\[\s*layer_index\s*\]\s*->ProcessEvent\s*\(\s*set_render_translation_' `
    -and $attach -match `
        '(?:native_parent|current_parent)\s*->ProcessEvent\s*\(\s*add_child_to_canvas_' `
    -and $attach -match `
        'native_parent_slots_\s*\[\s*layer_index\s*\]\s*=\s*(?:native_slot|add_host\.return_value)' `
    -and $attach -match `
        'configure_full_stretch_overlay_slot\s*\(' `
    -and $attach -match `
        'configure_full_stretch_canvas_slot\s*\(\s*root_slot' `
    -and $attach -match `
        'resolve_world_map_render_parent\s*\(' `
    -and $attach -match `
        'find_native_icon_class_witness\s*\(' `
    -and $attach -match 'kHitTestInvisible') `
    'Initial attachment must use full-stretch outer/root slots, place the atlas through the inner Image Canvas slot, and insert each hit-test-invisible host into the named FogAbove Canvas.'
Assert-True ($attach -match `
        '(?s)native_parent->ProcessEvent\s*\(\s*add_child_to_canvas_.*?configure_full_stretch_overlay_slot\s*\(.*?configure_full_stretch_canvas_slot\s*\(\s*root_slot.*?set_slot_vector\s*\(\s*atlas_image_slots\s*\[\s*layer_index\s*\]\s*,\s*set_slot_position_\s*,\s*atlas_bounds\.left\s*,\s*atlas_bounds\.top\s*\).*?zero_translation.*?set_render_translation_') `
    'After the host enters the live native Canvas, attachment must reassert outer fill, inner fill, Image Canvas-slot atlas placement, then zero Image render translation.'
Assert-True ($rendererCode -notmatch `
        '(?:hosts?|root_panels?)\s*(?:\[[^\]]+\])?\s*->ProcessEvent\s*\(\s*set_render_translation_' `
    -and $rendererCode -notmatch `
        'set_render_translation\s*\(\s*(?:hosts?|root_panels?)\b' `
    -and [regex]::Matches(
        $rendererCode,
        'ProcessEvent\s*\(\s*set_render_translation_').Count -eq 1 `
    -and $rendererCode.Replace($attach, '') -notmatch `
        'ProcessEvent\s*\(\s*set_render_translation_') `
    'Only explicit zero Image translations are permitted; atlas placement must never move a host/root render transform or create a second zoom pivot.'
Assert-True ($refreshNativeParent -notmatch `
        '(?:set_slot_position_|set_render_translation_|remove_from_parent_|add_child_to_canvas_|configure_full_stretch_(?:overlay|canvas)_slot|request_retainer_render_)') `
    'Native-parent refresh must be observation-only; parent replacement is reported for a fresh bounded attachment rather than live-mutating retained hosts.'

Assert-True ($refreshNativeParent -match `
        'resolve_world_map_render_parent\s*\(' `
    -and $refreshNativeParent -notmatch `
        '(?:find_native_icon_class_witness|ArrayIconInfo)' `
    -and $refreshNativeParent -match `
        'validate_host_payload_unsafe\s*\(' `
    -and $refreshNativeParent -match `
        'retain_world_map_atlas_placement\s*\(' `
    -and $refreshNativeParent -match `
        'native_parent_slots_\s*\[' `
    -and $refreshNativeParent -match `
        'WorldMapLayeringRefreshResult::Retained' `
    -and $refreshNativeParent -match `
        'WorldMapLayeringRefreshResult::RebuildRequired' `
    -and $refreshNativeParent -match `
        'WorldMapLayeringRefreshResult::Unchanged' `
    -and $refreshNativeParent -match `
        'world_map_parent_extent_maximum_delta\s*\(') `
    'Native-parent refresh must revalidate owned payload, resolve the named FogAbove Canvas without scanning zoom-rebuilt icons, retain immutable same-parent placement, observe in-place extent drift, and request a bounded rebuild when needed.'

$rebuildReporter = [regex]::Match(
    $refreshNativeParent,
    '(?ms)^\s{4}const\s+auto\s+request_current_session_rebuild\s*=\s*\[\]\s*\([^;]*?\)\s*\{(?<body>[\s\S]*?)^\s{4}\};')
Assert-True ($rebuildReporter.Success `
    -and $rebuildReporter.Groups['body'].Value -match `
        'rebuild_result\s*=\s*WorldMapLayeringRefreshResult::RebuildRequired\s*;[\s\S]*?return\s+true\s*;' `
    -and $rebuildReporter.Groups['body'].Value -notmatch `
        '(?:transform_ready_|reconcile_host_visibility|apply_host_visibility|set_visibility|ProcessEvent|detach)') `
    'A rebuild observation must be report-only; it may not collapse, invalidate, detach, or repaint a still-valid payload before Main accepts the bounded rebuild.'
Assert-True ($refreshNativeParent -match `
        '(?s)if\s*\(\s*\*extent_delta\s*>\s*dswros::kWorldMapGeometryStabilityTolerance\s*\).*?reparent_geometry_parent_index_.*?reparent_geometry_parent_serial_.*?observe_world_map_geometry_sample\s*\(\s*reparent_geometry_sample_valid_\s*,\s*reparent_geometry_sample_\s*,\s*current_geometry\s*,\s*reparent_geometry_sample_max_delta_\s*\).*?reparent_geometry_stability_result_\s*!=\s*dswros::WorldMapGeometryStabilityResult::Stable.*?handle_transient_observation\s*\(\s*dswros::WorldMapTransformObservationFailure::\s*GeometryUnavailable\s*,\s*result\s*\).*?request_current_session_rebuild\s*\(\s*result\s*\)' `
    -and $refreshNativeParent -match `
        '(?s)request_current_session_rebuild\s*\(\s*result\s*\)\s*;\s*\}\s*reset_reparent_geometry_stability_sample\s*\(\s*\)') `
    'A parent-extent drift must be stable for two matching successful samples on one parent before requesting rebuild, while a restored extent clears the pending sample.'

# Every successful same-parent path must observe live geometry without any Image
# Canvas-slot/translation, tree mutation, or Retainer render request. The
# legacy final-restack argument is deliberately ignored: maximum Canvas Z plus
# stable insertion order must not turn any zoom tail into write-back work.
$unchangedReturnIndex = $refreshNativeParent.IndexOf(
    'WorldMapLayeringRefreshResult::Unchanged')
$liveGeometryReadIndex = $refreshNativeParent.IndexOf(
    'read_live_widget_local_extent')
$extentObservationIndex = $refreshNativeParent.IndexOf(
    'world_map_parent_extent_maximum_delta')
$firstImageTranslationIndex = $refreshNativeParent.IndexOf(
    'set_render_translation_')
$firstImageSlotWriteIndex = $refreshNativeParent.IndexOf(
    'set_slot_position_')
$firstTreeRemovalIndex = $refreshNativeParent.IndexOf(
    'remove_from_parent_')
$firstRetainerRequestIndex = $refreshNativeParent.IndexOf(
    'request_retainer_render_')
$refreshVisibilityReconcileCount = [regex]::Matches(
    $refreshNativeParent,
    'reconcile_host_visibility_unsafe\s*\(').Count
Assert-True ($unchangedReturnIndex -ge 0 `
    -and $liveGeometryReadIndex -ge 0 `
    -and $liveGeometryReadIndex -lt $unchangedReturnIndex `
    -and $extentObservationIndex -gt $liveGeometryReadIndex `
    -and $extentObservationIndex -lt $unchangedReturnIndex `
    -and $firstImageTranslationIndex -eq -1 `
    -and $firstImageSlotWriteIndex -eq -1 `
    -and $firstTreeRemovalIndex -eq -1 `
    -and $firstRetainerRequestIndex -eq -1 `
    -and $refreshVisibilityReconcileCount -eq 1 `
    -and $refreshNativeParent -match `
        '\(void\)restack_unchanged_parent\s*;' `
    -and $refreshNativeParent -notmatch `
        'if\s*\([^)]*restack_unchanged_parent' `
    -and $refreshNativeParent -notmatch `
        '(?:PlayerIconWidget|read_live_player_canvas_anchor)' `
    -and $refreshNativeParent -notmatch `
        '(?i)\b(?:GetViewportSize|GetViewportScale|calculate_world_map_viewport_placement|SetPositionInViewport|SetDesiredSizeInViewport)\b') `
    'Same-parent samples must read only the native owner extent and perform no PlayerIcon dependency or transform/tree/Retainer/visibility writes; the sole reconcile call belongs to the transient fail-closed handler.'
$syncNativeParent = Get-RendererFunction `
    $rendererCode 'sync_viewport_transform'
Assert-True ($syncNativeParent -match `
        'return\s+refresh_layering\s*\(\s*current_layer\s*,\s*false\s*,\s*0(?:\.0+)?\s*,\s*0(?:\.0+)?\s*\)' `
    -and $syncNativeParent -notmatch `
        '(?:ProcessEvent|set_render_translation_|set_slot_|request_retainer_render_)') `
    'The compatibility sync entry point must be a write-free native-parent validation pass, never a viewport transform or Retainer update.'
foreach ($forbiddenRefreshWork in @(
        'build_rle_tga_atlas', 'ImportFileAsTexture2D', 'NewObject',
        'CreateWidget', 'ClearChildren', 'ForceLayoutPrepass',
        'StaticFindObject', 'FindAllOf', 'FindFirstOf',
        'filesystem', 'fstream', 'ofstream')) {
    Assert-True ($refreshNativeParent -notmatch `
            [regex]::Escape($forbiddenRefreshWork)) `
        "Native-parent refresh contains forbidden reraster, discovery, allocation, or file work: $forbiddenRefreshWork"
}

Assert-True ($detachUnsafe -match `
        'host->ProcessEvent\s*\(\s*remove_from_parent_' `
    -and $rendererCode -notmatch `
        '(?:native_parent|witnessed_parent|retainer|retainer_box)->ProcessEvent\s*\(\s*(?:clear_children_|remove_from_parent_)') `
    'Detach may remove only Mod-owned hosts; native parent and Retainer widgets must never be cleared or removed.'

$setContentIntent = Get-RendererFunction `
    $rendererCode 'set_content_visibility_intent'
$publishRuntimeVisibility = Get-RendererFunction `
    $rendererCode 'publish_runtime_visibility'
$reconcileVisibilityUnsafe = Get-RendererFunction `
    $rendererCode 'reconcile_host_visibility_unsafe'
$reconcileVisibilityGuarded = Get-RendererFunction `
    $rendererCode 'reconcile_host_visibility_guarded'
$applyHostVisibility = Get-RendererFunction `
    $rendererCode 'apply_host_visibility_unsafe'
$faultAndDetach = Get-RendererFunction $rendererCode 'fault_and_detach'

Assert-True ([regex]::Matches(
        $setContentIntent,
        '\bcontent_visibility_intent_\s*=\s*enabled').Count -eq 1 `
    -and $setContentIntent -match `
        '\breconcile_host_visibility_guarded\s*\(' `
    -and $setContentIntent -notmatch `
        '\b(?:runtime_visibility_allowed_|transform_ready_|applied_host_visibility_)\s*(?:=|\.)' `
    -and $setContentIntent -notmatch '\bset_visibility\s*\(') `
    'Content publication must update only durable content intent and delegate host reconciliation.'
Assert-True ([regex]::Matches(
        $publishRuntimeVisibility,
        '\bruntime_visibility_allowed_\s*=\s*visible').Count -eq 1 `
    -and $publishRuntimeVisibility -match `
        '\breconcile_host_visibility_guarded\s*\(' `
    -and $publishRuntimeVisibility -notmatch `
        '\b(?:content_visibility_intent_|transform_ready_|applied_host_visibility_)\s*(?:=|\.)' `
    -and $publishRuntimeVisibility -notmatch '\bset_visibility\s*\(') `
    'Runtime publication must update only the live-layer allowance and delegate host reconciliation.'
Assert-True ($reconcileVisibilityUnsafe -match `
        'world_map_host_visibility_target\s*\(\s*\{[\s\S]*?content_visibility_intent_[\s\S]*?runtime_visibility_allowed_[\s\S]*?state_\s*==\s*WorldMapUmgRendererState::Attached[\s\S]*?transform_ready_' `
    -and $reconcileVisibilityUnsafe -match `
        'apply_host_visibility_unsafe\s*\(\s*!force_collapsed\s*&&\s*show' `
    -and $reconcileVisibilityUnsafe -notmatch '\bset_visibility\s*\(') `
    'Host reconciliation must derive one target from all four gates and delegate the reflected write.'
Assert-True ($reconcileVisibilityGuarded -match `
        'reconcile_host_visibility_unsafe\s*\(' `
    -and $reconcileVisibilityGuarded -match '\breturn\s+completed\s*;' `
    -and $reconcileVisibilityGuarded -notmatch '\bset_visibility\s*\(') `
    'The guarded visibility boundary must propagate reconciliation failure without writing hosts itself.'

$visibilityDedupeIndex = $applyHostVisibility.IndexOf(
    'world_map_host_visibility_write_required')
$visibilityFirstHostReadIndex = $applyHostVisibility.IndexOf(
    'hosts_[index].Get()')
$visibilityCacheResetIndex = $applyHostVisibility.IndexOf(
    'applied_host_visibility_.reset()')
$visibilityWriteIndex = $applyHostVisibility.IndexOf('set_visibility(')
$visibilityPublishIndex = $applyHostVisibility.IndexOf(
    'applied_host_visibility_ = visible')
Assert-True ($visibilityDedupeIndex -ge 0 `
    -and $visibilityFirstHostReadIndex -gt $visibilityDedupeIndex `
    -and $visibilityCacheResetIndex -gt $visibilityFirstHostReadIndex `
    -and $visibilityWriteIndex -gt $visibilityCacheResetIndex `
    -and $visibilityPublishIndex -gt $visibilityWriteIndex `
    -and [regex]::Matches(
        $applyHostVisibility, '\bset_visibility\s*\(').Count -eq 1 `
    -and $applyHostVisibility -match `
        'for\s*\(\s*UObject\s*\*\s*host\s*:\s*hosts\s*\)' `
    -and [regex]::Matches(
        $applyHostVisibility,
        'ProcessEvent\s*\(\s*request_retainer_render_').Count -eq 1) `
    'A repeated target must be a no-UObject no-op; a changed target must write both hosts, request one retained render, and publish only after all writes succeed.'
Assert-True ([regex]::Matches(
        $rendererCode,
        'ProcessEvent\s*\(\s*request_retainer_render_').Count -eq 1 `
    -and $attach -notmatch `
        'ProcessEvent\s*\(\s*request_retainer_render_' `
    -and $rendererCode.Replace($applyHostVisibility, '') -notmatch `
        'ProcessEvent\s*\(\s*request_retainer_render_') `
    'RequestRender must remain bounded to an actual visibility edge; attach and all native-parent validation paths must not repaint.'

$setVisibilityHelper = Get-FreeRendererFunction `
    $rendererCode 'void' 'set_visibility'
$rendererWithoutVisibilityOwner = $rendererCode.Replace(
    $setVisibilityHelper, '').Replace($applyHostVisibility, '')
Assert-True ([regex]::Matches(
        $rendererWithoutVisibilityOwner,
        '\bset_visibility\s*\(').Count -eq 3 `
    -and [regex]::Matches(
        $attach,
        '\bset_visibility\s*\(').Count -eq 3 `
    -and $rendererWithoutVisibilityOwner.Replace(
        $attach, '') -notmatch '\bset_visibility\s*\(' `
    -and $rendererWithoutVisibilityOwner -match `
        'set_visibility\s*\(\s*root_panels\s*\[\s*layer_index\s*\]' `
    -and $rendererWithoutVisibilityOwner -match `
        'set_visibility\s*\(\s*atlas_images\s*\[\s*layer_index\s*\]' `
    -and [regex]::Matches(
        $rendererWithoutVisibilityOwner,
        'set_visibility\s*\(\s*hosts\s*\[\s*layer_index\s*\]\s*,\s*set_visibility_\s*,\s*kCollapsed\s*\)').Count -eq 1) `
    'Runtime native-child SetVisibility writes must be confined to the idempotent two-host apply helper; direct root, image, and host collapse writes may occur only inside initial construction.'
Assert-True ($faultAndDetach -match `
        'runtime_visibility_allowed_\s*=\s*false' `
    -and $faultAndDetach -match `
        'transform_ready_\s*=\s*false' `
    -and $faultAndDetach -match `
        'applied_host_visibility_\.reset\s*\(\s*\)' `
    -and $faultAndDetach -match `
        'state_\s*=\s*WorldMapUmgRendererState::Faulted' `
    -and $faultAndDetach -match '\bdetach_guarded\s*\(' `
    -and $faultAndDetach -notmatch `
        '\bcontent_visibility_intent_\s*=') `
    'Hard visibility or transform failures must clear transient gates and cache, detach, and preserve durable content intent.'

Assert-True ([regex]::Matches(
        $rendererCode,
        '\bcontent_visibility_intent_\s*=').Count -eq 1) `
    'Renderer activation, attachment, suspension, resume, detach, and reset must never overwrite durable content intent.'
Assert-True ($attach -match `
        'runtime_visibility_allowed_\s*=\s*false' `
    -and $attach -match `
        'transform_ready_\s*=\s*false' `
    -and $attach -match `
        'applied_host_visibility_\.reset\s*\(\s*\)' `
    -and $attach -match `
        'state_\s*=\s*WorldMapUmgRendererState::Attached' `
    -and $attach -match `
        'transform_ready_\s*=\s*true' `
    -and $attach -notmatch `
        '\bcontent_visibility_intent_\s*=' `
    -and $attach -notmatch `
        '\bsync_viewport_transform_unsafe\s*\(') `
    'Fresh attachment must start hidden, publish valid native-child ownership, then mark inherited transform readiness without a viewport sync.'

$suspendGuarded = Get-RendererFunction $rendererCode 'suspend_guarded'
$resumeRenderer = Get-RendererFunction `
    $rendererCode 'resume_suspended_guarded'
$resetRuntimeHandles = Get-RendererFunction `
    $rendererCode 'reset_runtime_handles'
Assert-True ($suspendGuarded -match `
        'runtime_visibility_allowed_\s*=\s*false' `
    -and $suspendGuarded -match `
        'transform_ready_\s*=\s*false' `
    -and $suspendGuarded -match `
        'state_\s*=\s*WorldMapUmgRendererState::Suspended' `
    -and $suspendGuarded -match `
        'reconcile_host_visibility_unsafe\s*\(' `
    -and $suspendGuarded -notmatch '\bset_visibility\s*\(' `
    -and $suspendGuarded -notmatch `
        '\bcontent_visibility_intent_\s*=') `
    'F8 suspension must clear transient permission/readiness and collapse through the shared visibility owner while preserving content intent.'
Assert-True ($resumeRenderer -match `
        'state_\s*=\s*WorldMapUmgRendererState::Attached[\s\S]*?refresh_native_parent_unsafe' `
    -and $resumeRenderer -match `
        'WorldMapLayeringRefreshResult::Faulted' `
    -and $resumeRenderer -notmatch '\bset_visibility\s*\(' `
    -and $resumeRenderer -notmatch `
        '\bcontent_visibility_intent_\s*=') `
    'F7 resume must revalidate/refresh native-parent ownership and leave visibility to the shared gates.'
Assert-True ($resetRuntimeHandles -match `
        'runtime_visibility_allowed_\s*=\s*false' `
    -and $resetRuntimeHandles -match `
        'transform_ready_\s*=\s*false' `
    -and $resetRuntimeHandles -match `
        'applied_host_visibility_\.reset\s*\(\s*\)' `
    -and $resetRuntimeHandles -notmatch `
        '\bcontent_visibility_intent_\s*=') `
    'Detach/reset must invalidate transient permission, transform proof, and applied cache without erasing durable content intent.'
foreach ($lifecyclePath in @($suspendGuarded, $resumeRenderer)) {
    foreach ($forbiddenLifecycleWork in @(
            'build_rle_tga_atlas', 'write_rle_tga',
            'import_file_as_texture_', 'create_widget_', 'NewObject',
            'ofstream', 'filesystem', 'FindFirstOf', 'StaticFindObject')) {
        Assert-True ($lifecyclePath -notmatch `
                [regex]::Escape($forbiddenLifecycleWork)) `
            "Suspend/resume contains forbidden rebuild, file, allocation, or lookup work: $forbiddenLifecycleWork"
    }
}

$validateHostPayload = Get-RendererFunction `
    $rendererCode 'validate_host_payload_unsafe'
$validateHost = Get-RendererFunction $rendererCode 'validate_host_unsafe'
Assert-True ($validateHostPayload -match `
        'read_object_property\s*\(\s*host\s*,\s*L"WidgetTree"' `
    -and $validateHostPayload -match `
        'read_object_property\s*\(\s*root_panel\s*,\s*L"Slot"' `
    -and $validateHostPayload -match `
        'read_object_property\s*\(\s*root_slot\s*,\s*L"Parent"' `
    -and $validateHostPayload -match `
        'read_object_property\s*\(\s*root_slot\s*,\s*L"Content"' `
    -and $validateHostPayload -match `
        'read_struct_object_property\s*\([\s\S]*?L"Brush"\s*,\s*L"ResourceObject"' `
    -and $validateHostPayload -match '\bretainer_box_\b' `
    -and $validateHostPayload -match `
        'read_object_property\s*\(\s*current_layer\s*,\s*L"RetainerBox"') `
    'Owned-payload validation must continue to prove each Mod widget tree and atlas texture independently of native-parent replacement.'
Assert-True ($validateHost -match `
        'validate_host_payload_unsafe\s*\(' `
    -and $validateHost -match `
        'resolve_world_map_render_parent\s*\(' `
    -and $validateHost -notmatch `
        '(?:find_native_icon_class_witness|ArrayIconInfo)' `
    -and $validateHost -match '\bnative_parent_slots_\b' `
    -and $validateHost -match '\bnative_parent_\b' `
    -and $validateHost -match `
        'read_object_property\s*\(\s*host\s*,\s*L"Slot"' `
    -and $validateHost -match `
        'read_object_property\s*\(\s*native_slot\s*,\s*L"Parent"' `
    -and $validateHost -match `
        'read_object_property\s*\(\s*native_slot\s*,\s*L"Content"') `
    'Full host validation must prove the retained layer/Retainer, owned payload, and each neutral native Canvas slot graph.'

Assert-True ([regex]::Matches(
        $rendererCode, '\bbuild_rle_tga_atlas\s*\(').Count -le 2 `
    -and [regex]::Matches(
        $rendererCode, '\bImportFileAsTexture2D\b').Count -le 2) `
    'Atlas construction/import must remain confined to definition plus initial attachment and never service inherited transforms.'

# TGA identifier bytes persist the atlas fingerprint and encoded-payload
# checksum across game restarts. Cache hits fully decode the RLE envelope; all
# writes publish through a same-directory temporary file and atomic replace.
$atlasFingerprint = Get-FreeRendererFunction `
    $rendererCode 'std::uint64_t' 'atlas_input_fingerprint'
$persistentAtlasCache = Get-FreeRendererFunction `
    $rendererCode 'bool' 'read_persistent_atlas_cache'
$writeAtlas = Get-FreeRendererFunction `
    $rendererCode 'bool' 'write_rle_tga'
$buildAtlas = Get-FreeRendererFunction `
    $rendererCode 'AtlasBuildResult' 'build_rle_tga_atlas'
Assert-True ($rendererCode -match `
        "kAtlasCacheMagic\s*\{[\s\S]*?'D'\s*,\s*'S'\s*,\s*'N'\s*,\s*'W'\s*,\s*'R'\s*,\s*'A'\s*,\s*'5'\s*,\s*'2'" `
    -and $rendererCode -match `
        'kAtlasCacheTagBytes\s*=\s*32U' `
    -and $persistentAtlasCache -match `
        'header\s*\[\s*0\s*\]\s*!=\s*kAtlasCacheTagBytes' `
    -and $persistentAtlasCache -match `
        'fingerprint\s*!=\s*expected_fingerprint' `
    -and $persistentAtlasCache -match `
        'visible_count\s*!=\s*expected_visible_count' `
    -and $persistentAtlasCache -match `
        'expected_pixels[\s\S]*?while\s*\(\s*decoded_pixels\s*<\s*expected_pixels\s*\)' `
    -and $persistentAtlasCache -match `
        'packet_pixels\s*>\s*expected_pixels\s*-\s*decoded_pixels' `
    -and $persistentAtlasCache -match `
        'payload_checksum\s*!=\s*expected_payload_checksum' `
    -and $persistentAtlasCache -match `
        'payload_end[\s\S]*?==\s*file_bytes' `
    -and $writeAtlas -match `
        'temporary_path\s*=\s*path' `
    -and $writeAtlas -match `
        'std::ofstream\s+output\s*\{\s*temporary_path' `
    -and $writeAtlas -notmatch `
        'std::ofstream\s+output\s*\{\s*path\s*,' `
    -and $writeAtlas -match `
        'MoveFileExW\s*\([\s\S]*?MOVEFILE_REPLACE_EXISTING\s*\|\s*MOVEFILE_WRITE_THROUGH' `
    -and $writeAtlas -match `
        'if\s*\(\s*!MoveFileExW[\s\S]*?filesystem::remove\s*\(\s*temporary_path' `
    -and $buildAtlas -match `
        'read_persistent_atlas_cache\s*\(' `
    -and $buildAtlas -match `
        'cache_hit\s*=\s*true') `
    'The renderer must fully validate revision-52 RLE payloads and atomically publish same-directory cache files before reuse.'
Assert-True ($atlasFingerprint -match `
        'hash_quantized_atlas_value\s*\([\s\S]*?local_positions\s*\[\s*index\s*\]\.x\s*-\s*bounds\.left' `
    -and $atlasFingerprint -match `
        'hash_quantized_atlas_value\s*\([\s\S]*?local_positions\s*\[\s*index\s*\]\.y\s*-\s*bounds\.top' `
    -and $atlasFingerprint -match `
        'hash_quantized_atlas_value\s*\(\s*hash\s*,\s*bounds\.width\s*\)' `
    -and $atlasFingerprint -match `
        'hash_quantized_atlas_value\s*\(\s*hash\s*,\s*bounds\.height\s*\)' `
    -and $atlasFingerprint -notmatch `
        '\bmarker\.id\b|hash_u64\s*\(\s*hash\s*,\s*marker_count\s*\)|bit_cast' `
    -and $rendererCode -match `
        'kAtlasFingerprintUnitsPerLogicalUnit\s*=\s*4096\.0' `
    -and $rendererCode -match `
        'llround\s*\(\s*scaled\s*\)') `
    'Persistent atlas identity must quantize only raster-affecting layer-local geometry and ignore common translation, IDs, and unrelated marker counts.'

# Main owns the visible-session state machine. Retained native-child hosts must be
# collapsed on every uncertain or closed-map edge, suspended by F8, and fully
# detached before travel invalidates the old world.
$layerVisibilityGate = Get-MainMethod `
    $mainCode 'world_map_layer_open_and_visible_guarded'
$visibilityGate = Get-MainMethod `
    $mainCode 'world_map_atlas_visibility_allowed_guarded'
$candidateOpenEvidence = Get-MainMethod `
    $mainCode 'world_map_candidate_has_open_evidence'
$candidateConfirmedVisible = Get-MainMethod `
    $mainCode 'world_map_candidate_confirmed_visible'
$clearOpenEvidence = Get-MainMethod `
    $mainCode 'clear_world_map_open_evidence'
Assert-True ($mainCode -match `
        'std::uint64_t\s+world_map_candidate_serial_\s*\{\s*\}\s*;\s*std::uint64_t\s+world_map_open_serial_\s*\{\s*\}\s*;' `
    -and $candidateOpenEvidence -match `
        'return\s+dswros::world_map_serial_matches\s*\([\s\S]*?world_map_candidate_available_[\s\S]*?world_map_candidate_serial_[\s\S]*?world_map_open_serial_' `
    -and $sessionPolicyCode -match `
        'return\s+candidate_available\s*&&\s*candidate_serial\s*!=\s*0U[\s\S]*?open_serial\s*==\s*candidate_serial' `
    -and $candidateConfirmedVisible -match `
        'return\s+world_map_candidate_has_open_evidence\s*\(\s*\)\s*&&\s*world_map_visible_serial_\s*==\s*world_map_candidate_serial_\s*;') `
    'Open and confirmed-visible evidence must each remain bound to the same nonzero candidate serial.'
Assert-True ($layerVisibilityGate -match '\benabled_\b' `
    -and $layerVisibilityGate -match '\btransition_active_\b' `
    -and $layerVisibilityGate -match '\bactivity_suppressed_\b' `
    -and $layerVisibilityGate -match '\bworld_map_compact_suppressed_\b' `
    -and $layerVisibilityGate -match `
        '\bworld_map_candidate_has_open_evidence\s*\(\s*\)' `
    -and $layerVisibilityGate -match 'object_world_guarded\s*\(' `
    -and $layerVisibilityGate -match `
        'read_world_map_layer_visibility_guarded\s*\([\s\S]*?&&\s*native_layer_visible' `
    -and $visibilityGate -match `
        'world_map_layer_open_and_visible_guarded\s*\(' `
    -and $visibilityGate -match 'WorldMapUmgRendererState::Attached' `
    -and $visibilityGate -match 'attached_layer_matches\s*\(') `
    'World-map runtime visibility must fail closed unless the exact open-evidence candidate and attached current-world layer are natively visible.'
Assert-True ($clearOpenEvidence -match `
        'world_map_open_serial_\s*=\s*0' `
    -and $clearOpenEvidence -match `
        'world_map_visible_serial_\s*=\s*0' `
    -and $clearOpenEvidence -match `
        'world_map_open_evidence_at_\s*=\s*\{\s*\}' `
    -and $clearOpenEvidence -match `
        'world_map_session_pending_\s*=\s*false' `
    -and $clearOpenEvidence -match `
        'world_map_service_retry_after_\s*=\s*\{\s*\}' `
    -and $clearOpenEvidence -match `
        'world_map_readiness_attempts_\s*=\s*0' `
    -and $clearOpenEvidence -match `
        'world_map_service_attempts_\s*=\s*0' `
    -and $clearOpenEvidence -match `
        'world_map_serviced_serial_\s*=\s*0' `
    -and $clearOpenEvidence -match `
        'world_map_set_image_rearm_consumed_\s*=\s*false' `
    -and $clearOpenEvidence -match `
        'world_map_layering_refresh_pending_\s*=\s*false' `
    -and $clearOpenEvidence -match `
        'world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)') `
    'An authoritative close must retire candidate-open evidence, both work budgets, retry/rearm state, the settle tail, and runtime visibility together.'

$contentIntent = Get-MainMethod `
    $mainCode 'world_map_content_visibility_intent'
$publishContentIntent = Get-MainMethod `
    $mainCode 'publish_world_map_content_visibility_intent'
Assert-True ($contentIntent -match `
        'return\s+dsnwr::world_radar_visibility_mask\s*\(\s*visibility_masks_\s*\)\s*!=\s*0U\s*;' `
    -and $publishContentIntent -match `
        'set_content_visibility_intent\s*\(\s*content_visible\s*\)' `
    -and $publishContentIntent -match `
        'if\s*\(\s*!content_visible\s*\)\s*\{[\s\S]*?world_map_session_pending_\s*=\s*false[\s\S]*?world_map_service_retry_after_\s*=\s*\{\s*\}[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false' `
    -and [regex]::Matches(
        $mainCode,
        'world_map_umg_renderer_\.set_content_visibility_intent\s*\(').Count -eq 1) `
    'Main must derive durable content intent only from the world visibility mask and publish it through one authoritative edge.'
Assert-True ($mainCode -match `
        'visibility_masks_\s*=\s*visibility\.masks\s*;\s*publish_world_map_content_visibility_intent\s*\(\s*\)' `
    -and $mainCode -match `
        'if\s*\(\s*world_mask_changed\s*\)\s*\{\s*publish_world_map_content_visibility_intent\s*\(\s*\)\s*;\s*\}') `
    'Startup and each F6 world-mask edge must publish content intent before any renderer lifecycle can consume it.'

$applyVisibility = Get-MainMethod `
    $mainCode 'apply_world_map_atlas_visibility_guarded'
Assert-True ($applyVisibility -match `
        'publish_runtime_visibility\s*\(\s*world_map_atlas_visibility_allowed_guarded\s*\(' `
    -and $applyVisibility -notmatch `
        'set_content_visibility_intent\s*\(') `
    'Every live-layer visibility update must publish only runtime allowance through the fail-closed gate.'

# Native-parent reconciliation is event-triggered and finite. Keep the known
# post-open settle tail, but never turn it into a steady poll or process more
# than one parent observation in one game-thread pass.
Assert-True ($maxWorldMapServiceAttempts -eq 3 `
    -and $maxWorldMapReadinessAttempts -eq 40 `
    -and $worldMapServiceRetryDelayMs -eq 150 `
    -and $worldMapOpenVisibilityGraceMs -eq 1000 `
    -and $mainCode -match `
        'kWorldMapLayeringSettleDelays\s*\{[\s\S]*?milliseconds\s*\{\s*100\s*\}[\s\S]*?milliseconds\s*\{\s*250\s*\}[\s\S]*?milliseconds\s*\{\s*500\s*\}[\s\S]*?milliseconds\s*\{\s*1000\s*\}[\s\S]*?milliseconds\s*\{\s*1250\s*\}\s*\}\s*;') `
    'The bounded attach retry and finite 100/250/500/1000/1250 ms native-parent settle schedule changed.'

$armLayeringRefresh = Get-MainMethod `
    $mainCode 'arm_world_map_layering_refresh'
Assert-True ($armLayeringRefresh -match `
        'if\s*\(\s*!world_map_content_visibility_intent\s*\(\s*\)[\s\S]*?\|\|\s*!world_map_candidate_has_open_evidence\s*\(\s*\)[\s\S]*?\|\|\s*!world_map_compact_suppressed_[\s\S]*?\)\s*\{\s*return\s*;\s*\}' `
    -and $armLayeringRefresh -match `
        'WorldMapLayeringArmPolicy\s+policy' `
    -and $armLayeringRefresh -match `
        'same_serial_tail_pending\s*=\s*[\s\S]*?world_map_layering_refresh_pending_[\s\S]*?world_map_layering_refresh_serial_\s*==\s*world_map_candidate_serial_' `
    -and $armLayeringRefresh -match `
        'policy\s*==\s*WorldMapLayeringArmPolicy::Coalesce[\s\S]*?same_serial_tail_pending[\s\S]*?return\s*;[\s\S]*?world_map_layering_refresh_pending_\s*=\s*true' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_started_\s*=\s*now' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_attempt_\s*=\s*0' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_due_\s*=\s*now\s*\+\s*kWorldMapLayeringSettleDelays\.front\s*\(\s*\)' `
    -and $armLayeringRefresh -match `
        'world_map_layering_refresh_serial_\s*=\s*world_map_candidate_serial_') `
    'Same-serial Coalesce events must preserve a running tail; only a new serial or explicit Restart may reset its absolute deadlines and progress.'

$mapOpenLatch = Get-MainMethod `
    $mainCode 'latch_world_map_compact_suppression'
Assert-True ($mapOpenLatch -notmatch `
        'world_map_umg_renderer_\.(?:publish_runtime_visibility|set_content_visibility_intent)\s*\(') `
    'Compact-map suppression must not directly change expanded native-child host visibility.'

$captureWorldMapCandidate = Get-MainMethod `
    $mainCode 'capture_world_map_candidate_unsafe'
$worldMapImagePost = Get-MainMethod $mainCode 'world_map_image_post'
$listenerCandidateCapture = Get-MainMethod `
    $mainCode 'consume_world_map_listener_candidate_unsafe'
Assert-True ($captureWorldMapCandidate -match `
        'if\s*\(\s*!exact_attached_layer\s*\)\s*\{[\s\S]*?world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)' `
    -and [regex]::Matches(
        $captureWorldMapCandidate,
        'world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)').Count -eq 1 `
    -and $captureWorldMapCandidate -match `
        'if\s*\(\s*attached_layer_validation\s*\)\s*\{[\s\S]*?arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::SetWorldMapImage\s*,[\s\S]*?WorldMapLayeringArmPolicy::Coalesce\s*\)') `
    'A map-open event may collapse hosts only for a new or mismatched layer; an exact attached layer must enter the native-parent settle path without hiding.'
$sameLayerOpenSession = [regex]::Match(
    $captureWorldMapCandidate,
    '(?ms)const\s+bool\s+new_open_edge\s*=\s*dswros::world_map_open_session_started\s*\(\s*candidate_transition\s*\)\s*;\s*world_map_open_serial_\s*=\s*world_map_candidate_serial_\s*;\s*if\s*\(\s*new_open_edge\s*\)\s*\{(?<body>.*?)\}\s*if\s*\(\s*content_visible\s*&&\s*!exact_attached_layer\s*\)')
Assert-True ($sameLayerOpenSession.Success `
    -and $sameLayerOpenSession.Groups['body'].Value -match `
        '^\s*world_map_visible_serial_\s*=\s*0\s*;\s*world_map_open_evidence_at_\s*=\s*Clock::now\s*\(\s*\)\s*;\s*world_map_session_rebuild_consumed_\s*=\s*false\s*;\s*$') `
    'A same-layer SetWorldMapImage edge must start visibility grace and a fresh one-rebuild allowance exactly once; duplicate open events must not reset either bound.'
$sameLayerEvidenceIndex = Get-RequiredPatternIndex `
    $captureWorldMapCandidate `
    'world_map_open_serial_\s*=\s*world_map_candidate_serial_\s*;' `
    'same-layer map-open evidence publication'
$contentDisabledCandidateIndex = Get-RequiredPatternIndex `
    $captureWorldMapCandidate `
    'if\s*\(\s*same_layer\s*&&\s*!content_visible\s*\)' `
    'same-layer disabled-content gate'
$passiveSameLayerIndex = Get-RequiredPatternIndex `
    $captureWorldMapCandidate `
    'if\s*\(\s*same_layer\s*&&\s*!set_world_map_image_event\s*\)' `
    'same-layer passive-listener gate'
$candidateBudgetReadIndex = Get-RequiredPatternIndex `
    $captureWorldMapCandidate `
    'const\s+bool\s+retry_budget_consumed\b' `
    'candidate retry-budget observation'
Assert-True ($sameLayerEvidenceIndex -ge 0 `
    -and $contentDisabledCandidateIndex -gt $sameLayerEvidenceIndex `
    -and $candidateBudgetReadIndex -gt $contentDisabledCandidateIndex `
    -and $passiveSameLayerIndex -gt $contentDisabledCandidateIndex `
    -and $candidateBudgetReadIndex -gt $passiveSameLayerIndex `
    -and $captureWorldMapCandidate -match `
        'if\s*\(\s*same_layer\s*&&\s*!content_visible\s*\)\s*\{\s*return\s*;\s*\}' `
    -and $captureWorldMapCandidate -match `
        'if\s*\(\s*same_layer\s*&&\s*!set_world_map_image_event\s*\)\s*\{\s*return\s*;\s*\}') `
    'Same-layer SetWorldMapImage must bind evidence before content exits; listener-only callbacks must return before budgets, rearm, or serial publication.'
Assert-True ($captureWorldMapCandidate -match `
        'reset_world_map_runtime\s*\(\s*false\s*\)[\s\S]*?world_map_layer_candidate_\s*=\s*current_layer\s*;\s*world_map_candidate_available_\s*=\s*true[\s\S]*?world_map_open_serial_\s*=\s*set_world_map_image_event\s*\?\s*world_map_candidate_serial_\s*:\s*0\s*;\s*world_map_visible_serial_\s*=\s*0\s*;\s*world_map_open_evidence_at_\s*=\s*set_world_map_image_event\s*\?\s*Clock::now\s*\(\s*\)\s*:\s*Clock::time_point\s*\{\s*\}\s*;' `
    -and $captureWorldMapCandidate -match `
        'world_map_session_pending_\s*=\s*content_visible\s*&&\s*set_world_map_image_event\s*;' `
    -and [regex]::Matches(
        $captureWorldMapCandidate,
        'world_map_visible_serial_\s*=\s*0\s*;').Count -eq 2 `
    -and [regex]::Matches(
        $captureWorldMapCandidate,
        'world_map_open_evidence_at_\s*=').Count -eq 2 `
    -and $listenerCandidateCapture -match `
        'capture_world_map_candidate_unsafe\s*\(\s*current_layer\s*,\s*false\s*\)' `
    -and $listenerCandidateCapture -match `
        'const\s+bool\s+exact_current_world\s*=' `
    -and $listenerCandidateCapture -match `
        'world_map_listener_open_probe_allowed\s*\([\s\S]*?enabled_[\s\S]*?transition_active_[\s\S]*?activity_suppressed_[\s\S]*?exact_current_world' `
    -and $listenerCandidateCapture -match `
        'world_map_open_serial_\s*=\s*world_map_candidate_serial_\s*;[\s\S]*?world_map_visible_serial_\s*=\s*0\s*;[\s\S]*?world_map_open_evidence_at_\s*=\s*Clock::now\s*\(\s*\)\s*;[\s\S]*?world_map_session_pending_\s*=\s*false\s*;' `
    -and $listenerCandidateCapture -notmatch `
        'latch_world_map_compact_suppression|service_world_map_atlas|world_map_(?:readiness|service)_attempts_\s*=' `
    -and $worldMapImagePost -match `
        'capture_world_map_candidate_unsafe\s*\(\s*context\.Context\s*,\s*true\s*\)' `
    -and $worldMapImagePost -notmatch '\benabled_\b' `
    -and $sessionPolicyCode -match `
        'next_world_map_candidate_serial[\s\S]*?numeric_limits<std::uint64_t>::max\s*\(\s*\)[\s\S]*?current_serial\s*\+\s*1U') `
    'Only an activated exact-current-world creation may publish a visibility-gated opening probe; it must not latch compact suppression, consume budgets, or run atlas work, while SetWorldMapImage remains recordable during F8-disabled state.'
Assert-True ([regex]::Matches(
        $mainCode,
        '\bworld_map_open_serial_\s*=(?!=)').Count -eq 5) `
    'Only same-candidate SetWorldMapImage, new-candidate publication, exact post-activation listener probe, authoritative close, and full candidate reset may write map-open evidence.'

$zoomPost = Get-MainMethod $mainCode 'world_map_zoom_post_unsafe'
Assert-True ($zoomPost -match `
        'arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::ZoomChanged\s*,[\s\S]*?WorldMapLayeringArmPolicy::Coalesce\s*\)') `
    'Repeated zoom notifications for one live layer must coalesce into the running finite settle tail.'

$serviceWorldMapAtlas = Get-MainMethod `
    $mainCode 'service_world_map_atlas'
$contentDisabledService = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)^\s{8}if\s*\(\s*!world_map_content_visibility_intent\s*\(\s*\)\s*\)\s*\{(?<body>[\s\S]*?)^\s{8}\}')
Assert-True $contentDisabledService.Success `
    'The service-level disabled-content gate was not found.'
$contentDisabledServiceBody = `
    $contentDisabledService.Groups['body'].Value
$contentDisabledServiceIndex = Get-RequiredPatternIndex `
    $serviceWorldMapAtlas `
    'if\s*\(\s*!world_map_content_visibility_intent\s*\(\s*\)\s*\)' `
    'service disabled-content gate'
$serviceClockIndex = Get-RequiredPatternIndex `
    $serviceWorldMapAtlas `
    'const\s+auto\s+now\s*=\s*Clock::now\s*\(\s*\)' `
    'service clock sample'
$workGateIndex = Get-RequiredPatternIndex `
    $serviceWorldMapAtlas `
    'dswros::world_map_work_gate\s*\(' `
    'world-map pure work gate'
$firstReadinessAttemptIndex = Get-RequiredPatternIndex `
    $serviceWorldMapAtlas `
    '\+\+world_map_readiness_attempts_\s*;' `
    'first readiness attempt consumption'
$firstServiceAttemptIndex = Get-RequiredPatternIndex `
    $serviceWorldMapAtlas `
    '\+\+world_map_service_attempts_\s*;' `
    'real attachment attempt consumption'
Assert-True ($contentDisabledServiceBody -match `
        'world_map_session_pending_\s*=\s*false' `
    -and $contentDisabledServiceBody -match `
        'world_map_service_retry_after_\s*=\s*\{\s*\}' `
    -and $contentDisabledServiceBody -match '\breturn\s*;' `
    -and $contentDisabledServiceBody -notmatch `
        'world_map_(?:readiness|service)_attempts_|attach_once|begin_map_session' `
    -and $contentDisabledServiceIndex -ge 0 `
    -and $serviceClockIndex -gt $contentDisabledServiceIndex `
    -and $firstReadinessAttemptIndex -gt $contentDisabledServiceIndex `
    -and $firstServiceAttemptIndex -gt $contentDisabledServiceIndex) `
    'Disabled world-map content must retire pending service before time, retry-budget, session, or attachment work.'
Assert-True ($workGateIndex -gt $contentDisabledServiceIndex `
    -and $workGateIndex -lt $serviceClockIndex `
    -and $firstReadinessAttemptIndex -gt $workGateIndex `
    -and $firstServiceAttemptIndex -gt $workGateIndex `
    -and $serviceWorldMapAtlas -match `
        'WorldMapWorkGate::Ignore[\s\S]*?world_map_session_pending_\s*=\s*false[\s\S]*?WorldMapWorkGate::Hold[\s\S]*?return\s*;[\s\S]*?WorldMapWorkGate::CloseSession[\s\S]*?clear_world_map_open_evidence\s*\(' `
    -and $sessionPolicyCode -match `
        'if\s*\(\s*!context\.content_intent[\s\S]*?!context\.candidate_available[\s\S]*?!context\.has_serial_bound_open_evidence[\s\S]*?WorldMapWorkGate::Ignore' `
    -and $sessionPolicyCode -match `
        'visibility\s*==\s*WorldMapVisibilitySample::Unknown[\s\S]*?WorldMapWorkGate::Hold' `
    -and $sessionPolicyCode -match `
        'visibility\s*==\s*WorldMapVisibilitySample::Hidden[\s\S]*?WorldMapWorkGate::CloseSession' `
    -and $sessionPolicyCode -match `
        'attempts\s*>=\s*context\.maximum_attempts[\s\S]*?WorldMapWorkGate::Exhausted' `
    -and $sessionPolicyCode -match `
        'return\s+WorldMapWorkGate::ConsumeAttempt') `
    'A listener-only, replacement-mismatched, or closed candidate must retire before time sampling; an opening transition and an unconfirmed-visible candidate must preserve the request without consuming either budget.'

$candidateServiceReset = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)^\s{8}if\s*\(\s*world_map_serviced_serial_\s*!=\s*world_map_candidate_serial_\s*\)\s*\{(?<body>[\s\S]*?)^\s{8}\}')
$readinessLimitIndex = Get-RequiredPatternIndex `
    $serviceWorldMapAtlas `
    'if\s*\(\s*world_map_readiness_attempts_\s*>=' `
    'readiness-budget exhaustion gate'
Assert-True ($candidateServiceReset.Success `
    -and $candidateServiceReset.Groups['body'].Value -match `
        'world_map_serviced_serial_\s*=\s*world_map_candidate_serial_' `
    -and $candidateServiceReset.Groups['body'].Value -match `
        'world_map_readiness_attempts_\s*=\s*0' `
    -and $candidateServiceReset.Groups['body'].Value -match `
        'world_map_service_attempts_\s*=\s*0' `
    -and $readinessLimitIndex -gt $candidateServiceReset.Index `
    -and $workGateIndex -gt $candidateServiceReset.Index) `
    'A new evidence-bound candidate must start with both budgets reset before either exhaustion check is evaluated.'

$mapIdReadinessBranch = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)if\s*\(\s*!world_map_umg_renderer_\.detect_current_map_id[\s\S]*?^\s{8}\}')
$readinessRetryFlowPattern =
    '(?ms)\+\+world_map_readiness_attempts_\s*;\s*const\s+bool\s+retry\s*=\s*world_map_readiness_attempts_\s*<\s*kWorldMapMaxReadinessAttempts\s*;\s*world_map_session_pending_\s*=\s*retry\s*;\s*world_map_service_retry_after_\s*=\s*retry\s*\?\s*now\s*\+\s*kWorldMapServiceRetryDelay\s*:\s*Clock::time_point\s*\{\s*\}\s*;'
Assert-True ($mapIdReadinessBranch.Success `
    -and $mapIdReadinessBranch.Value -match $readinessRetryFlowPattern `
    -and $mapIdReadinessBranch.Value -notmatch `
        '\+\+world_map_service_attempts_|\battach_once\s*\(') `
    'Map-id readiness failures must schedule the next bounded 150 ms readiness sample and must not consume a real attachment attempt.'

$controllerReadinessBranch = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)if\s*\(\s*!expected_owning_player\s*\)\s*\{[\s\S]*?^\s{8}\}')
Assert-True ($controllerReadinessBranch.Success `
    -and $controllerReadinessBranch.Value -match `
        $readinessRetryFlowPattern `
    -and $controllerReadinessBranch.Value -notmatch `
        '\+\+world_map_service_attempts_|\battach_once\s*\(') `
    'Owning-player readiness failures must schedule the next bounded 150 ms readiness sample and must not consume a real attachment attempt.'
$attachAttemptFlow = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)\+\+world_map_service_attempts_\s*;\s*const\s+bool\s+attached\s*=\s*world_map_umg_renderer_\.attach_once\s*\([^;]*?\)\s*;\s*const\s+bool\s+retry\s*=\s*!attached\s*&&\s*world_map_umg_renderer_\.retryable_not_ready\s*\(\s*\)\s*&&\s*world_map_service_attempts_\s*<\s*kWorldMapMaxServiceAttempts\s*;\s*world_map_session_pending_\s*=\s*retry\s*;\s*world_map_service_retry_after_\s*=\s*retry\s*\?\s*now\s*\+\s*kWorldMapServiceRetryDelay\s*:\s*Clock::time_point\s*\{\s*\}\s*;')
Assert-True ([regex]::Matches(
        $serviceWorldMapAtlas,
        '\+\+world_map_readiness_attempts_').Count -eq 2 `
    -and [regex]::Matches(
        $serviceWorldMapAtlas,
        '\+\+world_map_service_attempts_').Count -eq 1 `
    -and $attachAttemptFlow.Success `
    -and $attachAttemptFlow.Value -notmatch `
        'world_map_readiness_attempts_|kWorldMapMaxReadinessAttempts') `
    'The three-attempt attachment budget must be consumed only immediately before attach_once and must schedule retries only from its own bound.'

$zeroMarkerBranch = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)^\s{8}if\s*\(\s*world_map_marker_count_\s*==\s*0\s*\)\s*\{(?<body>[\s\S]*?)^\s{8}\}')
Assert-True $zeroMarkerBranch.Success `
    'The successful zero-marker completion branch was not found.'
$zeroMarkerBranchBody = $zeroMarkerBranch.Groups['body'].Value
Assert-True ($zeroMarkerBranchBody -match `
        'world_map_session_pending_\s*=\s*false' `
    -and $zeroMarkerBranchBody -match `
        'world_map_service_retry_after_\s*=\s*\{\s*\}' `
    -and $zeroMarkerBranchBody -match 'WORLD_MAP_ATLAS_SKIPPED' `
    -and $zeroMarkerBranchBody -match `
        'reason=no_selected_or_eligible_world_markers' `
    -and $zeroMarkerBranchBody -notmatch `
        'WORLD_MAP_ATLAS_ATTACH_FAILED|\+\+world_map_(?:readiness|service)_attempts_|attach_once|begin_map_session') `
    'A valid empty marker selection must complete as a non-failure without consuming either budget or starting a renderer session.'

$sameLayerReuse = [regex]::Match(
    $serviceWorldMapAtlas,
    '(?ms)^\s{8}if\s*\(\s*world_map_umg_renderer_\.attached_to\s*\([^)]*\)\s*\)\s*\{(?<body>[\s\S]*?)^\s{8}if\s*\(\s*map_id\s*!=')
Assert-True $sameLayerReuse.Success `
    'The same-attached-layer world-map reuse branch was not found.'
$sameLayerReuseBody = $sameLayerReuse.Groups['body'].Value
Assert-True ($sameLayerReuseBody -match `
        '(?:sync_viewport_transform|refresh_layering)\s*\([\s\S]*?apply_world_map_atlas_visibility_guarded\s*\(' `
    -and $sameLayerReuseBody -notmatch `
        '(?:publish_runtime_visibility|set_content_visibility_intent)\s*\(\s*false\s*\)' `
    -and [regex]::Matches(
        $sameLayerReuseBody,
        'arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::SetWorldMapImage\s*,[\s\S]*?WorldMapLayeringArmPolicy::Coalesce\s*\)').Count -eq 2) `
    'The same attached layer must validate its inherited native parent before reapplying gated visibility and must never hide first.'

$menuVisibility = Get-MainMethod $mainCode 'refresh_compact_menu_state'
Assert-True ($menuVisibility -match `
        'WorldMapVisibilitySample::Unknown[\s\S]*?read_world_map_layer_visibility_guarded[\s\S]*?WorldMapVisibilitySample::Visible[\s\S]*?WorldMapVisibilitySample::Hidden' `
    -and $menuVisibility -match `
        'dswros::decide_world_map_visibility\s*\([\s\S]*?world_map_candidate_available_[\s\S]*?exact_candidate_live[\s\S]*?exact_current_world_layer[\s\S]*?world_map_candidate_has_open_evidence\s*\(\s*\)[\s\S]*?world_map_candidate_confirmed_visible\s*\(\s*\)[\s\S]*?milliseconds_since_open[\s\S]*?kWorldMapOpenVisibilityGrace\.count' `
    -and $menuVisibility -match `
        'WorldMapVisibilityAction::ConfirmVisible[\s\S]*?world_map_visible_serial_\s*=\s*world_map_candidate_serial_[\s\S]*?world_map_compact_suppressed_\s*=\s*true[\s\S]*?visibility_decision\.recovery_edge[\s\S]*?world_map_session_pending_\s*=\s*true[\s\S]*?world_map_service_retry_after_\s*=\s*\{\s*\}') `
    'The first confirmed-visible sample must restore one held request for the same unattached layer without resetting either finite budget.'
Assert-True ($sessionPolicyCode -match `
        'sample\s*==\s*WorldMapVisibilitySample::Unknown[\s\S]*?WorldMapVisibilityAction::Preserve' `
    -and $sessionPolicyCode -match `
        'sample\s*==\s*WorldMapVisibilitySample::Visible[\s\S]*?WorldMapVisibilityAction::ConfirmVisible' `
    -and $sessionPolicyCode -match `
        'previously_confirmed_visible[\s\S]*?milliseconds_since_open[\s\S]*?>=\s*context\.open_visibility_grace_milliseconds[\s\S]*?WorldMapVisibilityAction::CloseSession') `
    'A false native visibility sample must preserve a fresh opening edge until the 1,000 ms grace expires, then close immediately after grace or any prior visible confirmation.'
Assert-True ($menuVisibility -match `
        'authoritative_world_map_close\s*=\s*world_map_candidate_available_\s*&&\s*!exact_candidate_live' `
    -and $menuVisibility -match `
        'WorldMapVisibilityAction::CloseSession[\s\S]*?authoritative_world_map_close\s*=\s*true[\s\S]*?if\s*\(\s*authoritative_world_map_close\s*\)\s*\{[\s\S]*?clear_world_map_open_evidence\s*\(\s*\)' `
    -and [regex]::Matches(
        $menuVisibility,
        '\bclear_world_map_open_evidence\s*\(').Count -eq 1 `
    -and $menuVisibility -match `
        'apply_world_map_atlas_visibility_guarded\s*\(') `
    'Only a confirmed post-open false visibility result, expiry of the bounded opening grace, or a destroyed evidence candidate may retire open evidence and both budgets; a transient false or unknown read must not erase the opening edge.'

$layeringService = Get-MainMethod `
    $mainCode 'service_world_map_layering_refresh'
$scheduleWorldMapRebuild = Get-MainMethod `
    $mainCode 'schedule_world_map_geometry_or_payload_rebuild'
Assert-True ($layeringService -match `
        'WorldMapLayeringRefreshResult::Updated[\s\S]*?WorldMapLayeringRefreshResult::Unchanged[\s\S]*?WorldMapLayeringRefreshResult::Retained[\s\S]*?apply_world_map_atlas_visibility_guarded' `
    -and $layeringService -match `
        'if\s*\(\s*current_layer\s*\)\s*\{[\s\S]*?refresh_layering\s*\(\s*current_layer\s*,\s*false\s*,[\s\S]*?\}\s*else\s*\{[\s\S]*?world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)' `
    -and $layeringService -match `
        'WorldMapLayeringRefreshResult::RebuildRequired[\s\S]*?schedule_world_map_geometry_or_payload_rebuild\s*\(' `
    -and $layeringService -match `
        'final_retry_rebuild\s*=\s*final_attempt[\s\S]*?WorldMapLayeringRefreshResult::RetryLater[\s\S]*?schedule_rebuild\s*=\s*rebuild_required\s*\|\|\s*final_retry_rebuild' `
    -and $layeringService -match `
        'if\s*\(\s*schedule_rebuild\s*\)[\s\S]*?schedule_world_map_geometry_or_payload_rebuild\s*\([\s\S]*?layering_final_retry_later' `
    -and [regex]::Matches(
        $layeringService,
        'world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)').Count -eq 1 `
    -and $layeringService -notmatch `
        'set_content_visibility_intent\s*\(') `
    'Successful or retained native-parent observations may reapply the runtime gate; only a missing layer is collapsed by main, while a final renderer-owned RetryLater closes into the once-per-open rebuild path.'
Assert-True ([regex]::Matches(
        $layeringService, '\brefresh_layering\s*\(').Count -eq 1 `
    -and $layeringService -notmatch '\bsync_viewport_transform\s*\(' `
    -and $layeringService -notmatch `
        'refresh_layering\s*\(\s*current_layer\s*,\s*final_attempt') `
    'One settle-service pass must perform exactly one native-parent refresh, always disable same-parent tree mutation, and route rebuild-required explicitly.'
Assert-True ($layeringService -match `
        '!world_map_layering_refresh_pending_[\s\S]*?now\s*<\s*world_map_layering_refresh_due_' `
    -and $layeringService -match `
        'world_map_layering_refresh_serial_\s*!=\s*world_map_candidate_serial_[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false' `
    -and $layeringService -match `
        'attempt_index\s*>=\s*kWorldMapLayeringSettleDelays\.size\s*\(\s*\)[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false' `
    -and $layeringService -match `
        'WorldMapLayeringRefreshResult::Faulted[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false[\s\S]*?final_attempt[\s\S]*?world_map_layering_refresh_pending_\s*=\s*false' `
    -and $layeringService -match `
        'world_map_layering_refresh_due_\s*=\s*world_map_layering_refresh_started_\s*\+\s*kWorldMapLayeringSettleDelays') `
    'The settle tail must clear on invalid serial/state, bounds, faults, or its final sample and retain absolute event-relative deadlines.'
Assert-True ($layeringService -notmatch `
        '\barm_world_map_layering_refresh\s*\(' `
    -and $layeringService -notmatch `
        '\b(?:while|do)\s*(?:\(|\{)' `
    -and $layeringService -notmatch `
        '(?:FindAllOf|FindFirstOf|StaticFindObject|filesystem|fstream|ofstream|AddChildToCanvas|RemoveFromParent|AddToViewport|ForceLayoutPrepass)') `
    'The settle service must not re-arm itself, catch up overdue observations, discover objects, touch files, or directly mutate widget trees.'

Assert-True ($scheduleWorldMapRebuild -match `
        'exact_live_attachment[\s\S]*?current_session[\s\S]*?!world_map_session_rebuild_consumed_' `
    -and $scheduleWorldMapRebuild -match `
        'world_map_session_rebuild_consumed_\s*=\s*true[\s\S]*?world_map_service_attempts_\s*=\s*0[\s\S]*?world_map_umg_renderer_\.begin_map_session\s*\(\s*\)' `
    -and $scheduleWorldMapRebuild -match `
        'WorldMapUmgRendererState::Ready[\s\S]*?world_map_renderer_session_started_\s*=\s*true[\s\S]*?world_map_readiness_attempts_\s*=\s*0[\s\S]*?world_map_session_pending_\s*=\s*true' `
    -and [regex]::Matches(
        $scheduleWorldMapRebuild,
        'world_map_service_attempts_\s*=\s*0').Count -eq 1 `
    -and $scheduleWorldMapRebuild -notmatch `
        'world_map_marker_snapshot_built_\s*=|world_map_umg_markers_\.(?:fill|clear)|world_map_marker_count_\s*=' `
    -and $scheduleWorldMapRebuild -notmatch `
        '(?:publish_runtime_visibility|set_content_visibility_intent|transform_ready_|reconcile_host_visibility)') `
    'A geometry/payload rebuild must be exact-session and once-per-open; it must spend the latch before starting one fresh three-attempt attach budget, while rejection preserves the current renderer visibility and marker snapshot.'

$disable = Get-MainMethod $mainCode 'disable'
Assert-True ($disable -match `
        'preserve_world_map_evidence_on_f8\s*\([\s\S]*?world_map_candidate_available_[\s\S]*?exact_candidate_live[\s\S]*?candidate_in_current_world[\s\S]*?world_map_candidate_has_open_evidence\s*\(\s*\)' `
    -and $disable -match `
        'world_map_umg_renderer_\.suspend\s*\(\s*\)[\s\S]*?reset_world_map_runtime\s*\(\s*preserve_world_map_candidate\s*\)') `
    'F8 disable must synchronously collapse the native-child atlas hosts while preserving exact live session evidence independently of renderer suspension success.'

$transitionBegin = Get-MainMethod $mainCode 'transition_begin'
Assert-True ($transitionBegin -match `
        'compact_umg_renderer_\.detach\s*\(\s*\)[\s\S]*?reset_compact_pool_runtime\s*\(\s*\)[\s\S]*?world_map_umg_renderer_\.detach\s*\(\s*\)[\s\S]*?reset_world_map_runtime\s*\(\s*false\s*\)[\s\S]*?if\s*\(\s*!enabled_\s*\)') `
    'Travel must detach/reset both renderers before the disabled early return can preserve old-world hosts.'

$activationCatchUp = Get-MainMethod `
    $mainCode 'world_map_activation_catch_up_unsafe'
$resetWorldMapRuntime = Get-MainMethod `
    $mainCode 'reset_world_map_runtime'
$rearmWorldMapFromF7 = Get-MainMethod `
    $mainCode 'rearm_world_map_from_f7'
Assert-True ($activationCatchUp -match `
        'refresh_compact_menu_state\s*\([\s\S]*?expected_world[\s\S]*?world_map_activation_catch_up_retained' `
    -and $activationCatchUp -match `
        'world_map_session_pending_\s*=\s*world_map_content_visibility_intent\s*\(\s*\)\s*&&\s*world_map_candidate_has_open_evidence\s*\(\s*\)\s*&&\s*world_map_compact_suppressed_' `
    -and $activationCatchUp -match `
        'capture_world_map_candidate_unsafe\s*\(\s*current_layer\s*,\s*false\s*\)' `
    -and $resetWorldMapRuntime -match `
        'world_map_session_pending_\s*=\s*preserve_candidate\s*&&\s*world_map_candidate_available_\s*&&\s*world_map_candidate_has_open_evidence\s*\(\s*\)\s*&&\s*world_map_compact_suppressed_\s*&&\s*world_map_content_visibility_intent\s*\(\s*\)' `
    -and $resetWorldMapRuntime -match `
        'world_map_readiness_attempts_\s*=\s*0[\s\S]*?world_map_service_attempts_\s*=\s*0' `
    -and $rearmWorldMapFromF7 -match `
        'readiness_budget_exhausted\s*=\s*world_map_readiness_attempts_\s*>=\s*kWorldMapMaxReadinessAttempts' `
    -and $rearmWorldMapFromF7 -match `
        'attach_budget_exhausted\s*=\s*world_map_service_attempts_\s*>=\s*kWorldMapMaxServiceAttempts[\s\S]*?WorldMapUmgRendererState::Ready[\s\S]*?retryable_not_ready\s*\(\s*\)' `
    -and $rearmWorldMapFromF7 -match `
        'world_map_f7_rearm_action\s*\([\s\S]*?world_map_candidate_available_[\s\S]*?exact_candidate_live[\s\S]*?candidate_in_current_world[\s\S]*?world_map_candidate_has_open_evidence\s*\(\s*\)[\s\S]*?visibility_sample[\s\S]*?WorldMapF7RearmAction::Rearm' `
    -and $rearmWorldMapFromF7 -match `
        'enabled_\s*&&\s*!transition_active_[\s\S]*?&&\s*world_map_content_visibility_intent\s*\(\s*\)[\s\S]*?&&\s*f7_rearm_allowed[\s\S]*?&&\s*\(\s*readiness_budget_exhausted\s*\|\|\s*attach_budget_exhausted\s*\)') `
    'F7 catch-up must revalidate retained evidence against native visibility without erasing a fresh SetWorldMapImage edge during the bounded opening grace; reset/rearm may proceed only for that same visible evidence candidate.'

$resumeWorldMap = Get-MainMethod `
    $mainCode 'resume_suspended_world_map_after_f7'
Assert-True ($resumeWorldMap -match `
        'world_map_f7_rearm_action\s*\([\s\S]*?WorldMapF7RearmAction::Rearm[\s\S]*?activity_suppressed_[\s\S]*?\|\|\s*!world_map_content_visibility_intent\s*\(\s*\)[\s\S]*?\|\|\s*!f7_rearm_allowed[\s\S]*?\|\|\s*world_map_umg_renderer_\.active_marker_count\s*\(\s*\)\s*==\s*0' `
    -and $resumeWorldMap -match `
        'if\s*\(\s*resumed\s*\)\s*\{[\s\S]*?world_map_readiness_attempts_\s*=\s*0[\s\S]*?world_map_service_attempts_\s*=\s*0' `
    -and $resumeWorldMap -match `
        'if\s*\(\s*resumed\s*\)\s*\{[\s\S]*?apply_world_map_atlas_visibility_guarded\s*\([\s\S]*?arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::F7Resume\s*,[\s\S]*?WorldMapLayeringArmPolicy::Restart\s*\)') `
    'F7 resume must require enabled content, current candidate evidence, revalidated visible state, and a retained payload before restarting the finite settle tail.'
Assert-True ($serviceWorldMapAtlas -match `
        'apply_world_map_atlas_visibility_guarded\s*\([\s\S]*?arm_world_map_layering_refresh\s*\(\s*Clock::now\s*\(\s*\)\s*,\s*WorldMapLayeringTrigger::Attach\s*,[\s\S]*?WorldMapLayeringArmPolicy::Restart\s*\)' `
    -and $serviceWorldMapAtlas -notmatch `
        'arm_world_map_layering_refresh\s*\(\s*now\s*,\s*WorldMapLayeringTrigger::Attach') `
    'A fresh native-child attachment must start its finite settle tail from a new post-attach clock sample, after publishing initial visibility.'

$zoomTopologyArm = Get-MainMethod $mainCode 'arm_world_map_zoom_topology'
$zoomTopologyService = Get-MainMethod `
    $mainCode 'service_world_map_zoom_topology'
$zoomTopologyPost = Get-MainMethod $mainCode 'world_map_zoom_post_unsafe'
$engineTickUnsafe = Get-MainMethod $mainCode 'engine_tick_unsafe'
$zoomTopologyCapture = Get-RendererFunction `
    $rendererCode 'capture_zoom_topology_unsafe'
$zoomTopologyDebounce = Get-MillisecondConstant `
    $mainCode 'kWorldMapZoomTopologyDebounce'
Assert-True ($zoomTopologyDebounce -eq 1250 `
    -and $zoomTopologyArm -match `
        '!dsnwr::native_event_log_enabled\s*\(\s*\)[\s\S]*?cancel_world_map_zoom_topology\s*\(\s*\)[\s\S]*?return\s*;[\s\S]*?world_map_zoom_topology_pending_\s*=\s*true' `
    -and $zoomTopologyArm -match `
        'world_map_zoom_topology_due_\s*=\s*now\s*\+\s*kWorldMapZoomTopologyDebounce' `
    -and $zoomTopologyArm -match `
        'world_map_zoom_topology_serial_\s*=\s*world_map_candidate_serial_') `
    'Zoom topology diagnostics must be disabled-cost-free and use a serial-bound 1,250 ms trailing-edge deadline.'
Assert-True ($zoomTopologyPost -match `
        'arm_world_map_zoom_topology\s*\(\s*now\s*\)[\s\S]*?arm_world_map_layering_refresh\s*\(') `
    'Every accepted ZoomChanged edge must restart the independent diagnostic deadline without replacing the renderer settle path.'
Assert-True ($zoomTopologyService -match `
        '^\s{4}void\s+service_world_map_zoom_topology[^\{]*\{\s*if\s*\(\s*!world_map_zoom_topology_pending_\s*\)\s*\{\s*return\s*;\s*\}\s*if\s*\(\s*!dsnwr::native_event_log_enabled\s*\(\s*\)\s*\)[\s\S]*?cancel_world_map_zoom_topology\s*\(\s*\)[\s\S]*?return\s*;' `
    -and $zoomTopologyService -match `
        'world_map_zoom_topology_serial_\s*!=\s*world_map_candidate_serial_[\s\S]*?cancel_world_map_zoom_topology\s*\(\s*\)' `
    -and $zoomTopologyService -match `
        'now\s*<\s*world_map_zoom_topology_due_[\s\S]*?return\s*;' `
    -and [regex]::Matches(
        $zoomTopologyService,
        'world_map_umg_renderer_\.capture_zoom_topology\s*\(').Count -eq 1 `
    -and $zoomTopologyService -notmatch `
        '(?:refresh_layering|sync_viewport_transform|arm_world_map_zoom_topology)\s*\(') `
    'Topology service must cancel when diagnostics/session/serial become invalid and perform one read-only capture without refreshing or rearming rendering.'
$topologyServiceIndex = $engineTickUnsafe.IndexOf(
    'service_world_map_zoom_topology(now)')
$layeringServiceIndex = $engineTickUnsafe.IndexOf(
    'service_world_map_layering_refresh(now)')
Assert-True ($topologyServiceIndex -ge 0 `
    -and $layeringServiceIndex -gt $topologyServiceIndex `
    -and $engineTickUnsafe -match `
        'if\s*\(\s*world_map_zoom_topology_pending_\s*\)\s*\{\s*service_world_map_zoom_topology\s*\(\s*now\s*\)\s*;\s*\}') `
    'The diagnostic service must have zero normal-tick call overhead while not pending and must execute before renderer layering refresh when armed.'
Assert-True ($zoomTopologyCapture -match `
        'MapOverlayOutSide[\s\S]*?RetainerBox[\s\S]*?FogUnderPanel[\s\S]*?FogAbovePanel[\s\S]*?TrackingPanel[\s\S]*?SelectedPanel' `
    -and $zoomTopologyCapture -match `
        'UObject\*\s+mod_host\s*=\s*hosts_\s*\[[^]]+\]\.Get\s*\(\s*\)' `
    -and $zoomTopologyCapture -match `
        'capture_chain\s*\(\s*mod_host\s*,\s*snapshot\.mod_host_ancestry\s*\[[^]]+\]\s*\)' `
    -and $zoomTopologyCapture -match `
        'UObject\*\s+mod_root\s*=\s*root_panels_\s*\[[^]]+\]\.Get\s*\(\s*\)' `
    -and $zoomTopologyCapture -match `
        'capture_chain\s*\(\s*mod_root\s*,\s*snapshot\.mod_root_ancestry\s*\[[^]]+\]\s*\)' `
    -and $zoomTopologyCapture -match `
        'UObject\*\s+mod_image\s*=\s*atlas_images_\s*\[[^]]+\]\.Get\s*\(\s*\)' `
    -and $zoomTopologyCapture -match `
        'capture_chain\s*\(\s*mod_image\s*,\s*snapshot\.mod_image_ancestry\s*\[[^]]+\]\s*\)' `
    -and $zoomTopologyCapture -notmatch `
        '(?:set_|add_child|remove_from_parent|request_retainer_render|reconcile_host_visibility|fault_and_detach)\s*\(') `
    'Topology capture must remain a fixed, getter-only observation of named native branches and both Mod atlas layers.'
Assert-True ($zoomTopologyService -match `
        '"mod_host"\s*,\s*layer_index\s*,\s*topology\.mod_host_ancestry\s*\[\s*layer\s*\]' `
    -and $zoomTopologyService -match `
        '"mod_root"\s*,\s*layer_index\s*,\s*topology\.mod_root_ancestry\s*\[\s*layer\s*\]' `
    -and $zoomTopologyService -match `
        '"mod_image"\s*,\s*layer_index\s*,\s*topology\.mod_image_ancestry\s*\[\s*layer\s*\]') `
    'Each Mod host, root, and image layer must emit its own explicit ancestry-chain record.'
Assert-True ($mainCode -notmatch 'renderer_state_unchanged' `
    -and $mainCode -match 'capture_call_read_only=true' `
    -and $mainCode -match 'WORLD_MAP_ZOOM_TOPOLOGY_SUMMARY' `
    -and $mainCode -match 'WORLD_MAP_ZOOM_TOPOLOGY_WIDGET' `
    -and $mainCode -match 'WORLD_MAP_ZOOM_TOPOLOGY_PARENT' `
    -and $mainCode -match 'WORLD_MAP_ZOOM_TOPOLOGY_CHAIN') `
    'Topology diagnostics must use sample-id correlated bounded records and must not claim the renderer state was globally unchanged.'

& (Join-Path $PSScriptRoot 'Verify-HubMapPreview.ps1')
Write-Host 'Native world-map full-stretch-host inner-atlas-layout canary passed.'
