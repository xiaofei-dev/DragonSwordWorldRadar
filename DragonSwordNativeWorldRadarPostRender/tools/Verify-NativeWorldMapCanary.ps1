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
$viewportProjection = Get-ProjectionFunction `
    $projectionCode 'calculate_world_map_viewport_placement'

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
Assert-True ($atlasTextureSize -ge 3072) `
    "World-map atlas texture regressed below 3,072 square; found $atlasTextureSize."
Assert-True ($atlasLayerCount -eq 2) `
    "World-map renderer must retain exactly two atlas layers; found $atlasLayerCount."
Assert-True ($metadata.world_map_umg_renderer.capacity -ge 4096 `
    -and $metadata.world_map_umg_renderer.atlas_texture_size -ge 3072 `
    -and $metadata.world_map_umg_renderer.umg_host_count -eq 2 `
    -and $metadata.world_map_umg_renderer.umg_image_count -eq 2) `
    'Release metadata no longer describes the bounded dual-atlas renderer.'

# Projection is an affine conversion between live Slate geometries. It must not
# infer DPI or aspect ratio from a resolution table.
Assert-True ($projectionCode -match `
        'struct\s+WorldMapSlateGeometry\s*\{[\s\S]*?absolute_left[\s\S]*?absolute_top[\s\S]*?local_width[\s\S]*?local_height[\s\S]*?absolute_scale_x[\s\S]*?absolute_scale_y' `
    -and $projectionCode -match `
        'calculate_world_map_viewport_placement\s*\([\s\S]*?native_geometry\.absolute_left[\s\S]*?native_local_atlas\.left\s*\*\s*native_geometry\.absolute_scale_x[\s\S]*?viewport_geometry\.absolute_left[\s\S]*?viewport_geometry\.absolute_scale_x' `
    -and $projectionCode -match `
        'native_local_atlas\.width\s*\*\s*native_geometry\.absolute_scale_x[\s\S]*?viewport_geometry\.absolute_scale_x' `
    -and $projectionCode -match `
        'kWorldMapViewportTransformTolerance\s*=\s*0\.5' `
    -and $projectionCode -match `
        'world_map_viewport_transform_changed\s*\([\s\S]*?maximum_delta\s*>\s*tolerance') `
    'Pure viewport projection must compose native LocalToAbsolute with viewport AbsoluteToLocal and suppress sub-half-unit jitter.'
Assert-True ($projectionCode -notmatch `
        '\b(?:WorldMapAtlasCanvasLayout|layout_world_map_atlas_canvas|rebase_world_map_atlas_placement)\b') `
    'The rejected native-parent slot ownership or rebase projection helper returned.'
Assert-True ($viewportProjection -notmatch `
        '(?i)(?:resolution|aspect)[_a-z]*\s*(?:==|>=|<=)|\b(?:1920|2560|3440|3840|8000)\b') `
    'Viewport projection must not contain resolution, aspect-ratio, or oversized-container branches.'

# The unit tests pin DPI, aspect ratio, translation, invalid geometry, and the
# transform epsilon independently of any UE object lifecycle.
foreach ($requiredTest in @(
        'calculate_world_map_viewport_placement',
        'for (const double dpi : {1.0, 1.25, 1.5})',
        '4K viewport conversion must apply 100, 125, and 150 percent DPI exactly once',
        '3440.0, 1440.0',
        '2560.0, 1600.0',
        'nonzero viewport origins',
        'independent X and Y Slate scales and negative viewport origins',
        'the exact native-to-viewport affine relation',
        'a common desktop DPI transform must cancel rather than being applied twice',
        'a common absolute desktop translation must not move the viewport-local atlas',
        'an A-B-A live-geometry sequence must return exactly to the original viewport placement without retained drift',
        'reconstruct the witnessed absolute atlas origin',
        'reconstruct the witnessed absolute atlas bottom-right extent',
        'quiet_NaN',
        'numeric_limits<double>::infinity()',
        'zero scale',
        'non-positive atlas or geometry extents, non-positive native or viewport scales, and arithmetic overflow',
        'world_map_viewport_transform_changed',
        'exact_tolerance',
        'above_tolerance',
        'suppress sub-half-unit jitter')) {
    Assert-True ($nativeTests.Contains($requiredTest)) `
        "Native viewport regression matrix is missing: $requiredTest"
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

# The world-map atlas must be a separate viewport host. Native map widgets are
# geometry witnesses only and are never mutated or made owners of our slots.
Assert-True ($rendererCode -match 'UserWidget:AddToViewport' `
    -and $rendererCode -match '\badd_to_viewport_\b') `
    'The independent world-map host is not attached through AddToViewport.'
Assert-True ($rendererCode -match 'WidgetLayoutLibrary:GetViewportWidgetGeometry' `
    -and $rendererCode -match 'SlateBlueprintLibrary:LocalToAbsolute' `
    -and $rendererCode -match 'SlateBlueprintLibrary:AbsoluteToLocal' `
    -and $rendererCode -match 'Widget:SetAlignmentInViewport' `
    -and $rendererCode -match 'Widget:SetPositionInViewport' `
    -and $rendererCode -match 'Widget:SetDesiredSizeInViewport' `
    -and $rendererCode -match 'calculate_world_map_viewport_placement') `
    'The renderer must derive viewport placement from live Slate geometry and apply it through viewport widget APIs.'

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
Assert-True ($rendererCode -notmatch `
        '(?:native_parent|witnessed_parent)->ProcessEvent\s*\(\s*add_child_to_canvas_' `
    -and $rendererCode -notmatch `
        '(?:native_parent|witnessed_parent)->ProcessEvent\s*\(\s*(?:set_slot_|remove_from_parent_)' `
    -and $rendererCode -notmatch '\bnative_parent_slots_\b' `
    -and $rendererCode -notmatch `
        '\b(?:retainer|retainer_box)->ProcessEvent\s*\(' `
    -and $rendererCode -notmatch '\brequest_retainer_render_?\b' `
    -and $rendererCode -notmatch '\bForceLayoutPrepass\b') `
    'The renderer must never mutate the native map parent or RetainerBox, retain native-parent slots, or force a native layout prepass.'
Assert-True ($rendererCode -notmatch `
        '\b(?:reproject_atlas_unsafe|rebase_world_map_atlas_placement|layout_world_map_atlas_canvas)\b' `
    -and $rendererCode -notmatch `
        'WorldMapLayeringRefreshResult::(?:Reparented|Reprojected)') `
    'The old native-parent reparent/reproject lifecycle returned.'

# One AddChildToCanvas call is still required to build each Mod-owned
# host->root-panel->atlas-image graph. It must be the single looped call on our
# root panel; any second call would necessarily reintroduce native Canvas
# ownership or later reparenting.
$addChildToCanvasCalls = [regex]::Matches(
    $rendererCode, 'ProcessEvent\s*\(\s*add_child_to_canvas_')
Assert-True ($addChildToCanvasCalls.Count -eq 1 `
    -and $rendererCode -match `
        'root_panels\s*\[\s*layer_index\s*\]\s*->ProcessEvent\s*\(\s*add_child_to_canvas_') `
    'AddChildToCanvas must occur exactly once in source, inside the bounded loop for each Mod-owned root panel.'

$detachUnsafe = Get-RendererFunction $rendererCode 'detach_unsafe'
Assert-True ([regex]::Matches(
        $rendererCode,
        'ProcessEvent\s*\(\s*remove_from_parent_').Count -eq 1 `
    -and $detachUnsafe -match `
        'host->ProcessEvent\s*\(\s*remove_from_parent_') `
    'RemoveFromParent must exist only in detach and may target only the Mod-owned viewport hosts.'

$syncViewportTransform = Get-RendererFunction `
    $rendererCode 'sync_viewport_transform_unsafe'
$nativeIconTemplateLookup = Get-FreeRendererFunction `
    $rendererCode 'NativeIconTemplateLookupResult' 'find_native_icon_template'
Assert-True ($syncViewportTransform -match `
        'calculate_world_map_viewport_placement' `
    -and $syncViewportTransform -match `
        'world_map_viewport_transform_changed' `
    -and $syncViewportTransform -match `
        'set_position_in_viewport_' `
    -and $syncViewportTransform -match `
        'set_desired_size_in_viewport_') `
    'Viewport transform sync must use the pure conversion and epsilon gate before updating position and size.'
Assert-True ($syncViewportTransform -match `
        'remove_dpi_scale\s*=\s*false') `
    'SetPositionInViewport must explicitly keep RemoveDPIScale=false because the live Slate conversion already applies DPI exactly once.'
Assert-True ($syncViewportTransform -match `
        'native_parent_\.Get\(\)' `
    -and $syncViewportTransform -notmatch `
        'find_native_icon_template' `
    -and $syncViewportTransform -notmatch `
        'ArrayIconInfo' `
    -and $syncViewportTransform -match `
        'classify_world_map_transform_observation_failure' `
    -and $syncViewportTransform -match `
        'WorldMapLayeringRefreshResult::Retained' `
    -and $syncViewportTransform -notmatch `
        'viewport_transform_valid_\s*=\s*false') `
    'Transform sync must keep the attachment-verified native Canvas as its coordinate witness and retain the same-layer last verified transform across transient observation gaps.'
$syncViewportTransformGuarded = Get-RendererFunction `
    $rendererCode 'sync_viewport_transform'
Assert-True ($syncViewportTransformGuarded -match `
        'world_map_transform_failure_for_stage' `
    -and $syncViewportTransformGuarded -match `
        'classify_world_map_transform_observation_failure' `
    -and $syncViewportTransformGuarded -match `
        'world_map_transform_visibility_policy' `
    -and $syncViewportTransformGuarded -match `
        'WorldMapLayeringRefreshResult::Retained' `
    -and $syncViewportTransformGuarded -match `
        'WorldMapLayeringRefreshResult::RetryLater' `
    -and $syncViewportTransformGuarded -match `
        'fault_and_detach') `
    'The guarded sync wrapper must classify the exact failed stage, reconcile transient visibility, and use one fail-closed detach path only for hard failures.'
Assert-True ($syncViewportTransform -match `
        'world_map_transform_visibility_policy' `
    -and $syncViewportTransform -match `
        'transform_ready_\s*=\s*visibility_policy\.transform_ready' `
    -and $syncViewportTransform -match `
        'reconcile_host_visibility_unsafe\s*\(' `
    -and $syncViewportTransform -match `
        'transform_ready_\s*=\s*true[\s\S]*?reconcile_host_visibility_unsafe\s*\(\s*false' `
    -and $syncViewportTransform -notmatch `
        '\bset_visibility\s*\(' `
    -and $syncViewportTransform -notmatch `
        '\b(?:content_visibility_intent_|runtime_visibility_allowed_)\s*=') `
    'Transform sync may publish readiness and reconcile hosts, but it must not overwrite either upstream visibility intent or write hosts directly.'
$layerMismatchGuardIndex = $syncViewportTransform.IndexOf(
    'if (!current_layer || current_layer != retained_layer)')
$ownedPayloadValidationIndex = $syncViewportTransform.IndexOf(
    'validate_host_payload_unsafe')
Assert-True ($layerMismatchGuardIndex -ge 0 `
    -and $ownedPayloadValidationIndex -gt $layerMismatchGuardIndex) `
    'A missing or replacement game layer must enter hidden bounded retry before exact-layer owned-payload validation.'
Assert-True ($nativeIconTemplateLookup -match `
        '(?s)count\s*<\s*0.*?InvalidSchema' `
    -and $nativeIconTemplateLookup -match `
        '(?s)count\s*==\s*0.*?NotReady' `
    -and $nativeIconTemplateLookup -match `
        'GetPropertyByNameInChain\(L"Panel_Point"\)' `
    -and $nativeIconTemplateLookup -match `
        'GetPropertyByNameInChain\(L"Slot"\)' `
    -and $nativeIconTemplateLookup -match `
        'GetPropertyByNameInChain\(L"Parent"\)' `
    -and $nativeIconTemplateLookup -match `
        'GetPropertyByNameInChain\(L"Content"\)' `
    -and $nativeIconTemplateLookup -notmatch `
        'read_object_property\(\s*(?:icon|slot)') `
    'Native icon lookup must distinguish an empty/not-ready list from corrupt counts and missing or type-invalid nested widget ABI fields.'
$positionParametersMatch = [regex]::Match(
    $syncViewportTransform,
    'PositionInViewportParameters\s+(?<name>[A-Za-z_]\w*)')
Assert-True $positionParametersMatch.Success `
    'Viewport transform sync must use a named PositionInViewportParameters value.'
$positionParametersName = [regex]::Escape(
    $positionParametersMatch.Groups['name'].Value)
Assert-True ($syncViewportTransform -match `
        "(?s)\b$positionParametersName\.remove_dpi_scale\s*=\s*false[\s\S]*?ProcessEvent\s*\(\s*set_position_in_viewport_\s*,\s*&$positionParametersName\s*\)") `
    'RemoveDPIScale=false must be set on the exact parameter object passed to SetPositionInViewport.'
foreach ($forbiddenSyncWork in @(
        'build_rle_tga_atlas', 'ImportFileAsTexture2D', 'NewObject',
        'CreateWidget', 'AddToViewport', 'AddChildToCanvas',
        'RemoveFromParent', 'ClearChildren', 'ForceLayoutPrepass',
        'request_retainer_render', 'StaticFindObject', 'FindAllOf',
        'FindFirstOf', 'filesystem', 'fstream', 'ofstream')) {
    Assert-True ($syncViewportTransform -notmatch `
            [regex]::Escape($forbiddenSyncWork)) `
        "Viewport transform sync contains forbidden rebuild, tree mutation, discovery, or file work: $forbiddenSyncWork"
}
Assert-True ($syncViewportTransform -notmatch `
        '(?i)\b(?:GetViewportSize|GetViewportScale|resolution|aspect(?:ratio)?)\b' `
    -and $syncViewportTransform -notmatch `
        '\b(?:1920|2160|2560|3440|3840|8000)\b') `
    'Viewport transform sync must consume live Slate geometry directly; resolution tables, aspect-ratio branches, and viewport-scale compensation are forbidden.'

$attach = Get-RendererFunction $rendererCode 'attach_unsafe'
Assert-True ([regex]::Matches(
        $attach, 'ProcessEvent\s*\(\s*add_to_viewport_').Count -eq 1 `
    -and $attach -match 'layer_index\s*<\s*kWorldMapAtlasLayerCount' `
    -and $attach -match 'kHitTestInvisible') `
    'Initial attachment must add every hit-test-invisible atlas host to the viewport exactly once through the bounded layer loop.'
Assert-True ($attach -match `
        'set_alignment_in_viewport_' `
    -and ($attach -match `
            '(?s)alignment[^;=]*\{\s*\{\s*0(?:\.0+)?\s*,\s*0(?:\.0+)?\s*\}\s*\}' `
        -or $attach -match `
            '(?s)alignment[^;=]*\{\s*\}')) `
    'Each viewport host must use an explicit zero alignment so live placement remains a top-left affine transform.'
Assert-True ($attach -notmatch `
        '(?:native_parent|witnessed_parent)->ProcessEvent\s*\(\s*add_child_to_canvas_') `
    'Initial attachment must not insert either atlas host into the game-owned native Canvas.'
Assert-True ($rendererCode -notmatch `
        'read_object_property\s*\(\s*(?:native_parent|witnessed_parent)\s*,\s*L"Slot"' `
    -and $rendererCode -notmatch `
        '(?:native_parent|witnessed_parent|retainer|retainer_box)->ProcessEvent\s*\(\s*(?:clear_children_|set_slot_|remove_from_parent_|request_retainer_render_)') `
    'The game-owned map parent and RetainerBox must remain read-only geometry witnesses, including outside the named legacy paths.'

# CanvasPanelSlot setters are permitted only while constructing the Mod-owned
# root-panel -> atlas-image graph. Keeping them out of every other renderer
# method prevents an alias from hiding a write back into the native map tree.
$rendererOutsideAttach = $rendererCode.Replace($attach, '')
Assert-True ($rendererOutsideAttach -notmatch `
        'ProcessEvent\s*\(\s*add_to_viewport_') `
    'AddToViewport escaped initial attachment; retained hosts must never be re-added during geometry settling, suspend, or resume.'
foreach ($slotSetter in @(
        'set_slot_position_', 'set_slot_size_',
        'set_slot_alignment_', 'set_slot_z_order_')) {
    Assert-True ($rendererOutsideAttach -notmatch `
            "ProcessEvent\s*\(\s*$([regex]::Escape($slotSetter))") `
        "Canvas slot mutation escaped initial Mod-owned construction: $slotSetter"
    Assert-True ($rendererOutsideAttach -notmatch `
            "set_slot_vector\s*\([^;]*?$([regex]::Escape($slotSetter))") `
        "Canvas slot vector mutation escaped initial Mod-owned construction: $slotSetter"
}

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
$visibilityCacheResetIndex = $applyHostVisibility.LastIndexOf(
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
        'for\s*\(\s*UObject\s*\*\s*host\s*:\s*hosts\s*\)') `
    'A repeated target must be a no-UObject no-op; a changed target must invalidate the cache before writing both hosts and publish only after both succeed.'

$setVisibilityHelper = Get-FreeRendererFunction `
    $rendererCode 'void' 'set_visibility'
$rendererWithoutVisibilityOwner = $rendererCode.Replace(
    $setVisibilityHelper, '').Replace($applyHostVisibility, '')
Assert-True ([regex]::Matches(
        $rendererWithoutVisibilityOwner,
        '\bset_visibility\s*\(').Count -eq 4 `
    -and [regex]::Matches(
        $attach,
        '\bset_visibility\s*\(').Count -eq 4 `
    -and $rendererWithoutVisibilityOwner.Replace(
        $attach, '') -notmatch '\bset_visibility\s*\(' `
    -and $rendererWithoutVisibilityOwner -match `
        'set_visibility\s*\(\s*root_panels\s*\[\s*layer_index\s*\]' `
    -and $rendererWithoutVisibilityOwner -match `
        'set_visibility\s*\(\s*atlas_images\s*\[\s*layer_index\s*\]' `
    -and [regex]::Matches(
        $rendererWithoutVisibilityOwner,
        'set_visibility\s*\(\s*hosts\s*\[\s*layer_index\s*\]\s*,\s*set_visibility_\s*,\s*kCollapsed\s*\)').Count -eq 2) `
    'Runtime viewport-host SetVisibility writes must be confined to the idempotent two-host apply helper; direct root, image, and host collapse writes may occur only inside initial construction.'
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
        'state_\s*=\s*WorldMapUmgRendererState::Attached[\s\S]*?sync_viewport_transform_unsafe' `
    -and $attach -match `
        'if\s*\(\s*!transform_synced\s*\|\|\s*sync_result\s*==\s*WorldMapLayeringRefreshResult::Faulted\s*\)' `
    -and $attach -notmatch `
        '\bcontent_visibility_intent_\s*=' `
    -and $attach -notmatch `
        'sync_result\s*==\s*WorldMapLayeringRefreshResult::RetryLater[\s\S]*?(?:detach|return\s+false)') `
    'Fresh attachment must reset transient visibility state, publish Attached ownership before sync, and retain its hosts when first transform proof returns RetryLater.'

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
        'state_\s*=\s*WorldMapUmgRendererState::Attached[\s\S]*?sync_viewport_transform_unsafe' `
    -and $resumeRenderer -match `
        'if\s*\(\s*!transform_synced\s*\|\|\s*result\s*==\s*WorldMapLayeringRefreshResult::Faulted\s*\)' `
    -and $resumeRenderer -notmatch '\bset_visibility\s*\(' `
    -and $resumeRenderer -notmatch `
        '\bcontent_visibility_intent_\s*=') `
    'F7 resume must treat RetryLater as retained Attached lifecycle success and leave visibility to the shared gates.'
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

$validateHost = Get-RendererFunction $rendererCode 'validate_host_unsafe'
Assert-True ($validateHost -notmatch `
        'read_object_property\s*\(\s*host\s*,\s*L"Slot"' `
    -and $validateHost -notmatch '\bnative_parent_slots_\b' `
    -and $validateHost -notmatch '\bretainer_box_\b' `
    -and $validateHost -notmatch '\bnative_parent_\b' `
    -and $validateHost -notmatch `
        'current_layer_witnesses_native_parent' `
    -and $validateHost -match `
        'read_object_property\s*\(\s*host\s*,\s*L"WidgetTree"' `
    -and $validateHost -match `
        'read_struct_object_property\s*\([\s\S]*?L"Brush"\s*,\s*L"ResourceObject"') `
    'Host validation must prove only the owned widget tree and atlas payload, never transient native-parent or RetainerBox identity.'
Assert-True ($rendererCode -notmatch `
        'current_layer_witnesses_native_parent') `
    'Transient native-Canvas witness identity must not be treated as Mod-owned host integrity.'

Assert-True ([regex]::Matches(
        $rendererCode, '\bbuild_rle_tga_atlas\s*\(').Count -le 2 `
    -and [regex]::Matches(
        $rendererCode, '\bImportFileAsTexture2D\b').Count -le 2) `
    'Atlas construction/import must remain confined to definition plus initial attachment and never service transform changes.'

# Main owns the visible-session state machine. Retained viewport hosts must be
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

# Transform reconciliation is event-triggered and finite. Keep the known
# post-open settle tail, but never turn it into a steady poll or process more
# than one reflected geometry sample in one game-thread pass.
Assert-True ($maxWorldMapServiceAttempts -eq 3 `
    -and $maxWorldMapReadinessAttempts -eq 40 `
    -and $worldMapServiceRetryDelayMs -eq 150 `
    -and $worldMapOpenVisibilityGraceMs -eq 1000 `
    -and $mainCode -match `
        'kWorldMapLayeringSettleDelays\s*\{[\s\S]*?milliseconds\s*\{\s*100\s*\}[\s\S]*?milliseconds\s*\{\s*250\s*\}[\s\S]*?milliseconds\s*\{\s*500\s*\}[\s\S]*?milliseconds\s*\{\s*1000\s*\}[\s\S]*?milliseconds\s*\{\s*1250\s*\}\s*\}\s*;') `
    'The bounded attach retry and finite 100/250/500/1000/1250 ms transform-settle schedule changed.'

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
    'Compact-map suppression must not change independent world-map host visibility.'

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
        'if\s*\(\s*attached_layer_transform_sync\s*\)\s*\{[\s\S]*?arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::SetWorldMapImage\s*,[\s\S]*?WorldMapLayeringArmPolicy::Coalesce\s*\)') `
    'A map-open event may collapse hosts only for a new or mismatched layer; an exact attached layer must enter the transform-only settle path without hiding.'
$sameLayerOpenSession = [regex]::Match(
    $captureWorldMapCandidate,
    '(?ms)const\s+bool\s+new_open_edge\s*=\s*dswros::world_map_open_session_started\s*\(\s*candidate_transition\s*\)\s*;\s*world_map_open_serial_\s*=\s*world_map_candidate_serial_\s*;\s*if\s*\(\s*new_open_edge\s*\)\s*\{(?<body>.*?)\}\s*if\s*\(\s*content_visible\s*&&\s*!exact_attached_layer\s*\)')
Assert-True ($sameLayerOpenSession.Success `
    -and $sameLayerOpenSession.Groups['body'].Value -match `
        '^\s*world_map_visible_serial_\s*=\s*0\s*;\s*world_map_open_evidence_at_\s*=\s*Clock::now\s*\(\s*\)\s*;\s*$') `
    'A same-layer SetWorldMapImage edge must start visibility grace exactly once; duplicate open events must not reset confirmed visibility or the grace clock.'
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
    -and $listenerCandidateCapture -notmatch `
        'world_map_(?:open_serial_|session_pending_|readiness_attempts_|service_attempts_)|latch_world_map_compact_suppression|service_world_map_atlas' `
    -and $worldMapImagePost -match `
        'capture_world_map_candidate_unsafe\s*\(\s*context\.Context\s*,\s*true\s*\)' `
    -and $worldMapImagePost -notmatch '\benabled_\b' `
    -and $sessionPolicyCode -match `
        'next_world_map_candidate_serial[\s\S]*?numeric_limits<std::uint64_t>::max\s*\(\s*\)[\s\S]*?current_serial\s*\+\s*1U') `
    'Passive discovery may publish identity only; candidate replacement must invalidate inherited evidence, while SetWorldMapImage remains recordable during F8-disabled state.'
Assert-True ([regex]::Matches(
        $mainCode,
        '\bworld_map_open_serial_\s*=(?!=)').Count -eq 4) `
    'Only same-candidate SetWorldMapImage, new-candidate publication, authoritative close, and full candidate reset may write map-open evidence.'

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
        'sync_viewport_transform\s*\([\s\S]*?apply_world_map_atlas_visibility_guarded\s*\(' `
    -and $sameLayerReuseBody -notmatch `
        '(?:publish_runtime_visibility|set_content_visibility_intent)\s*\(\s*false\s*\)' `
    -and [regex]::Matches(
        $sameLayerReuseBody,
        'arm_world_map_layering_refresh\s*\([\s\S]*?WorldMapLayeringTrigger::SetWorldMapImage\s*,[\s\S]*?WorldMapLayeringArmPolicy::Coalesce\s*\)').Count -eq 2) `
    'The same attached layer must synchronize its live transform before reapplying gated visibility and must never hide first.'

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
Assert-True ($layeringService -match `
        'WorldMapLayeringRefreshResult::Updated[\s\S]*?WorldMapLayeringRefreshResult::Unchanged[\s\S]*?WorldMapLayeringRefreshResult::Retained[\s\S]*?apply_world_map_atlas_visibility_guarded' `
    -and $layeringService -match `
        'if\s*\(\s*current_layer\s*\)\s*\{[\s\S]*?sync_viewport_transform\s*\([\s\S]*?\}\s*else\s*\{[\s\S]*?world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)' `
    -and [regex]::Matches(
        $layeringService,
        'world_map_umg_renderer_\.publish_runtime_visibility\s*\(\s*false\s*\)').Count -eq 1 `
    -and $layeringService -notmatch `
        'set_content_visibility_intent\s*\(') `
    'Successful or retained transforms may reapply the runtime gate; only a missing layer is collapsed by main, while renderer-owned RetryLater preserves the host.'
Assert-True ([regex]::Matches(
        $layeringService, '\bsync_viewport_transform\s*\(').Count -eq 1) `
    'One settle-service pass must perform exactly one reflected viewport-transform observation.'
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
    'The settle service must not re-arm itself, catch up overdue observations, discover objects, touch files, or mutate widget trees.'

$disable = Get-MainMethod $mainCode 'disable'
Assert-True ($disable -match `
        'preserve_world_map_evidence_on_f8\s*\([\s\S]*?world_map_candidate_available_[\s\S]*?exact_candidate_live[\s\S]*?candidate_in_current_world[\s\S]*?world_map_candidate_has_open_evidence\s*\(\s*\)' `
    -and $disable -match `
        'world_map_umg_renderer_\.suspend\s*\(\s*\)[\s\S]*?reset_world_map_runtime\s*\(\s*preserve_world_map_candidate\s*\)') `
    'F8 disable must synchronously collapse the independent atlas hosts while preserving exact live session evidence independently of renderer suspension success.'

$transitionBegin = Get-MainMethod $mainCode 'transition_begin'
Assert-True ($transitionBegin -match `
        'compact_umg_renderer_\.detach\s*\(\s*\)[\s\S]*?reset_compact_pool_runtime\s*\(\s*\)[\s\S]*?world_map_umg_renderer_\.detach\s*\(\s*\)[\s\S]*?reset_world_map_runtime\s*\(\s*false\s*\)[\s\S]*?if\s*\(\s*!enabled_\s*\)') `
    'Travel must detach/reset both viewport renderers before the disabled early return can preserve old-world hosts.'

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
        'WorldMapLayeringTrigger::Attach\s*,[\s\S]*?WorldMapLayeringArmPolicy::Restart\s*\)') `
    'A fresh viewport attachment is a lifecycle edge and must explicitly restart the finite settle tail.'

Write-Host 'Native world-map independent-viewport canary passed.'
