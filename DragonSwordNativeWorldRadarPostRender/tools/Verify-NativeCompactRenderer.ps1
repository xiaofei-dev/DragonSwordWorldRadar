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

function Get-MainFunction {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?(?:static\s+)?(?:bool|void|std::size_t|UObject\*)\s+' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "Native owner function was not found: $Name"
    return $match.Value
}

function Get-SharpHeadTipCorner {
    param(
        [double]$BaseY,
        [bool]$Upper,
        [double]$Thickness = 7.0
    )

    $tipX = 0.0
    $tipY = 0.0
    $baseX = -9.0
    $relativeBaseX = $baseX - $tipX
    $relativeBaseY = if ($Upper) { $BaseY - $tipY } else { -($BaseY - $tipY) }
    $distanceSquared = $relativeBaseX * $relativeBaseX + $relativeBaseY * $relativeBaseY
    $radius = $Thickness * 0.5
    $tangentScale = $radius * [Math]::Sqrt($distanceSquared - $radius * $radius) / $distanceSquared
    $radialScale = $radius * $radius / $distanceSquared
    $startX = $tipX + $radialScale * $relativeBaseX - $tangentScale * $relativeBaseY
    $upperStartY = $tipY + $radialScale * $relativeBaseY + $tangentScale * $relativeBaseX
    $startY = if ($Upper) { $upperStartY } else { 2.0 * $tipY - $upperStartY }
    $dx = $baseX - $startX
    $dy = $BaseY - $startY
    $length = [Math]::Sqrt($dx * $dx + $dy * $dy)
    $normalX = -$dy / $length * $radius
    $normalY = $dx / $length * $radius
    $cornerAX = $startX + $normalX
    $cornerAY = $startY + $normalY
    $cornerBX = $startX - $normalX
    $cornerBY = $startY - $normalY
    $distanceA = $cornerAX * $cornerAX + $cornerAY * $cornerAY
    $distanceB = $cornerBX * $cornerBX + $cornerBY * $cornerBY
    if ($distanceA -le $distanceB) {
        return [pscustomobject]@{ X = $cornerAX; Y = $cornerAY }
    }
    return [pscustomobject]@{ X = $cornerBX; Y = $cornerBY }
}

$main = Get-Content (Join-Path $projectRoot 'src\native\main.cpp') -Raw
$renderer = Get-Content (Join-Path $projectRoot 'src\native\compact_umg_renderer.cpp') -Raw
$rendererHeader = Get-Content (Join-Path $projectRoot 'src\native\compact_umg_renderer.hpp') -Raw
$visibilityHub = Get-Content `
    (Join-Path $projectRoot 'src\native\radar_visibility_hub.cpp') -Raw
$visibilityHubHeader = Get-Content `
    (Join-Path $projectRoot 'src\native\radar_visibility_hub.hpp') -Raw
$visibilityConfig = Get-Content `
    (Join-Path $projectRoot 'config\visibility.ini') -Raw
$visibilityParser = Get-Content `
    (Join-Path $projectRoot 'include\dswros\visibility_config.hpp') -Raw
$saveReconciler = Get-Content (Join-Path $projectRoot 'src\native\native_save_reconciler.cpp') -Raw
$saveReconcilerHeader = Get-Content (Join-Path $projectRoot 'src\native\native_save_reconciler.hpp') -Raw
$areaQuestVisibility = Get-Content (Join-Path $projectRoot 'include\dswros\area_quest_visibility.hpp') -Raw
$model = Get-Content (Join-Path $projectRoot 'include\dswros\compact_render_model.hpp') -Raw
$objectState = Get-Content (Join-Path $projectRoot 'include\dswros\object_state.hpp') -Raw
$renderProjection = Get-Content `
    (Join-Path $projectRoot 'include\dswros\render_projection.hpp') -Raw
$nativeTests = Get-Content (Join-Path $projectRoot 'tests\native_state_tests.cpp') -Raw
$cmake = Get-Content (Join-Path $projectRoot 'CMakeLists.txt') -Raw
$deploy = Get-Content (Join-Path $projectRoot 'tools\Deploy-NativePrototype.ps1') -Raw
$treasureOverrides = Get-Content `
    (Join-Path $projectRoot 'src\data\defaults\treasure_overrides.txt') -Raw
$treasureRenderCatalog = Get-Content `
    (Join-Path $projectRoot 'src\data\generated\treasures.lua') -Raw
$treasureActorCatalogPath = Join-Path $projectRoot `
    'src\data\generated\treasure-actors.tsv'
$treasureActorRows = @(Get-Content $treasureActorCatalogPath | Select-Object -Skip 1)
$metadata = Get-Content (Join-Path $projectRoot 'metadata\release.json') -Raw |
    ConvertFrom-Json
$mainCode = Remove-CppComments $main

$activeTreasureOverrideRows = @($treasureOverrides -split "`r?`n" |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -and -not $_.StartsWith('#') })
Assert-True ($activeTreasureOverrideRows.Count -eq 1 `
    -and $activeTreasureOverrideRows[0] -eq 'ignore 11230106') `
    'The release must ignore exactly the one confirmed absent treasure ID 11230106.'
$confirmedAbsentRenderPattern =
    '(?m)^\s*\{\s*save_id\s*=\s*11230106,\s*section\s*=\s*"2142120000100",\s*' +
    'x\s*=\s*182813,\s*y\s*=\s*162051,\s*z\s*=\s*3150,\s*' +
    'uid_name\s*=\s*"DT_Unlock_G3_11206",\s*group_id\s*=\s*0\s*\},\s*$'
Assert-True ([regex]::Matches(
        $treasureRenderCatalog, $confirmedAbsentRenderPattern).Count -eq 1) `
    'The confirmed absent treasure render identity or coordinates changed.'
$renderIdMatches = [regex]::Matches(
    $treasureRenderCatalog, '\bsave_id\s*=\s*(\d+),')
$renderIds = @($renderIdMatches | ForEach-Object {
        [int64]$_.Groups[1].Value
    })
$actorIds = @($treasureActorRows | ForEach-Object {
        [int64](($_ -split "`t", 2)[0])
    })
$actorIdSet = [System.Collections.Generic.HashSet[int64]]::new()
$renderIdSet = [System.Collections.Generic.HashSet[int64]]::new()
foreach ($id in $actorIds) { [void]$actorIdSet.Add($id) }
foreach ($id in $renderIds) { [void]$renderIdSet.Add($id) }
$renderOnlyIds = @($renderIds | Where-Object { -not $actorIdSet.Contains($_) })
$actorOnlyIds = @($actorIds | Where-Object { -not $renderIdSet.Contains($_) })
Assert-True ($renderIds.Count -eq 1693 `
    -and $renderIdSet.Count -eq 1693 `
    -and $actorIds.Count -eq 1692 `
    -and $actorIdSet.Count -eq 1692 `
    -and $renderOnlyIds.Count -eq 1 `
    -and $renderOnlyIds[0] -eq 11230106 `
    -and $actorOnlyIds.Count -eq 0) `
    'Treasure render/Actor catalog difference must remain exactly the confirmed absent ID 11230106.'

Assert-True ($metadata.version -eq '2.1.0' `
    -and $mainCode -match `
        'kVersion\s*=\s*STR\("2\.1\.0"\)' `
    -and $mainCode -match `
        'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0') `
    'Release metadata version does not match the native compact-pool milestone.'
Assert-True ($main -match '#include "compact_umg_renderer\.hpp"') `
    'The native owner does not include the compact UMG renderer.'
Assert-True ($main -match 'CompactUmgRenderer') `
    'The native owner does not own the compact UMG renderer.'
Assert-True ($main -notmatch 'UmgMiniMapCanary') `
    'The temporary dev14 visual witness is still active.'
Assert-True ($main -match 'kCompactRebindDistance\s*=\s*1000\.0') `
    'Compact rebinding must not regress to the dev12 near-frame movement threshold.'
Assert-True ($main -match 'F7_COALESCED') `
    'Repeated F7 input must remain idempotent while the native radar is active.'
Assert-True ($main -match 'SaveId\\tClassName\\tX\\tY\\tZ') `
    'The native owner does not validate the treasure actor SaveId header.'
Assert-True ($cmake -match 'src/native/compact_umg_renderer\.cpp') `
    'The compact UMG renderer is missing from the native target.'
Assert-True ($cmake -notmatch 'src/native/umg_minimap_canary\.cpp') `
    'The temporary dev14 visual witness is still compiled.'
Assert-True ($cmake -notmatch 'src/native/late_present_canary\.cpp') `
    'The runtime-rejected late Present path must remain uncompiled.'

foreach ($required in @(
        '/Script/UMG\.CanvasPanel',
        '/Script/UMG\.Border',
        'UMG\.Border:SetBrushColor',
        'UMG\.Widget:SetRenderTranslation',
        'UMG\.Widget:SetRenderTransformAngle',
        'UMG\.Widget:SetRenderTransformPivot',
        'CanvasPanel:AddChildToCanvas',
        'PanelWidget:ClearChildren',
        'WidgetLayoutLibrary:GetViewportScale',
        'UserWidget:SetPositionInViewport',
        'UserWidget:SetDesiredSizeInViewport',
        'Widget:RemoveFromParent')) {
    Assert-True ($renderer -match $required) `
        "Compact UMG renderer is missing required reflected API: $required"
}
Assert-True ($renderer -match 'player_icon->GetClassPrivate\(\)') `
    'The production pool must use the runtime-proven player-icon Blueprint host.'
Assert-True ($renderer -match 'read_object_property\(host, L"WidgetTree"\)') `
    'The production pool must retain the Blueprint-created WidgetTree.'
Assert-True ($renderer -notmatch 'NewObject<UObject>\(host, widget_tree_class_\)') `
    'The runtime-invisible private WidgetTree replacement must not return.'
Assert-True ($renderer -notmatch 'SetObjectPropertyValue') `
    'The native renderer must not replace WidgetTree or RootWidget properties.'
Assert-True ($renderer -match 'NewObject<UObject>\(tree, border_class_\)') `
    'Marker chest pieces must be owned by the Blueprint-created WidgetTree.'
Assert-True ($renderer -match 'read_object_property\(tree, L"RootWidget"\)') `
    'Marker attachment must use the Blueprint-created root widget.'
Assert-True ($renderer -match 'marker_canvas->IsA\(canvas_panel_class_\)') `
    'The Blueprint root must fail closed unless it is a CanvasPanel.'
Assert-True ($renderer -match 'movement_group->ProcessEvent\(add_child_to_canvas_, &add_piece\)') `
    'Foreground marker pieces must attach to the foreground moving Canvas.'
Assert-True ($renderer -notmatch 'add_marker_panel') `
    'The runtime-invisible dev12 nested marker canvas must not return.'
Assert-True ($renderer -match 'marker_piece_style\(marker\.kind, piece\)') `
    'Marker pieces must use the fixed bounded glyph-style table.'
Assert-True ($renderer -match 'kTreasureOther' -and $renderer -match 'kTreasureMiniGame' `
    -and $renderer -match 'kTreasureMap' -and $renderer -match 'kTreasurePuzzle') `
    'The stable white, green, orange, and blue treasure palette is incomplete.'
Assert-True ($renderer -notmatch 'UMG\.Image:SetBrush') `
    'The chest pool must not return to copied-arrow marker art.'
Assert-True ($renderer -notmatch 'nearest_arrow|source_arrow|copy_property') `
    'The temporary dev20 parity arrow and second viewport host must be absent.'
Assert-True ($renderer -match `
        "kViewportZOrder\s*=\s*2'000'000'000") `
    'The compact host must remain above ordinary game task indicators.'
Assert-True ($renderer -match `
        'AddToViewportParameters\s+add_to_viewport\s*\{\s*kViewportZOrder\s*\}') `
    'The audited high Z order is not used by the real AddToViewport call.'
Assert-True ($renderer -match `
        'case 0:\s*return\s*\{1\.10,\s*0\.88' `
    -and $renderer -match 'case 1:\s*return\s*\{0\.82,\s*0\.46' `
    -and $renderer -match 'case 2:\s*return\s*\{0\.86,\s*0\.22' `
    -and $renderer -match 'default:\s*return\s*\{0\.18,\s*0\.30') `
    'Treasure glyphs no longer match the accepted dev23 silhouette.'
Assert-True ($renderer -notmatch 'treasure_body_color|kTreasure\w+Body') `
    'The rejected dev24 two-tone treasure treatment returned.'
foreach ($requiredGeometry in @(
        'kReferenceTreasureHalfWidth\s*=\s*0\.55',
        'kReferenceHeightLength\s*=\s*26\.0',
        'kReferenceHeightClearance\s*=\s*2\.5',
        'kReferenceHeightHeadLength\s*=\s*10\.0',
        'kReferenceHeightHeadHalfWidth\s*=\s*7\.5',
        'kReferenceHeightOutlineWidth\s*=\s*7\.5',
        'kReferenceHeightInnerWidth\s*=\s*3\.25',
        'kReferenceHeightEndCapInset\s*=\s*2\.0',
        'kReferenceHeightTailInset\s*=\s*3\.0')) {
    Assert-True ($renderer -match $requiredGeometry) `
        "The nearest-height pointer geometry regressed: $requiredGeometry"
}
Assert-True ([regex]::Matches(
        $renderer,
        'NewObject<UObject>\(tree, canvas_panel_class_\)').Count -eq 3) `
    'The renderer must retain one source-bounded movement, dual-height loop, and fixed clock Canvas allocation path.'
Assert-True ($renderer -match 'marker_canvas->ProcessEvent\(add_child_to_canvas_, &add_clock_group\)' `
    -and $renderer -match 'movement_group->ProcessEvent\(\s*add_child_to_canvas_, &add_height_group\)' `
    -and $renderer -match 'root_panel_ = movement_group' `
    -and $renderer -match 'host_root_panel_ = marker_canvas') `
    'Single-host movement, dual-height, and fixed-clock ownership is not explicit.'
Assert-True ($renderer -notmatch 'Widget:GetParent|CanvasPanelSlot:GetPosition|CanvasPanelSlot:GetSize|CanvasPanelSlot:GetAlignment|CanvasPanelSlot:GetAnchors|CanvasPanelSlot:GetZOrder|player_tree|player_root|encounter_group') `
    'The runtime-faulting split encounter attachment returned.'
Assert-True ($renderer -match 'height_group->ProcessEvent\(add_child_to_canvas_' `
    -and $renderer -match 'height_group->ProcessEvent\(\s*set_render_pivot_' `
    -and $rendererHeader -match 'kCompactUmgHeightChannelCount\s*=\s*2' `
    -and $renderer -match 'CompactUmgHeightChannel::AreaQuest') `
    'The fixed pointer pieces must be children of two centered category groups.'
Assert-True ($renderer -match `
        'height_pointer_fill_color\s*\([\s\S]*?CompactUmgMarkerKind::AreaQuest[\s\S]*?\?\s*kOfficialCyan\s*:\s*treasure_color\(kind\)' `
    -and $renderer -match `
        'const\s+LinearColor\s+fill\s*=\s*height_pointer_fill_color\(height_kind\[channel\]\)' `
    -and $renderer -notmatch `
        'treasure_color\(height_kind\[channel\]\)') `
    'Treasure and area-quest height channels must keep type-specific treasure and official-cyan fill colors.'
Assert-True ($renderer -match 'height_segments\{\{' `
    -and $renderer -match 'height_end_cap_inset' `
    -and $renderer -match 'height_outline_segments\(' `
    -and $renderer -match 'sharp_head_segment\(' `
    -and $renderer -match `
        'height_outline_order\{\{true,\s*false,\s*true,\s*true,\s*false,\s*false\}\}') `
    'The grouped pointer lost its six-piece tangent-tip geometry or inset caps.'
Assert-True ($renderer -match `
        'radius\s*\*\s*std::sqrt\(distance_squared\s*-\s*radius\s*\*\s*radius\)\s*/\s*distance_squared' `
    -and $renderer -match `
        'radial_scale\s*=\s*radius\s*\*\s*radius\s*/\s*distance_squared') `
    'The outer head no longer derives its square-ended Border endpoints from the tangent construction.'
$upperTipCorner = Get-SharpHeadTipCorner -BaseY 7.0 -Upper $true
$lowerTipCorner = Get-SharpHeadTipCorner -BaseY -7.0 -Upper $false
$tipTolerance = 1.0e-9
$upperTipDistance = [Math]::Sqrt($upperTipCorner.X * $upperTipCorner.X + $upperTipCorner.Y * $upperTipCorner.Y)
$lowerTipDistance = [Math]::Sqrt($lowerTipCorner.X * $lowerTipCorner.X + $lowerTipCorner.Y * $lowerTipCorner.Y)
$tipDeltaX = $upperTipCorner.X - $lowerTipCorner.X
$tipDeltaY = $upperTipCorner.Y - $lowerTipCorner.Y
$coincidentTipDistance = [Math]::Sqrt($tipDeltaX * $tipDeltaX + $tipDeltaY * $tipDeltaY)
Assert-True ($upperTipDistance -le $tipTolerance `
    -and $lowerTipDistance -le $tipTolerance `
    -and $coincidentTipDistance -le $tipTolerance) `
    'The numeric tangent construction does not make both outer head corners meet at the exact tip.'
Assert-True ($renderer -match 'height_arrow_right_extent\(' `
    -and $renderer -match `
        'translation_x\s*=\s*-height_marker_half_widths_\[channel\][\s\S]*?-\s*arrow_right_extent') `
    'The grouped pointer must preserve angle-aware chest clearance.'
Assert-True ($renderer -notmatch 'tip_x\s*=\s*height_anchor_x') `
    'The old per-rebind absolute pointer geometry must not return.'
Assert-True ($renderer -match 'case 0:\s*return\s*\{0\.98,\s*0\.98[\s\S]*?kOfficialWhite' `
    -and $renderer -match 'case 0:\s*return\s*\{0\.98,\s*0\.98[\s\S]*?kOfficialPale' `
    -and $renderer -match 'kOfficialGreenDark' `
    -and $renderer -match 'kOfficialCyan' `
    -and $renderer -match '0\.48,\s*0\.14,\s*-0\.12,\s*-0\.02,\s*38\.0' `
    -and $renderer -match '0\.48,\s*0\.14,\s*0\.12,\s*-0\.02,\s*-38\.0' `
    -and $renderer -notmatch 'kBossBacking|kBossIcon|kAssaultBacking|kAssaultGold') `
    'Official-reference compact Boss or Assault contrast geometry regressed.'
Assert-True ($main -match 'EncounterKind::Boss\s*\?\s*30\.0\s*:\s*27\.0' `
    -and $main -match '28\.0,[\s\S]*?CompactUmgMarkerKind::AreaQuest' `
    -and $main -match 'compact_area_quest_height_target_valid_' `
    -and $main -match 'CompactUmgHeightChannel::Treasure' `
    -and $main -match 'CompactUmgHeightChannel::AreaQuest') `
    'Compact Boss, Assault, or area-quest binding sizes regressed.'
Assert-True ($renderer -match '0\.16,\s*0\.16,\s*-0\.22,[^\r\n]+kTreasureOther' `
    -and $renderer -match 'case 0:\s*return\s*\{0\.92,\s*0\.92,[^\r\n]+kTreasureOther' `
    -and $renderer -match '0\.16,\s*0\.16,\s*0\.0,[^\r\n]+kTreasureOther' `
    -and $renderer -match '0\.16,\s*0\.16,\s*0\.22,[^\r\n]+kTreasureOther' `
    -and $renderer -match 'configure_area_quest_outline_brush' `
    -and $renderer -match 'ESlateBrushDrawType::RoundedBox' `
    -and $renderer -match 'kAreaQuestBackdrop\{8\.0F\s*/\s*255\.0F,\s*12\.0F\s*/\s*255\.0F,[\s\S]*?18\.0F\s*/\s*255\.0F,\s*0\.55F\}' `
    -and $renderer -match 'write_brush_float\(brush, 0x3C, kAreaQuestBackdrop\.alpha\)' `
    -and $renderer -match 'write_brush_float\(brush, 0x84, 2\.75F\)' `
    -and $renderer -match 'require_parameters\(1U << 23U, set_brush_, 208\)' `
    -and $renderer -notmatch 'kAreaQuestGold') `
    'Compact area quests must use one ABI-gated translucent charcoal rounded Brush, a thick dark frame, and three larger white dots.'
Assert-True ($renderer -notmatch 'kClockBackground|clock_background_' `
    -and $renderer -match 'kClockReferenceWidth\s*=\s*116\.0' `
    -and $renderer -match 'kClockReferenceHeight\s*=\s*42\.0' `
    -and $renderer -match 'local_center\s*\+\s*178\.0' `
    -and $rendererHeader -match 'kCompactClockPhasePieceCount\s*=\s*10' `
    -and $renderer -match 'clock_phase_pieces_' `
    -and $renderer -match 'dial_center_x\s*=\s*98\.0' `
    -and $renderer -match 'dial_center_y\s*=\s*15\.0' `
    -and $renderer -match 'kMoonPieces' `
    -and $renderer -match '107\.0, 9\.0, 5\.2, 1\.8' `
    -and $renderer -match 'const bool morning = phase == 0U' `
    -and $model -match 'enum class CompactTimePhase[\s\S]*?Morning[\s\S]*?Afternoon[\s\S]*?Evening[\s\S]*?Night') `
    'The lower clock must keep separated digits and four distinct bounded presentation-band glyphs.'
Assert-True ($rendererHeader -match 'clock_phase_brush_template_' `
    -and $renderer -match `
        'configure_clock_phase_brush[\s\S]*?brush\[0x11\]\s*=\s*std::byte\{4\}[\s\S]*?brush\[0x88\]\s*=\s*std::byte\{1\}' `
    -and $renderer -match `
        'if\s*\(\s*!brush_templates_ready_\s*\)[\s\S]*?clock_phase_brush_template_\s*=\s*solid_brush_template_[\s\S]*?configure_clock_phase_brush\(\s*clock_phase_brush_template_\s*\)[\s\S]*?brush_templates_ready_\s*=\s*true' `
    -and $renderer -match `
        'set_brush\(\s*border,\s*set_brush_,\s*clock_phase_brush_template_\s*\)' `
    -and [regex]::Matches(
        $renderer, 'configure_clock_phase_brush\s*\(').Count -eq 2 `
    -and [regex]::Matches(
        $renderer,
        'set_brush\(\s*border,\s*set_brush_,\s*clock_phase_brush_template_\s*\)').Count -eq 1) `
    'Clock presentation-band geometry must use one attachment-configured rounded Brush template with no steady-state Brush mutation.'
Assert-True ($renderer -match 'viewport_dpi_scale_\s*=\s*viewport_scale\.return_value') `
    'The renderer must retain the attachment UMG viewport DPI baseline.'
Assert-True ($renderer -match 'umg_unit_scale_\s*=\s*display_scale_\s*/\s*viewport_dpi_scale_') `
    'UMG local lengths must convert physical reference pixels exactly once.'
Assert-True ($renderer -match '/Script/UMG\.Widget:SetRenderScale' `
    -and $renderer -match `
        'require_parameters\(1U << 24U, set_render_scale_, 16\)' `
    -and $renderer -match `
        'calculate_compact_viewport_layout\([\s\S]*?viewport_width_[\s\S]*?viewport_height_[\s\S]*?viewport_dpi_scale_[\s\S]*?umg_unit_scale_\)' `
    -and $renderProjection -match `
        'host_render_scale\s*=\s*umg_unit_scale\s*/\s*content_umg_unit_scale') `
    'A retained compact host must have an ABI-gated render-only resize correction derived from live viewport and DPI geometry.'
Assert-True ($nativeTests -match `
        'calculate_compact_viewport_layout\(\s*3840\.0,\s*2160\.0,\s*1\.5,\s*1\.0\)' `
    -and $nativeTests -match `
        'calculate_compact_viewport_layout\(\s*2560\.0,\s*1600\.0,\s*1\.0,\s*1\.0\)' `
    -and $nativeTests -match `
        'calculate_compact_viewport_layout\(\s*3440\.0,\s*1440\.0,\s*1\.0,\s*1\.0\)' `
    -and $nativeTests -match `
        'calculate_compact_viewport_layout\(\s*3840\.0,\s*1600\.0,\s*1\.0,\s*1\.0\)' `
    -and $nativeTests -match `
        'calculate_compact_viewport_layout\(\s*2560\.0,\s*1080\.0,\s*1\.0,\s*1\.0\)' `
    -and $nativeTests -match `
        'host_origin_x,\s*2097\.75') `
    'Native tests must pin deterministic fullscreen, ultrawide, and window-shaped numeric viewport/DPI inputs without claiming runtime black-bar geometry.'
Assert-True ($renderer -match 'marker_canvas->ProcessEvent\(clear_children_, nullptr\)') `
    'The complete authored Blueprint visual tree must be removed before marker creation.'
Assert-True ($rendererHeader -match 'kCompactUmgMarkerCapacity\s*=\s*80') `
    'The runtime pool must preserve the accepted stable capacity of 80 markers.'
Assert-True ($rendererHeader -match `
        'CompactUmgMarkerKind[\s\S]*?AreaQuest,[\s\S]*?BirdEgg,[\s\S]*?\};' `
    -and $rendererHeader -match `
        'bird_egg_brush_template_' `
    -and $renderer -match `
        'kind_code[\s\S]*?<=\s*static_cast<std::uint8_t>\(CompactUmgMarkerKind::BirdEgg\)' `
    -and $renderer -match `
        'case\s+CompactUmgMarkerKind::BirdEgg:[\s\S]*?kBirdEggShell[\s\S]*?kBirdEggHighlight[\s\S]*?kBirdEggNest' `
    -and $renderer -match `
        'configure_bird_egg_oval_brush[\s\S]*?brush\[0x11\]\s*=\s*std::byte\{4\}[\s\S]*?brush\[0x88\]\s*=\s*std::byte\{1\}' `
    -and $renderer -match `
        'case\s+CompactUmgMarkerKind::BirdEgg:[\s\S]*?return\s*\{0\.76,\s*1\.00[\s\S]*?return\s*\{0\.58,\s*0\.80' `
    -and $renderer -match `
        'CompactUmgMarkerKind::BirdEgg[\s\S]*?bird_egg_brush_template_' `
    -and $mainCode -match `
        'candidate\.position,\s*18\.0,[\s\S]*?CompactUmgMarkerKind::BirdEgg') `
    'Bird eggs must remain a valid four-piece vertical-oval compact marker without expanding the accepted 80-slot UMG pool.'
Assert-True ($rendererHeader -match 'FWeakObjectPtr') `
    'Runtime UMG identities must be retained weakly.'
Assert-True ($renderer -match 'kReferenceMarkerExtent\s*=\s*170\.0') `
    'Compact projection must use the accepted 170-pixel reference radius.'
Assert-True ($renderer -match 'kReferenceHostHalfSize\s*=') `
    'The compact host must have an explicit bounded local surface.'
Assert-True ($renderer -match 'const double local_center = kReferenceHostHalfSize \* umg_unit_scale_') `
    'Marker slots must use compact-host local coordinates.'
Assert-True ($renderer -match 'normalized_dx \* marker_extent') `
    'Steady render translation must use only the numeric motion delta.'

$clearChildren = $renderer.IndexOf('marker_canvas->ProcessEvent(clear_children_, nullptr)')
$firstMarkerAdd = $renderer.IndexOf('movement_group->ProcessEvent(add_child_to_canvas_, &add_piece)')
Assert-True ($clearChildren -ge 0 -and $firstMarkerAdd -gt $clearChildren) `
    'The authored Blueprint visual tree must be cleared before radar markers are added.'

$rebind = [regex]::Match(
    $renderer,
    'bool CompactUmgRenderer::rebind_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$translate = [regex]::Match(
    $renderer,
    'bool CompactUmgRenderer::translate_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$heightTransform = [regex]::Match(
    $renderer,
    'bool CompactUmgRenderer::apply_height_pointer_transform_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$scaleRead = [regex]::Match(
    $renderer,
    'bool CompactUmgRenderer::read_minimap_scale_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$viewportRefresh = [regex]::Match(
    $renderer,
    'bool CompactUmgRenderer::refresh_viewport_layout_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$clockUpdate = [regex]::Match(
    $renderer,
    'bool CompactUmgRenderer::update_world_clock_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
Assert-True ($rebind.Length -gt 0) 'Low-frequency marker binding path was not found.'
Assert-True ($translate.Length -gt 0) 'High-frequency panel translation path was not found.'
Assert-True ($heightTransform.Length -gt 0) 'Grouped height transform path was not found.'
Assert-True ($scaleRead.Length -gt 0) 'Live minimap-scale read path was not found.'
Assert-True ($viewportRefresh.Length -gt 0) 'Low-frequency viewport reflow path was not found.'
Assert-True ($clockUpdate.Length -gt 0) 'Native fixed clock update path was not found.'
$rebindCode = Remove-CppComments $rebind
$translateCode = Remove-CppComments $translate
$heightTransformCode = Remove-CppComments $heightTransform
$scaleReadCode = Remove-CppComments $scaleRead
$viewportRefreshCode = Remove-CppComments $viewportRefresh
$clockUpdateCode = Remove-CppComments $clockUpdate
foreach ($forbidden in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'NewObject',
        'StaticConstructObject', 'ofstream', 'ifstream', 'append_log', 'new ')) {
    Assert-True ($rebindCode -notmatch [regex]::Escape($forbidden)) `
        "Compact rebind path contains forbidden work: $forbidden"
    Assert-True ($translateCode -notmatch [regex]::Escape($forbidden)) `
        "Compact translation path contains forbidden work: $forbidden"
    Assert-True ($heightTransformCode -notmatch [regex]::Escape($forbidden)) `
        "Height transform path contains forbidden work: $forbidden"
}
foreach ($forbidden in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'NewObject',
        'StaticConstructObject', 'ofstream', 'ifstream', 'append_log',
        'filesystem')) {
    Assert-True ($clockUpdateCode -notmatch [regex]::Escape($forbidden)) `
        "Native clock minute-edge path contains forbidden work: $forbidden"
}
Assert-True ($clockUpdateCode -notmatch `
        'set_brush\s*\(|configure_clock_phase_brush|GetValuePtrByPropertyNameInChain') `
    'The native clock minute-edge path must not mutate or reconstruct Slate Brush state.'
Assert-True ($renderer -match 'minute == clock_minute_' `
    -and $renderer -match '!available && !clock_visible_' `
    -and $main -match 'update_world_clock\(') `
    'The native clock must remain scalar-only between displayed-minute edges.'
foreach ($forbidden in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'NewObject',
        'StaticConstructObject', 'ofstream', 'ifstream',
        'append_log', 'new ')) {
    Assert-True ($scaleReadCode -notmatch [regex]::Escape($forbidden)) `
        "One-hertz minimap-scale read contains forbidden work: $forbidden"
}
Assert-True ($scaleReadCode -match `
        'refresh_viewport_layout_unsafe\s*\(\s*player_icon\s*\)') `
    'The existing one-hertz minimap sample must also refresh retained viewport geometry.'
foreach ($forbidden in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'NewObject',
        'StaticConstructObject', 'ofstream', 'ifstream', 'append_log',
        'filesystem', 'for\s*\(', 'while\s*\(')) {
    Assert-True ($viewportRefreshCode -notmatch $forbidden) `
        "Low-frequency viewport reflow contains forbidden work: $forbidden"
}
$viewportRefreshCalls = [regex]::Matches(
    $viewportRefreshCode, 'ProcessEvent\(').Count
Assert-True ($viewportRefreshCalls -eq 7 `
    -and $viewportRefreshCode -match `
        'if\s*\(!viewport_changed\)\s*\{\s*return true;\s*\}' `
    -and $viewportRefreshCode -match `
        'host->ProcessEvent\(set_render_scale_' `
    -and $viewportRefreshCode -match `
        'host->ProcessEvent\(set_position_in_viewport_') `
    "Viewport reflow must use seven bounded reflected call sites and mutate the host only after a geometry edge; found $viewportRefreshCalls."
$translationCalls = [regex]::Matches($translateCode, 'ProcessEvent\(').Count
Assert-True ($translationCalls -eq 1 `
    -and $translateCode -match 'root_panel->ProcessEvent\(set_render_translation_') `
    "Single-host translation must contain exactly one root call; found $translationCalls."
$heightRenderCalls = [regex]::Matches($heightTransformCode, 'ProcessEvent\(').Count `
    + [regex]::Matches($heightTransformCode, 'set_render_angle\(').Count
Assert-True ($heightRenderCalls -eq 2) `
    "Grouped height motion must contain exactly two render-only calls; found $heightRenderCalls."
Assert-True ($heightTransformCode -notmatch `
        'set_slot_position_|set_slot_size_|set_marker_piece_geometry|for\s*\(') `
    'High-frequency height motion must not mutate layout or loop over pointer pieces.'

Assert-True ($model -match 'kMaximumSelectedTreasures\s*=\s*80') `
    'Compact treasure selection capacity must remain bounded to 80.'
Assert-True ($model -match 'kCompactMapId\s*=\s*100') `
    'Compact treasure selection must remain constrained to map ID 100.'
Assert-True ($model -match 'kComparablePlayerZOffset\s*=\s*-150\.0') `
    'Nearest treasure ranking must preserve the player Z adjustment.'
Assert-True ($model -match 'marker\.nearest = output_index == 0') `
    'Only one nearest treasure may receive emphasis.'
Assert-True ($model -match 'height_target_z' `
    -and $model -match 'height_angle_from_delta') `
    'Height motion must retain only scalar target Z and use the shared pure angle helper.'
Assert-True ($model -match 'choose_compact_nearest' `
    -and $model -match 'retained_distance <= best_distance \+ switch_advantage') `
    'Nearest-target stabilization is missing or no longer distance bounded.'
Assert-True ($model -match 'promote_compact_nearest' `
    -and $model -match 'std::rotate\(' `
    -and $main -match 'promote_compact_nearest\(') `
    'Stable-nearest promotion must preserve the remaining distance order.'
Assert-True ($model -match 'effective_magnitude' `
    -and $model -match 'std::copysign') `
    'Height dead-zone mapping must remain continuous at its boundary.'

foreach ($catalog in @(
        'treasure-actors.tsv', 'boss-actors.tsv', 'assault-actors.tsv',
        'area-quests.tsv')) {
    Assert-True ($deploy -match [regex]::Escape($catalog)) `
        "Deployment does not validate required native catalog: $catalog"
}
Assert-True ($deploy.Contains('Header = "SaveId`tClassName`tX`tY`tZ"')) `
    'Deployment does not validate the treasure actor SaveId header.'
Assert-True ($deploy -match `
        'function\s+Assert-DragonSwordStopped' `
    -and $deploy -match `
        'Refusing to \$Operation while DragonSword is running' `
    -and [regex]::Matches(
        $deploy,
        'Assert-DragonSwordStopped\s+-Operation').Count -ge 3) `
    'Deployment must fail closed while the game is running.'
Assert-True ($deploy -match 'function\s+Remove-ValidModEntries' `
    -and $deploy -match 'Remove-ValidModEntries\s+\$stableName' `
    -and $deploy -notmatch 'Set-ModState\s+\$stableName\s+0') `
    'The predecessor Radar authority must be removed without a redundant disabled entry.'

Assert-True ($main -match '#include "native_save_reconciler\.hpp"') `
    'The native owner does not include the one-shot save reconciler.'
Assert-True ($main -match 'kPositionInterval\s*=\s*std::chrono::milliseconds\{16\}') `
    'Compact motion sampling must run at the accepted 16 ms cadence.'
Assert-True ($main -notmatch `
        'kHeightPointerUpdateInterval|compact_height_update_after_') `
    'Height motion must not stack a second cadence gate on position sampling.'
Assert-True ($main -match 'kCompactNearestSwitchAdvantage\s*=\s*100\.0') `
    'Nearest-target stabilization must preserve the one-metre advantage threshold.'
Assert-True ($main -match 'choose_compact_nearest\(' `
    -and $main -match 'compact_height_target_z_' `
    -and $main -match 'update_height_pointer\(') `
    'The owner is missing stable-target selection or scalar height updates.'
Assert-True ($main -match '\b22\.0,' -and $main -match '\b14\.0,') `
    'Treasure icon sizes must preserve the strengthened 22/14 nearest emphasis.'
Assert-True ($main -match 'nearest\.height_available' `
    -and $main -match 'nearest\.height_angle_degrees' `
    -and $main -match 'compact_area_quest_height_target_valid_') `
    'The nearest treasure and nearest visible area quest must request independent bounded height channels.'
Assert-True ($main -match 'SAVE_RECONCILE_APPLIED') `
    'Successful native save reconciliation is not applied to eligibility.'
Assert-True ($main -match 'mark_treasure_opened\(observed\.id\)') `
    'Native disappearance evidence does not update compact eligibility.'
Assert-True ($cmake -match 'src/native/native_save_reconciler\.cpp') `
    'The native save reconciler is missing from the native target.'
Assert-True ($rendererHeader -match 'kCompactUmgMarkerPieceCount\s*=\s*4') `
    'Each fixed marker must use exactly four bounded glyph pieces.'
Assert-True ($rendererHeader -notmatch 'kCompactUmgEncounterMarkerCapacity|kCompactUmgEncounterPieceCount|encounter_root_panel_') `
    'A second encounter widget pool must not return after the dev40 runtime fault.'
Assert-True ($rendererHeader -match 'kCompactUmgHeightChannelCount\s*=\s*2' `
    -and $rendererHeader -match 'kCompactUmgHeightPieceCount\s*=\s*6') `
    'Each of the two fixed height channels must use exactly six preallocated pieces.'
Assert-True ($rendererHeader -match 'Boss' -and $rendererHeader -match 'Assault' `
    -and $rendererHeader -match 'Fly' -and $rendererHeader -match 'Mole' `
    -and $rendererHeader -match 'Wave' -and $rendererHeader -match 'AreaQuest' `
    -and $rendererHeader -match 'BirdEgg') `
    'The compact marker kind set is incomplete.'
Assert-True ($renderer -match 'kFlyWing\s*\{91\.0F\s*/\s*255\.0F,\s*178\.0F\s*/\s*255\.0F,\s*244\.0F\s*/\s*255\.0F' `
    -and $renderer -match 'kFlyArrow\s*\{35\.0F\s*/\s*255\.0F,\s*120\.0F\s*/\s*255\.0F,\s*218\.0F\s*/\s*255\.0F' `
    -and $renderer -match 'kFlyOutline\s*\{18\.0F\s*/\s*255\.0F,\s*51\.0F\s*/\s*255\.0F,\s*84\.0F\s*/\s*255\.0F' `
    -and $renderer -match 'configure_fly_outline_brush' `
    -and $renderer -match 'write_brush_float\(brush,\s*0x84,\s*1\.25F\)' `
    -and $renderer -match 'marker\.kind\s*==\s*CompactUmgMarkerKind::Fly[\s\S]*?fly_outline_brush_template_') `
    'Compact Fly markers must use the deeper blue palette and the single-brush native outline.'
Assert-True ($translateCode -match 'set_render_translation_') `
    'High-frequency motion must use render translation instead of layout mutation.'
Assert-True ($translateCode -notmatch 'set_position_in_viewport_|nearest_arrow') `
    'High-frequency translation must not mutate viewport layout or retain the test arrow.'

foreach ($forbidden in @(
        'UObject', 'FindAllOf', 'FindFirstOf', 'StaticFindObject',
        'ProcessEvent')) {
    Assert-True ($saveReconciler -notmatch [regex]::Escape($forbidden)) `
        "Background save reconciliation must not touch Unreal state: $forbidden"
}
Assert-True ($saveReconciler -match 'THREAD_PRIORITY_BELOW_NORMAL') `
    'The one-shot save worker must remain below normal priority.'
Assert-True ($saveReconciler -match `
        'request_active_\s*\|\|\s*pending_\.has_value\(\)[\s\S]*?\|\|\s*completed_\.has_value\(\)' `
    -and $saveReconcilerHeader -match `
        'enum\s+class\s+SaveReconcileScope[\s\S]*?FullActivation[\s\S]*?CompletionConfirmation[\s\S]*?TreasureConfirmation' `
    -and $saveReconciler -match `
        'request\.scope\s*==\s*SaveReconcileScope::FullActivation[\s\S]*?SaveReconcileScope::TreasureConfirmation[\s\S]*?\?\s*&source_fields\s*:\s*nullptr' `
    -and $saveReconciler -match `
        'WHERE CATEGORY IN \(' `
    -and $saveReconcilerHeader -match `
        'kMaximumTreasureConfirmationIds\s*=\s*64U') `
    'Save reconciliation must allow one bounded request and restrict event-driven treasure confirmation to at most 64 exact categories.'
$queueAreaQuestSaveConfirmation = Get-MainFunction `
    $mainCode 'queue_area_quest_save_confirmation'
$collectAreaQuestSaveConfirmations = Get-MainFunction `
    $mainCode 'collect_due_area_quest_save_confirmations'
$queueTreasureSaveConfirmation = Get-MainFunction `
    $mainCode 'queue_treasure_save_confirmation'
$collectTreasureSaveConfirmations = Get-MainFunction `
    $mainCode 'collect_due_treasure_save_confirmations'
$applyTreasureSaveConfirmation = Get-MainFunction `
    $mainCode 'apply_treasure_save_confirmation_result'
$applyCompletionConfirmation = Get-MainFunction `
    $mainCode 'apply_completion_confirmation_result'
$requestSaveReconcile = Get-MainFunction `
    $mainCode 'request_or_apply_save_reconcile'
$serviceCompletionWitnesses = Get-MainFunction `
    $mainCode 'service_area_quest_completion_witnesses'
Assert-True ($mainCode -match `
        'std::array<std::uint64_t,\s*kAreaQuestCompletionWordCount>\s*area_quest_save_confirmation_pending_' `
    -and $mainCode -match `
        'std::array<std::uint64_t,\s*kAreaQuestCompletionWordCount>\s*area_quest_save_confirmation_inflight_' `
    -and $mainCode -match `
        'kAreaQuestSaveConfirmationRetryDelay\s*=\s*[\s\S]*?std::chrono::seconds\{15\}' `
    -and $mainCode -match `
        'kAreaQuestSaveConfirmationMaximumAttempts\s*=\s*3U' `
    -and $queueAreaQuestSaveConfirmation -match `
        'area_quest_save_confirmation_pending_\[word\]\s*\|=\s*bit' `
    -and $serviceCompletionWitnesses -match `
        'if\s*\(expired\)[\s\S]*?queue_area_quest_save_confirmation\([\s\S]*?exact_task_identity_no_end_state_after_10s' `
    -and [regex]::Matches(
        $mainCode,
        'queue_area_quest_save_confirmation\(').Count -eq 2 `
    -and $collectAreaQuestSaveConfirmations -match `
        'area_quest_save_confirmation_pending_\[word\]\s*&=\s*~bit[\s\S]*?area_quest_save_confirmation_inflight_\[word\]\s*\|=\s*bit[\s\S]*?\+\+area_quest_save_confirmation_attempts_\[index\]' `
    -and $requestSaveReconcile -match `
        'collect_due_area_quest_save_confirmations\(now\)[\s\S]*?SaveReconcileScope::CompletionConfirmation' `
    -and $applyCompletionConfirmation -match `
        'accept_area_quest_save_confirmation\([\s\S]*?apply_exact_area_quest_completion\(index\)[\s\S]*?retry_area_quest_save_confirmation\([\s\S]*?now\s*\+\s*kAreaQuestSaveConfirmationRetryDelay' `
    -and $nativeTests -match `
        'area-quest save fallback must be positive-only and bounded to three event-driven attempts') `
    'Expired exact area-quest witnesses must use fixed-bit, coalesced, positive-only confirmation with three bounded event-driven attempts and no poll.'
Assert-True ($areaQuestVisibility -match `
        'kUnknownAreaQuestSaveCompletionCount\s*=\s*-1' `
    -and $areaQuestVisibility -match `
        'accept_area_quest_save_confirmation[\s\S]*?verified_baseline_count[\s\S]*?complete_count\s*>\s*verified_baseline_count' `
    -and $areaQuestVisibility -match `
        'area_quest_completion_generation_may_arm[\s\S]*?area_quest_completion_generation_arms_reactivation[\s\S]*?area_quest_completion_generation_may_unlock' `
    -and $mainCode -match `
        'std::array<std::int64_t,\s*kExpectedAreaQuestCount>\s*area_quest_save_completion_counts_' `
    -and $mainCode -match `
        'std::array<bool,\s*kExpectedAreaQuestCount>\s*area_quest_completion_generation_locked_' `
    -and $queueAreaQuestSaveConfirmation -match `
        'verified_complete_count_baseline_unknown[\s\S]*?same_cycle_generation_locked[\s\S]*?sql_requested=false' `
    -and $queueAreaQuestSaveConfirmation -match `
        'verified_baseline_count[\s\S]*?acceptance=strict_complete_count_growth' `
    -and $applyCompletionConfirmation -match `
        'area_quest_completion_generation_locked_\[index\]\s*=\s*true[\s\S]*?area_generation_locked' `
    -and $mainCode -match `
        'AREA_QUEST_COMPLETION_GENERATION_REARMED[\s\S]*?evidence=inactive_none_or_end_then_active' `
    -and $nativeTests -match `
        'save row must grow beyond its verified per-ID baseline' `
    -and $nativeTests -match `
        'failed area-quest generation must suppress repeated same-cycle witnesses' `
    -and $nativeTests -match `
        'unlock only after an inactive NONE/END sample followed by a fresh active sample') `
    'Area-quest SQL confirmation must require strict growth over a verified per-ID F7 baseline and lock an exhausted generation until a strict inactive-to-active boundary.'
Assert-True ($mainCode -match `
        'kTreasureSaveConfirmationInitialDelay\s*=\s*[\s\S]*?std::chrono::seconds\{15\}' `
    -and $mainCode -match `
        'kTreasureSaveConfirmationRetryDelay\s*=\s*[\s\S]*?std::chrono::seconds\{285\}' `
    -and $mainCode -match `
        'kTreasureSaveConfirmationMaximumAttempts\s*=\s*2U' `
    -and $mainCode -match `
        'std::array<std::uint64_t,\s*kTreasureSaveConfirmationWordCount>\s*treasure_save_confirmation_pending_' `
    -and $queueTreasureSaveConfirmation -match `
        'treasure_save_confirmation_pending_\[word\]\s*\|=\s*bit' `
    -and $collectTreasureSaveConfirmations -match `
        '!treasure_save_confirmation_pending_armed_[\s\S]*?now\s*<\s*treasure_save_confirmation_next_due_[\s\S]*?return\s+0' `
    -and $requestSaveReconcile -match `
        'SaveReconcileScope::TreasureConfirmation[\s\S]*?treasure_confirmation_ids\.data\(\)' `
    -and $applyTreasureSaveConfirmation -match `
        'accept_treasure_save_confirmation\([\s\S]*?save_result_contains\(result,\s*save_id\)[\s\S]*?mark_treasure_opened\(save_id\)' `
    -and $applyTreasureSaveConfirmation -match `
        'retry_treasure_save_confirmation\([\s\S]*?treasure_save_confirmation_attempts_\[index\][\s\S]*?kTreasureSaveConfirmationMaximumAttempts[\s\S]*?kTreasureSaveConfirmationRetryDelay' `
    -and $nativeTests -match `
        'underwater save confirmation must require an exact positive requested bit and allow only one bounded retry' `
    -and $mainCode -notmatch `
        'kTreasureSaveConfirmationPoll|TreasureConfirmation.*std::this_thread::sleep') `
    'Rejected nearby treasure interactions must use a dormant fixed-bit exact-ID confirmation with two bounded positive-only attempts and no polling.'
foreach ($queryLimit in @(
        [pscustomobject]@{ Name = 'kMaximumOpenedQueryRows'; Value = 4096 },
        [pscustomobject]@{ Name = 'kMaximumEncounterQueryRows'; Value = 65536 },
        [pscustomobject]@{ Name = 'kMaximumDynamicQuestCompletionQueryRows'; Value = 65536 })) {
    Assert-True ($saveReconciler -match
        ("constexpr\s+std::size_t\s+{0}\s*=\s*{1}\s*;" -f
            $queryLimit.Name, $queryLimit.Value)) `
        "Save reconciliation query-row limit changed: $($queryLimit.Name)"
}
foreach ($callbackName in @(
        'opened_query_callback',
        'encounter_query_callback',
        'dynamic_quest_completion_query_callback')) {
    $callback = [regex]::Match(
        $saveReconciler,
        ('(?ms)^int\s+__cdecl\s+{0}\s*\([^;]*?\)\s*noexcept\s*\{{(?:(?!^\}}).)*^\}}' -f
            [regex]::Escape($callbackName)))
    Assert-True $callback.Success `
        "Required SQLite callback was not found: $callbackName"
    Assert-True ($callback.Value -match 'row_count\s*>=\s*kMaximum' `
        -and $callback.Value -match 'catch\s*\(\.\.\.\)' `
        -and $callback.Value -match
            'catch\s*\(\.\.\.\)\s*\{\s*context->parse_failed\s*=\s*true\s*;\s*return\s+1\s*;') `
        "SQLite callback must stop at its fixed row limit and fail closed on every C++ exception: $callbackName"
}
Assert-True ($deploy -match 'e_sqlcipher\.dll') `
    'Deployment does not include the native one-shot SQLCipher dependency.'
Assert-True ($deploy -match 'treasure_overrides\.txt') `
    'Deployment does not include the static treasure override default.'
Assert-True ($main -match
        '!ignored_treasure_ids_\.contains\(id\)[\s\S]*?!runtime_opened_treasure_ids_\.contains\(id\)[\s\S]*?!save_result_contains\(\*result, id\)') `
    'Treasure eligibility must apply the exact ignore before runtime and save completion evidence.'
Assert-True ($deploy -match 'renderCatalogPath\s*=\s*Join-Path\s+\$sourceData\s+''treasures\.lua''') `
    'Deployment does not require the section-aware render catalog.'
Assert-True ($deploy -match 'renderCatalogRows\.Count -ne 1693') `
    'Deployment does not validate the complete render catalog count.'
Assert-True ($deploy -match 'worldMapRows\.Count -ne 1506') `
    'Deployment does not validate the map-100 render subset.'
Assert-True ($deploy -match 'renderCatalogIds.*Sort-Object -Unique') `
    'Deployment does not reject duplicate render SaveId values.'
Assert-True ($deploy -match 'Installed section-aware render catalog hash') `
    'Deployment does not verify the installed render catalog hash.'
Assert-True ($main -match 'kCompactRebindInterval\s*=\s*std::chrono::seconds\{5\}') `
    'Unchanged marker bindings must not return to the once-per-second layout path.'
Assert-True ($main -match 'load_render_catalog') `
    'The compact renderer must load the section-aware treasure catalog.'
Assert-True ($main -match 'world_count != 1506') `
    'The section-aware catalog must validate the current map-100 count.'
Assert-True ($main -match 'world treasure render ID is duplicated') `
    'The native loader must reject duplicate render catalog IDs.'
Assert-True ($main -match 'kExpectedEncounterCount\s*=\s*49' `
    -and $main -match 'kExpectedMiniGameCount\s*=\s*83' `
    -and $main -match 'kExpectedAreaQuestCount\s*=\s*147') `
    'Mixed compact catalog bounds are missing.'
Assert-True ($main -match 'GetQuestInfoInStandAlone' `
    -and $main -match 'area_quest_compact_visible\(decoded_state\)' `
    -and $main -match 'SetPropertyValue\(dynamic_value,\s*true\)' `
    -and $main -match 'AREA_QUEST_STATE_SCAN_COMPLETE' `
    -and $main -match 'schedule=one_id_per_game_frame' `
    -and $main -match 'area_quest_scan_faulted_') `
    'Dynamic area quest eligibility is not bounded, reflected, or fail closed.'
Assert-True ($main -match 'kAreaQuestRefreshDebounce\s*=\s*std::chrono::milliseconds\{1000\}' `
    -and $main -match 'area_quest_scan_states_' `
    -and $main -match 'area_quest_scan_eligibility_' `
    -and $main -match 'area_quest_scan_completion_observed_' `
    -and $main -match 'publication=transactional' `
    -and $main -match 'area_quest_completion_transition\(' `
    -and $main -match 'published_snapshot_preserved=' `
    -and $main -match 'quest_state_event_debounced') `
    'Quest-end task refresh must remain debounced, transactional, and terminal-evidence gated.'
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
    'Exact completion is not fenced from stale active samples until a later NONE/END boundary proves repeatable reactivation.'
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
    'The direct task-class map must reflect TaskActorClassContainer/DynamicQuestTaskList and filter Quest_Dynamic TaskUseType=4 by full class name.'
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
    'The direct task-class map must publish only exact, unambiguous 147-of-147 catalog coverage.'

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
    'A delayed save snapshot must preserve newer runtime state and refresh only the exact current world-map session.'
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
$activitySuppressionEdge = Get-MainFunction `
    $mainCode 'apply_activity_suppression_edge'
$compactRenderSuppressed = Get-MainFunction `
    $mainCode 'compact_render_suppressed'
$readPlayerPosition = Get-MainFunction $mainCode 'read_player_position'
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
Assert-True ($main -match `
        'compact_transition_hide=first_invalid_position_sample' `
    -and $compactRenderSuppressed -match '!position_valid_' `
    -and $engineTickUnsafe -match `
        'else\s*\{\s*position_valid_\s*=\s*false;[\s\S]*?apply_compact_suppression\(\);') `
    'The compact host must collapse on the first failed current-Pawn sample.'
$cursorReadIndex = $readPlayerPosition.IndexOf(
    'STR("bShowMouseCursor")', [StringComparison]::Ordinal)
$pawnNullIndex = $readPlayerPosition.IndexOf(
    'if (!pawn) return false;', [StringComparison]::Ordinal)
Assert-True ($cursorReadIndex -ge 0 `
    -and $pawnNullIndex -gt $cursorReadIndex) `
    'Cursor suppression must update from the current controller even when no Pawn exists.'
Assert-True ($mainCode -match '/Script/DS\.DETTaskBaseActor:OnRecvCompleteQuest' `
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
    -and $latchExactCompletion -match `
        'area_quest_eligibility_\[index\]\s*=\s*0' `
    -and $latchExactCompletion -match `
        'completed_dynamic_quest_ids_\.insert\(quest_id\)' `
    -and $latchExactCompletion -match `
        'area_quest_repeatable_reactivation_armed_\[index\]\s*=\s*false' `
    -and $latchExactCompletion -match `
        'area_quest_scan_repeatable_reactivation_armed_\[index\]\s*=\s*false' `
    -and $latchExactCompletion -match 'compact_rebind_dirty_\s*=\s*true' `
    -and $applyExactCompletion -match `
        'refresh_world_map_atlas_for_runtime_delta\(\s*"area_quest_exact_completion"' `
    -and $mainCode -match `
        'request_area_quest_scan\("quest_state_event_debounced"\)') `
    'Exact mapped completion must publish immediately while every completion still requests one generic debounced transactional refresh.'
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
        'area_quest_exact_completion_transition|candidate_count\s*==\s*1' `
    -and $nativeTests -notmatch 'ID-less completion event') `
    'The removed dev53 ID-less candidate/unambiguous inference path is still present.'
Assert-True ($main -match 'kAreaQuestRefreshMaxDebounce\s*=\s*std::chrono::milliseconds\{2000\}' `
    -and $main -match 'schedule_area_quest_rescan' `
    -and $main -match 'std::min\(\s*requested_after,\s*area_quest_rescan_deadline_\)') `
    'Quest callback coalescing must retain a bounded maximum delay.'

$onUnrealInit = Get-MainFunction $mainCode 'on_unreal_init'
$initializeQuestEvent = Get-MainFunction `
    $mainCode 'initialize_quest_event_trigger_provider'
$questEventPre = Get-MainFunction $mainCode 'quest_event_trigger_pre'
$questEventPreUnsafe = Get-MainFunction `
    $mainCode 'quest_event_trigger_pre_unsafe'
$armCompletionWitness = Get-MainFunction `
    $mainCode 'arm_area_quest_completion_witness'
$serviceCompletionWitnesses = Get-MainFunction `
    $mainCode 'service_area_quest_completion_witnesses'
$initializeTreasureInteract = Get-MainFunction `
    $mainCode 'initialize_treasure_interact_schema'
$treasureInteractPre = Get-MainFunction $mainCode 'treasure_interact_pre'
$treasureInteractPreUnsafe = Get-MainFunction `
    $mainCode 'treasure_interact_pre_unsafe'
$queueRejectedTreasureConfirmation = Get-MainFunction `
    $mainCode 'queue_rejected_treasure_interaction_confirmation'
$treasureDeathPre = Get-MainFunction $mainCode 'treasure_death_pre'
$treasureCompletionEvent = Get-MainFunction `
    $mainCode 'treasure_completion_event_unsafe'
$resolveTreasureActorId = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}\[\[nodiscard\]\]\s+std::optional<std::int64_t>\s+resolve_treasure_actor_id\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
$unregisterCallbacks = Get-MainFunction $mainCode 'unregister_callbacks'
$activateOwner = Get-MainFunction $mainCode 'activate'
$serviceEngineTickFault = Get-MainFunction `
    $mainCode 'service_engine_tick_fault'
$shutdownOwner = Get-MainFunction `
    $mainCode 'shutdown_for_process_lifetime'
Assert-True ($onUnrealInit -match `
        'const bool required_runtime_ready\s*=\s*catalog_ready_[\s\S]*?world_map_layer_class_[\s\S]*?compact_layer_class_[\s\S]*?object_create_listener_registered_\.load\([\s\S]*?encounter_class_name_keys_ready_[\s\S]*?transition_post_id_\s*!=\s*Hook::ERROR_ID' `
    -and [regex]::Matches(
        $onUnrealInit,
        'required_runtime_ready_\.store\(').Count -eq 1 `
    -and $onUnrealInit -match `
        'if\s*\(!required_runtime_ready\)[\s\S]*?enabled_\s*=\s*false[\s\S]*?unregister_object_create_listener\(\)[\s\S]*?DISABLED[\s\S]*?return' `
    -and $engineTickUnsafe -match `
        'if\s*\(!required_runtime_ready_\.load\(std::memory_order_acquire\)\)[\s\S]*?F7_REJECTED[\s\S]*?enabled_\s*=\s*false[\s\S]*?return' `
    -and $activateOwner -match `
        '!required_runtime_ready_\.load\(std::memory_order_acquire\)[\s\S]*?ACTIVATION_REJECTED[\s\S]*?return' `
    -and $serviceEngineTickFault -match `
        'const bool recover\s*=\s*was_enabled\s*&&\s*engine[\s\S]*?required_runtime_ready_\.load\(std::memory_order_acquire\)' `
    -and $shutdownOwner -match `
        'required_runtime_ready_\.store\(false,\s*std::memory_order_release\)' `
    -and $transitionBegin -match `
        '!required_runtime_ready_\.load\(std::memory_order_acquire\)' `
    -and $transitionEndLifecycle -match `
        '!required_runtime_ready_\.load\(std::memory_order_acquire\)' `
    -and $mainCode -notmatch `
        'required_runtime_ready_\.store\(\s*true' `
    -and $main -match 'required_runtime_ready=true') `
    'Required metadata, catalogs, listener, lifecycle hooks, F7, travel, and bounded fault recovery must share one immutable fail-closed runtime readiness latch.'
$disableForMainMenu = Get-MainFunction `
    $mainCode 'disable_for_main_menu_owner_boundary'
$probeActivityContext = Get-MainFunction `
    $mainCode 'probe_activity_context_unsafe'
$mainMenuWorldIdentity = Get-MainFunction `
    $mainCode 'is_main_menu_world_identity'
$captureCurrentWorldIdentity = Get-MainFunction `
    $mainCode 'capture_current_world_identity_guarded'
$titleTransitionIndex = $transitionEndLifecycle.IndexOf(
    'if (is_main_menu_world_identity(current_world_key_))',
    [StringComparison]::Ordinal)
$disabledTransitionIndex = $transitionEndLifecycle.IndexOf(
    'if (!enabled_ || main_menu_activation_latched_)',
    [StringComparison]::Ordinal)
$compactTransitionActivationIndex = $transitionEndLifecycle.IndexOf(
    'compact_umg_renderer_.begin_activation();',
    [StringComparison]::Ordinal)
$worldMapTransitionActivationIndex = $transitionEndLifecycle.IndexOf(
    'world_map_umg_renderer_.begin_activation();',
    [StringComparison]::Ordinal)
Assert-True ($objectState -match `
        'is_main_menu_world_identity\([\s\S]*?==\s*"World /Game/Title/TitleMap/DS_Title\.DS_Title"' `
    -and $nativeTests -match `
        'the exact title world must be a save-owner boundary' `
    -and $nativeTests -match `
        'the transition map must not become a save-owner boundary' `
    -and $nativeTests -match `
        'the open world must not become a save-owner boundary' `
    -and $nativeTests -match `
        'a broad title-path match must fail closed' `
    -and $nativeTests -match `
        'bird-egg and title-world policy checks must not allocate' `
    -and $mainMenuWorldIdentity -match `
        'return\s+dswros::is_main_menu_world_identity\(key\)') `
    'The title-map save-owner boundary must use one exact, allocation-free world-identity policy with negative transition/open-world coverage.'
Assert-True ($titleTransitionIndex -ge 0 `
    -and $disabledTransitionIndex -gt $titleTransitionIndex `
    -and $compactTransitionActivationIndex -gt $disabledTransitionIndex `
    -and $worldMapTransitionActivationIndex -gt $disabledTransitionIndex `
    -and $transitionEndLifecycle -match `
        'is_main_menu_world_identity\(current_world_key_\)[\s\S]*?disable_for_main_menu_owner_boundary\("transition-end"\)[\s\S]*?return' `
    -and $probeActivityContext -match `
        'is_main_menu_world_identity\(current_world_key_\)[\s\S]*?disable_for_main_menu_owner_boundary\("world-identity-probe"\)[\s\S]*?return') `
    'TitleMap must hard-stop before either renderer can begin, while the existing bounded identity probe remains the callback-miss fallback.'
Assert-True ($disableForMainMenu -match `
        'main_menu_activation_latched_\s*=\s*true[\s\S]*?disable\(\)' `
    -and $disableForMainMenu -match `
        'visibility_hub_\.detach\(\)[\s\S]*?compact_umg_renderer_\.detach\(\)[\s\S]*?world_map_umg_renderer_\.detach\(\)' `
    -and $disableForMainMenu -match `
        'clear_world_map_listener_candidate\(\)[\s\S]*?clear_compact_listener_candidate\(\)[\s\S]*?clear_created_encounter_candidates\(\)[\s\S]*?clear_bird_egg_candidates\(\)' `
    -and $disableForMainMenu -match `
        'runtime_opened_treasure_ids_\.clear\(\)[\s\S]*?encounter_next_available_unix_seconds_\.clear\(\)' `
    -and $disableForMainMenu -match `
        'pending_encounter_death_mask_\.store\(0,[\s\S]*?pending_encounter_death_process_mask_\.store\([\s\S]*?0,' `
    -and $disableForMainMenu -match `
        'save_reconcile_requested_\s*=\s*false[\s\S]*?save_reconcile_inflight_request_id_\s*=\s*0' `
    -and $disableForMainMenu -match `
        'reset_area_quest_task_class_runtime\(false\)[\s\S]*?area_quest_state_ready_\s*=\s*false' `
    -and $disableForMainMenu -match `
        'baseline_world_key_\.clear\(\)[\s\S]*?baseline_context_key_\.clear\(\)[\s\S]*?current_world_key_\.clear\(\)[\s\S]*?current_context_key_\.clear\(\)' `
    -and $disableForMainMenu -match `
        'f6_requests_\.store\(0,\s*std::memory_order_release\)' `
    -and $disableForMainMenu -match 'MAIN_MENU_DISABLED' `
    -and $disableForMainMenu -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|ProcessEvent|sleep_for|while\s*\(') `
    'TitleMap must perform one bounded hard stop that clears every previous-save mutable owner while retaining only immutable catalogs, configuration, and the create listener.'
Assert-True ($engineTickUnsafe -match `
        'if\s*\(main_menu_activation_latched_\s*\|\|\s*!enabled_\)[\s\S]*?capture_current_world_identity_guarded\(\s*engine,\s*&current_world,\s*&current_world_object\)[\s\S]*?if\s*\(!is_open_world_identity\(current_world\)\)[\s\S]*?F7_REJECTED[\s\S]*?else\s*\{[\s\S]*?main_menu_activation_latched_\s*=\s*false[\s\S]*?baseline_world_key_\.clear\(\)[\s\S]*?activate\(engine\)' `
    -and $captureCurrentWorldIdentity -match `
        'capture_current_world_identity_unsafe\(\s*engine,\s*output,\s*world_output\)[\s\S]*?return\s*!output->empty\(\)' `
    -and $serviceEngineTickFault -match `
        'const bool recover\s*=\s*was_enabled[\s\S]*?&&\s*!main_menu_activation_latched_[\s\S]*?engine_tick_fault_recovery_attempts_\s*==\s*0' `
    -and $mainCode -match 'bool\s+main_menu_activation_latched_') `
    'Only explicit F7 after a freshly verified loaded open world may clear the TitleMap latch; automatic fault recovery must never bypass it.'
Assert-True ($mainCode -match `
        'kQuestEventTriggerFunction\s*=\s*STR\("/Script/DS\.DETUtil:ETSendQuestEventTrigger"\)' `
    -and $mainCode -match 'kQuestEventParameterCapacity\s*=\s*64' `
    -and $initializeQuestEvent -match `
        'StaticFindObject<UFunction\*>\(\s*nullptr,\s*nullptr,\s*kQuestEventTriggerFunction\)' `
    -and $initializeQuestEvent -match `
        'HasAnyFunctionFlags\(\s*FUNC_Native\)' `
    -and $initializeQuestEvent -match `
        'parameter_bytes\s*==\s*0[\s\S]*?parameter_bytes\s*>\s*kQuestEventParameterCapacity' `
    -and $initializeQuestEvent -match `
        'property->GetOffset_Internal\(\)\s*>=\s*0[\s\S]*?property->GetSize\(\)\s*>\s*0[\s\S]*?property->GetOffset_Internal\(\)[\s\S]*?\+\s*property->GetSize\(\)\)\s*<=\s*parameter_bytes' `
    -and $initializeQuestEvent -match `
        'quest_event_id_property_->IsInteger\(\)' `
    -and $initializeQuestEvent -match `
        'quest_event_step_id_property_->IsInteger\(\)' `
    -and $initializeQuestEvent -match `
        'quest_event_step_count_property_->IsInteger\(\)' `
    -and $initializeQuestEvent -match 'AREA_QUEST_EVENT_HOOK_READY') `
    'The native dynamic-quest event provider is not exact, native-only, bounded, and schema validated.'
foreach ($requiredQuestEventProperty in @(
        'WorldContextObject', 'QuestID', 'StepID', 'StepCnt', 'IsSet',
        'IsDynamic')) {
    Assert-True ($initializeQuestEvent -match `
            [regex]::Escape('L"' + $requiredQuestEventProperty + '"')) `
        "The native quest-event schema is missing: $requiredQuestEventProperty"
}
Assert-True ([regex]::Matches(
        $onUnrealInit,
        'RegisterHook\(\s*quest_event_trigger_function_').Count -eq 1 `
    -and [regex]::Matches(
        $mainCode,
        'RegisterHook\(\s*quest_event_trigger_function_').Count -eq 1 `
    -and $onUnrealInit -match `
        'quest_event_trigger_schema_ready_[\s\S]*?quest_event_trigger_pre\(context\)' `
    -and [regex]::Matches(
        $unregisterCallbacks,
        'UnregisterHook\(\s*quest_event_trigger_function_').Count -eq 1 `
    -and [regex]::Matches(
        $mainCode,
        'UnregisterHook\(\s*quest_event_trigger_function_').Count -eq 1 `
    -and $unregisterCallbacks -match `
        'quest_event_trigger_hook_registered_\s*=\s*false' `
    -and $mainCode -notmatch `
        'TFieldRange<UFunction>|GetFunctionByName' `
    -and $mainCode -match `
        'area_quest_completion=exact_catalog_dynamic_event_or_exact_task_actor_then_ten_second_exact_id_end_probe_then_three_bounded_positive_only_save_attempts') `
    'The one exact native quest-event hook is not registered and released symmetrically, or a per-Blueprint discovery path returned.'
Assert-True ($questEventPre -match `
        '!enabled_[\s\S]*?transition_active_[\s\S]*?!game_thread\(\)[\s\S]*?!quest_event_trigger_schema_ready_' `
    -and $questEventPreUnsafe -match 'context\.TheStack\.Locals\(\)' `
    -and $questEventPreUnsafe -match `
        'quest_event_dynamic_property_->GetPropertyValue\([\s\S]*?dynamic_value\)' `
    -and $questEventPreUnsafe -match `
        'area_quest_catalog_index\(quest_id\)' `
    -and $questEventPreUnsafe -match `
        'arm_area_quest_completion_witness\(\s*\*index,\s*true,\s*"native_dynamic_event_exact_catalog_id"' `
    -and $questEventPreUnsafe -match `
        'progress_precondition=not_required[\s\S]*?completion_policy=exact_id_end_only' `
    -and $questEventPreUnsafe -notmatch `
        'query_area_quest_state_guarded|AreaQuestState::Progress|progress_proven' `
    -and $questEventPreUnsafe -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|GetFunctionByName|std::this_thread::sleep|filesystem|fstream|std::vector|\bnew\b') `
    'The quest-event callback must arm only one exact dynamic catalog ID without a lossy PROGRESS precondition or forbidden discovery/blocking work.'
Assert-True ($resolveTreasureActorId.Length -gt 0 `
    -and $mainCode -match `
        'kTreasureInteractFunction\s*=\s*STR\("/Script/DS\.DsAnimationProp:NetMultiExecuteInteractProp"\)' `
    -and $mainCode -match `
        'kTreasureDeathFunction\s*=\s*STR\("/Script/DS\.DsAnimationProp:SetDeathProcess"\)' `
    -and $mainCode -notmatch `
        'Server_RunInteractV2|treasure_actor_from_interaction_target|unique_nearby_id|ExecuteTargetObject|ExecuteTargetComponent' `
    -and $onUnrealInit -match `
        'StaticFindObject<UFunction\*>\([\s\S]*?kTreasureInteractFunction' `
    -and $onUnrealInit -match `
        'StaticFindObject<UFunction\*>\([\s\S]*?kTreasureDeathFunction' `
    -and [regex]::Matches(
        $mainCode,
        'RegisterHook\(\s*treasure_interact_function_').Count -eq 1 `
    -and [regex]::Matches(
        $mainCode,
        'RegisterHook\(\s*treasure_death_function_').Count -eq 1 `
    -and $onUnrealInit -match `
        'treasure_interact_function_->HasAnyFunctionFlags\(FUNC_Native\)' `
    -and $onUnrealInit -match `
        'treasure_death_function_->HasAnyFunctionFlags\(FUNC_Native\)' `
    -and $onUnrealInit -match `
        'treasure_interact_pre\(context\)' `
    -and $onUnrealInit -match `
        'treasure_death_pre\(context\)' `
    -and [regex]::Matches(
        $unregisterCallbacks,
        'UnregisterHook\(\s*treasure_interact_function_').Count -eq 1 `
    -and [regex]::Matches(
        $unregisterCallbacks,
        'UnregisterHook\(\s*treasure_death_function_').Count -eq 1 `
    -and $mainCode -match `
        'receiver=exact_ds_animation_prop_actor' `
    -and $mainCode -match `
        'treasure_completion=local_interactor_or_exact_current_mount_rider_exact_receiver_or_nearby_nonpawn_exact_id_delayed_positive_save_confirmation_or_set_death_process') `
    'Treasure completion must use only the two exact native DsAnimationProp receiver hooks with symmetric release.'
Assert-True ($mainCode -match `
        'kTreasureInteractParameterCapacity\s*=\s*16' `
    -and $initializeTreasureInteract -match `
        'GetParmsSize\(\)' `
    -and $initializeTreasureInteract -match `
        'CastField<FObjectPropertyBase>\([\s\S]*?L"InActor"' `
    -and $initializeTreasureInteract -match `
        'parameter_bytes\s*>\s*0[\s\S]*?parameter_bytes\s*<=\s*kTreasureInteractParameterCapacity' `
    -and $initializeTreasureInteract -match `
        'GetOffset_Internal\(\)\s*>=\s*0[\s\S]*?GetSize\(\)\s*>\s*0[\s\S]*?<=\s*parameter_bytes' `
    -and $treasureInteractPre -match `
        '!enabled_[\s\S]*?transition_active_[\s\S]*?!game_thread\(\)[\s\S]*?!context\.Context[\s\S]*?!treasure_interact_schema_ready_[\s\S]*?!engine_tick_engine_' `
    -and $treasureInteractPreUnsafe -match `
        'context\.TheStack\.Locals\(\)' `
    -and $treasureInteractPreUnsafe -match `
        'current_player_pawn\(engine_tick_engine_\)' `
    -and $treasureInteractPreUnsafe -match `
        '!interacting_actor\s*\|\|\s*!local_pawn' `
    -and $treasureInteractPreUnsafe -match `
        'interacting_actor\s*!=\s*local_pawn[\s\S]*?STR\("Rider"\)[\s\S]*?accept_mounted_treasure_interactor\(' `
    -and $treasureInteractPreUnsafe -match `
        'is_mount_treasure_class\(treasure_class_name\)[\s\S]*?interacting_actor\s*==\s*current_rider[\s\S]*?rider_world_matches[\s\S]*?exact_receiver_near_local_player' `
    -and $treasureInteractPreUnsafe -match `
        'distance_squared\(player_,\s*treasure_position\)[\s\S]*?kTreasureObservationRadius' `
    -and $treasureInteractPreUnsafe -match `
        'queue_rejected_treasure_interaction_confirmation\(\s*context\.Context,\s*interacting_actor,\s*local_pawn\)' `
    -and $treasureInteractPreUnsafe -match `
        'local_mounted_rider_net_multi_execute_interact_prop' `
    -and $treasureInteractPreUnsafe -match `
        'treasure_completion_event_unsafe\(\s*context\.Context' `
    -and $queueRejectedTreasureConfirmation -match `
        'interacting_actor\s*&&\s*treasure_actor\s*&&\s*position_valid_' `
    -and $queueRejectedTreasureConfirmation -match `
        'is_treasure_class\(class_name\)[\s\S]*?read_actor_position\(treasure_actor[\s\S]*?distance_squared\(player_,\s*position\)[\s\S]*?kTreasureObservationRadius' `
    -and $queueRejectedTreasureConfirmation -match `
        'resolve_treasure_actor_id\([\s\S]*?treasure_render_catalog_index\(save_id\)[\s\S]*?queue_treasure_save_confirmation\(' `
    -and $queueRejectedTreasureConfirmation -notmatch `
        'mark_treasure_opened|FindAllOf|FindFirstOf|StaticFindObject|ProcessEvent|std::this_thread::sleep' `
    -and $treasureDeathPre -match `
        '!enabled_[\s\S]*?transition_active_[\s\S]*?!game_thread\(\)[\s\S]*?!context\.Context' `
    -and $treasureDeathPre -match `
        'treasure_completion_event_unsafe\(\s*context\.Context,\s*"set_death_process"\)') `
    'NetMulti treasure evidence must keep fresh-Pawn or exact current mount-Rider immediate completion and route only any other nearby exact non-Pawn receiver to delayed positive save confirmation; SetDeathProcess must keep the exact receiver.'
$objectIdIndex = $resolveTreasureActorId.IndexOf(
    'STR("ObjectID")', [StringComparison]::Ordinal)
$reportedIdIndex = $resolveTreasureActorId.IndexOf(
    'tracker_.observe_reported_id(', [StringComparison]::Ordinal)
$observedIdentityIndex = $resolveTreasureActorId.IndexOf(
    'observed_objects_.find(identity.packed())', [StringComparison]::Ordinal)
$spatialFallbackIndex = $resolveTreasureActorId.IndexOf(
    'tracker_.observe(identity, class_name, position)', [StringComparison]::Ordinal)
Assert-True ($objectIdIndex -ge 0 `
    -and $reportedIdIndex -gt $objectIdIndex `
    -and $observedIdentityIndex -gt $reportedIdIndex `
    -and $spatialFallbackIndex -gt $observedIdentityIndex `
    -and $resolveTreasureActorId -match `
        'object_id_class_3d' `
    -and $resolveTreasureActorId -match `
        'unique_class_3d_fallback' `
    -and $treasureCompletionEvent -match `
        'is_treasure_class\(class_name\)[\s\S]*?read_actor_position\(actor,\s*&position\)' `
    -and $treasureCompletionEvent -match `
        'resolve_treasure_actor_id\([\s\S]*?actor,\s*class_name,\s*position,\s*identity' `
    -and $treasureCompletionEvent -match `
        'const bool visibility_changed\s*=\s*mark_treasure_opened\(\*id\)' `
    -and $treasureCompletionEvent -match `
        'if\s*\(visibility_changed\)[\s\S]*?TREASURE_OPENED_NATIVE[\s\S]*?else[\s\S]*?TREASURE_EVENT_NO_VISIBLE_TRANSITION' `
    -and ($initializeTreasureInteract + "`n" + $treasureInteractPre + "`n" +
        $treasureInteractPreUnsafe + "`n" + $treasureDeathPre + "`n" +
        $resolveTreasureActorId + "`n" + $treasureCompletionEvent) -notmatch `
        'FindAllOf|FindFirstOf|GetFunctionByName|UObjectArray|sqlite|sqlcipher|save_reconcil|filesystem|fstream|std::this_thread::sleep|\bnew\b' `
    -and $engineTickUnsafe -notmatch `
        'treasure_interact_pre|treasure_death_pre|resolve_treasure_actor_id') `
    'Treasure identity must prefer validated ObjectID plus exact class/3D, then observed identity and a bounded class/3D fallback, without recurring discovery.'
Assert-True ($objectState -match `
        'observe_reported_id' `
    -and $objectState -match `
        'xy_squared\s*<=\s*kMatchRadiusXY\s*\*\s*kMatchRadiusXY' `
    -and $objectState -match `
        'distance\s*=\s*xy_squared\s*\+\s*dz\s*\*\s*dz' `
    -and $nativeTests -match `
        'XY-overlapping treasure actors must resolve by exact 3D position' `
    -and $nativeTests -match `
        'adjacent same-class treasure actors must retain distinct IDs' `
    -and $nativeTests -match `
        'runtime ObjectID class conflict must fail closed without spatial fallback' `
    -and $nativeTests -match `
        'duplicate reported treasure IDs must fail closed') `
    'Treasure catalog correlation must keep adjacent and XY-overlapping actors distinct and fail closed on invalid ObjectID evidence.'

Assert-True ($mainCode -match `
        'kAreaQuestCompletionWitnessWindow\s*=\s*std::chrono::seconds\{10\}' `
    -and $mainCode -match `
        'kAreaQuestCompletionProbeInterval\s*=\s*std::chrono::milliseconds\{750\}' `
    -and $mainCode -match `
        'kAreaQuestCompletionProbeBudgetPerTick\s*=\s*1' `
    -and $mainCode -match `
        'std::array<bool,\s*kExpectedAreaQuestCount>\s*area_quest_completion_witnesses_' `
    -and $mainCode -match `
        'std::array<bool,\s*kExpectedAreaQuestCount>\s*area_quest_completion_witness_queued_' `
    -and $mainCode -match `
        'std::array<Clock::time_point,\s*kExpectedAreaQuestCount>\s*area_quest_completion_witness_deadlines_' `
    -and $mainCode -match `
        'std::array<Clock::time_point,\s*kExpectedAreaQuestCount>\s*area_quest_completion_probe_after_' `
    -and $mainCode -match `
        'std::array<std::size_t,\s*kExpectedAreaQuestCount>\s*area_quest_completion_witness_queue_' `
    -and $armCompletionWitness -match `
        '!exact_identity_proven[\s\S]*?exact_index\s*>=\s*area_quest_catalog_\.size\(\)' `
    -and $armCompletionWitness -match `
        'area_quest_completion_witnesses_\[exact_index\][\s\S]*?now\s*<=\s*area_quest_completion_witness_deadlines_\[exact_index\][\s\S]*?return\s+true' `
    -and $armCompletionWitness -match `
        'now\s*\+\s*kAreaQuestCompletionWitnessWindow' `
    -and $armCompletionWitness -match `
        'now\s*\+\s*kAreaQuestCompletionProbeInterval' `
    -and $armCompletionWitness -match `
        'area_quest_completion_witness_queued_\[exact_index\][\s\S]*?return\s+true' `
    -and $armCompletionWitness -match `
        'area_quest_completion_witness_queue_count_[\s\S]*?>=\s*area_quest_completion_witness_queue_\.size\(\)' `
    -and $armCompletionWitness -match `
        'area_quest_completion_witness_queue_\[[\s\S]*?area_quest_completion_witness_queue_count_\+\+\]\s*=\s*exact_index' `
    -and $armCompletionWitness -match `
        'area_quest_completion_witness_queued_\[exact_index\]\s*=\s*true' `
    -and [regex]::Matches(
        $armCompletionWitness,
        'area_quest_rescan_requests_\.fetch_add\(').Count -eq 1 `
    -and $armCompletionWitness -notmatch `
        '\bfor\s*\(|\bwhile\s*\(') `
    'Completion witnesses must be exact per-task ten-second state in one duplicate-proof fixed queue with a 750 ms probe schedule.'
Assert-True ($processOneAreaQuest -match `
        'Clock::now\(\)\s*<=\s*area_quest_completion_witness_deadlines_\[index\]' `
    -and $processOneAreaQuest -match `
        'area_quest_witnessed_completion_transition\(\s*area_quest_completion_witnesses_\[index\]' `
    -and $processOneAreaQuest -match `
        'area_quest_completion_witnesses_\[index\]\s*=\s*false[\s\S]*?area_quest_completion_witness_deadlines_\[index\]\s*=\s*\{\}[\s\S]*?area_quest_completion_probe_after_\[index\]\s*=\s*\{\}[\s\S]*?area_quest_completion_probe_counts_\[index\]\s*=\s*0' `
    -and $areaQuestVisibility -match `
        'area_quest_witnessed_completion_transition[\s\S]*?exact_quest_identity_witnessed[\s\S]*?current_state\s*==\s*AreaQuestState::End' `
    -and $nativeTests -match `
        'an exact quest identity may accept end over a stale snapshot' `
    -and $nativeTests -match `
        'witnessed completion must require exact identity and end') `
    'Only an exact witnessed task may accept a bounded NONE or stale-state to END completion transition.'
Assert-True ($serviceCompletionWitnesses -match `
        'area_quest_completion_witness_queue_count_\s*==\s*0[\s\S]*?activity_suppressed_[\s\S]*?!area_quest_state_provider_ready_' `
    -and $serviceCompletionWitnesses -match `
        'current_game_instance\(engine\)[\s\S]*?if\s*\(!world_context\)[\s\S]*?return' `
    -and $serviceCompletionWitnesses -match `
        'while\s*\(cursor\s*<\s*area_quest_completion_witness_queue_count_\)' `
    -and $serviceCompletionWitnesses -match `
        'area_quest_completion_witness_queued_\[index\]\s*=\s*false' `
    -and $serviceCompletionWitnesses -match `
        'queries_this_tick\s*<\s*kAreaQuestCompletionProbeBudgetPerTick' `
    -and [regex]::Matches(
        $serviceCompletionWitnesses,
        'query_area_quest_state_guarded\(').Count -eq 1 `
    -and $serviceCompletionWitnesses -match `
        'area_quest_catalog_\[index\]\.id' `
    -and $serviceCompletionWitnesses -match `
        'state\s*==\s*static_cast<std::int64_t>\(\s*dswros::AreaQuestState::End\)' `
    -and $serviceCompletionWitnesses -match `
        'area_quest_exact_completion_bits_\[word\]\.fetch_or\(' `
    -and $serviceCompletionWitnesses -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|std::vector|\bnew\b' `
    -and $resetTaskClassRuntime -match `
        'area_quest_completion_witnesses_\.fill\(false\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_completion_witness_queued_\.fill\(false\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_completion_witness_deadlines_\.fill\(Clock::time_point\{\}\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_completion_probe_after_\.fill\(Clock::time_point\{\}\)' `
    -and $resetTaskClassRuntime -match `
        'area_quest_completion_witness_queue_count_\s*=\s*0') `
    'The exact-ID witness service must spend at most one query per tick, accept only END, preserve queue membership, and clear all fixed state on lifecycle reset.'

$updateCompactPool = Get-MainFunction $mainCode 'update_compact_pool'
$captureCompactLayer = Get-MainFunction `
    $mainCode 'capture_created_compact_layer_guarded'
$consumeCompactLayer = Get-MainFunction `
    $mainCode 'consume_compact_listener_candidate'
$consumeCompactLayerGuarded = Get-MainFunction `
    $mainCode 'consume_compact_listener_candidate_guarded'
$consumeCompactLayerUnsafe = Get-MainFunction `
    $mainCode 'consume_compact_listener_candidate_unsafe'
$clearCompactLayer = Get-MainFunction `
    $mainCode 'clear_compact_listener_candidate'
$notifyObjectCreated = Get-MainFunction $mainCode 'NotifyUObjectCreated'
Assert-True ($mainCode -match `
        'kCompactMaxAttachAttempts\s*=\s*3U' `
    -and $mainCode -match `
        'kCompactAttachRetryDelay\s*=\s*std::chrono::milliseconds\{250\}' `
    -and [regex]::Matches(
        $updateCompactPool,
        '\+\+compact_attach_attempt_count_').Count -eq 1 `
    -and [regex]::Matches(
        $updateCompactPool,
        'compact_umg_renderer_\.attach_once\(').Count -eq 1 `
    -and $updateCompactPool -match `
        'compact_attach_attempt_count_\s*<\s*kCompactMaxAttachAttempts' `
    -and $updateCompactPool -match `
        'compact_attach_requested_\s*=\s*!retry' `
    -and $updateCompactPool -match `
        'failure\s*==\s*1U[\s\S]*?failure\s*==\s*18U' `
    -and $updateCompactPool -match `
        'soft_not_ready[\s\S]*?&&\s*compact_attach_attempt_count_[\s\S]*?<\s*kCompactMaxAttachAttempts' `
    -and $updateCompactPool -match `
        'now\s*\+\s*kCompactAttachRetryDelay' `
    -and $updateCompactPool -match `
        'compact_candidate_available_[\s\S]*?compact_layer_candidate_' `
    -and $updateCompactPool -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|std::this_thread::sleep|Sleep\(') `
    'Compact attachment recovery must remain one bounded three-attempt soft-not-ready transaction.'
Assert-True ($mainCode -match `
        'kCompactLayerClass\s*=\s*STR\("/Script/DSClient\.DLayerMiniMap"\)' `
    -and $captureCompactLayer -match `
        'unreal_object->IsA\(self->compact_layer_class_\)' `
    -and $notifyObjectCreated -match `
        'capture_created_compact_layer_guarded\(' `
    -and $notifyObjectCreated -match `
        'scoped_lock\s+lock\{compact_listener_mutex_\}[\s\S]*?compact_listener_candidate_\s*=\s*compact_weak[\s\S]*?compact_listener_pending_\.store\(' `
    -and $engineTickUnsafe -match `
        'consume_compact_listener_candidate\(\)' `
    -and $consumeCompactLayerUnsafe -match `
        'compact_umg_renderer_\.begin_activation\(\)' `
    -and $consumeCompactLayerUnsafe -match `
        'reset_compact_pool_runtime\(\)' `
    -and $consumeCompactLayer -match `
        'scoped_lock\s+lock\{compact_listener_mutex_\}[\s\S]*?compact_listener_pending_\.exchange\([\s\S]*?weak\s*=\s*compact_listener_candidate_' `
    -and $consumeCompactLayer -match `
        'if\s*\(!compact_listener_pending_\.load\(std::memory_order_acquire\)\)\s*\{\s*return;\s*\}[\s\S]*?scoped_lock\s+lock\{compact_listener_mutex_\}' `
    -and $consumeCompactLayer -match `
        'compact_listener_candidate_(?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})[\s\S]*?compact_listener_object_index_\s*=\s*-1' `
    -and $consumeCompactLayerUnsafe -match `
        'expected_world\s*&&\s*layer->GetWorld\(\)\s*!=\s*expected_world' `
    -and $consumeCompactLayerUnsafe -notmatch `
        'state\s*!=\s*dsnwr::CompactUmgRendererState::(?:Attached|Suppressed)' `
    -and $consumeCompactLayer -notmatch '__try' `
    -and $consumeCompactLayerGuarded -match `
        '__try\s*\{[\s\S]*?consume_compact_listener_candidate_unsafe\([\s\S]*?__except' `
    -and $clearCompactLayer -match `
        'scoped_lock\s+lock\{compact_listener_mutex_\}[\s\S]*?compact_listener_candidate_(?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})[\s\S]*?compact_listener_pending_\.store\(' `
    -and ($consumeCompactLayer + $consumeCompactLayerGuarded + `
        $consumeCompactLayerUnsafe) -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|while\s*\(' `
    -and $transitionBegin -match `
        'compact_layer_candidate_(?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})[\s\S]*?compact_candidate_available_\s*=\s*false' `
    -and $transitionBegin -notmatch `
        'clear_compact_listener_candidate\(\)' `
    -and $transitionEndLifecycle -match `
        'object_world_guarded\([\s\S]*?consume_compact_listener_candidate\(current_world\)' `
    -and $transitionEndLifecycle -match `
        'else\s*\{[\s\S]*?clear_compact_listener_candidate\(\)' `
    -and $engineTickUnsafe -match `
        'if\s*\(!transition_active_\s*&&\s*!main_menu_activation_latched_\)\s*\{[\s\S]*?consume_compact_listener_candidate\(\)') `
    'DLayerMiniMap creation must use one no-event atomic fast path plus a mutex-protected weak mailbox, preserve an early new-world event until exact world validation, and rearm once for every distinct replacement identity without polling.'
$runtimeVisibilityService = Get-MainFunction `
    $mainCode 'service_runtime_visibility_edges'
$recomputeCooldownEdge = Get-MainFunction `
    $mainCode 'recompute_next_encounter_cooldown_edge'
$establishVisibilityBaseline = Get-MainFunction `
    $mainCode 'establish_encounter_visibility_baseline'
$initializeWidgetIsVisible = Get-MainFunction `
    $mainCode 'initialize_widget_is_visible_schema'
$worldMapLayerVisible = Get-MainFunction `
    $mainCode 'world_map_layer_visible_guarded'
$refreshAtlasForRuntimeDelta = Get-MainFunction `
    $mainCode 'refresh_world_map_atlas_for_runtime_delta'
$worldMapImagePost = Get-MainFunction $mainCode 'world_map_image_post'
$captureWorldMapCandidate = Get-MainFunction `
    $mainCode 'capture_world_map_candidate_unsafe'
$resetWorldMapRuntime = Get-MainFunction `
    $mainCode 'reset_world_map_runtime'
$serviceWorldMapAtlas = Get-MainFunction `
    $mainCode 'service_world_map_atlas'
$encounterVisibilityMask = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?std::uint64_t\s+encounter_visibility_mask\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($encounterVisibilityMask.Length -gt 0 `
    -and $mainCode -match 'kExpectedEncounterCount\s*=\s*49' `
    -and $encounterVisibilityMask -match `
        'std::min\(\s*encounter_catalog_\.size\(\),\s*kExpectedEncounterCount\)' `
    -and $encounterVisibilityMask -match `
        'mask\s*\|=\s*std::uint64_t\{1\}\s*<<\s*index') `
    'Encounter visibility must remain a fixed 49-bit scalar mask.'
Assert-True ([regex]::Matches(
        $mainCode,
        'encounter_next_available_unix_seconds_\.clear\(\)').Count -eq 1 `
    -and $disableForMainMenu -match `
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
Assert-True ($engineTickUnsafe -match `
        'retryable_attach_failure\s*=[\s\S]*?CompactAttachGateStatus::CurrentPlayerControllerUnavailable[\s\S]*?CompactAttachGateStatus::RendererRejected[\s\S]*?CompactUmgRendererState::Faulted[\s\S]*?WorldMapUmgRendererState::Faulted[\s\S]*?area_quest_scan_faulted_' `
    -and $engineTickUnsafe -match `
        'if\s*\(enabled_\s*&&\s*!transition_active_\s*&&\s*!retryable_attach_failure\)') `
    'Explicit F7 must start one bounded activation for compact, world-map, or area-task scan faults instead of coalescing against stale state.'
Assert-True ($activitySuppressionEdge -match `
        'if\s*\(activity_suppressed_\)\s*\{[\s\S]*?compact_umg_renderer_\.detach\s*\(\s*\);\s*reset_compact_pool_runtime\s*\(\s*\)' `
    -and $activitySuppressionEdge -match `
        'else\s+if\s*\(enabled_\s*&&\s*!transition_active_\)\s*\{[\s\S]*?compact_umg_renderer_\.begin_activation\s*\(\s*\);\s*reset_compact_pool_runtime\s*\(\s*\)' `
    -and $activitySuppressionEdge -notmatch `
        'apply_compact_suppression\s*\(\s*\)') `
    'Activity-map edges must detach the old compact UMG tree and rearm a current-world host without first touching stale visibility.'
Assert-True ($renderer -match `
        '(?s)void\s+CompactUmgRenderer::begin_activation\s*\(\s*\)\s*noexcept\s*\{.*?faults_before_detach\s*=\s*fault_count_;.*?detach_guarded\s*\(\s*\);.*?state_\s*==\s*CompactUmgRendererState::Faulted.*?fault_count_\s*==\s*faults_before_detach.*?abi_failure_mask_\s*==\s*0.*?state_\s*=\s*CompactUmgRendererState::Ready') `
    'A prior runtime-only compact fault is not recoverable on one bounded reactivation after all weak handles are cleared.'
foreach ($forbiddenRuntimeEdgeWork in @(
        'FindAllOf', 'FindFirstOf', 'StaticFindObject', 'ProcessEvent',
        'UObject', 'read_player_position', 'service_world_map_atlas',
        'update_compact_pool', 'std::vector', 'new ')) {
    Assert-True ($runtimeVisibilityService -notmatch `
            [regex]::Escape($forbiddenRuntimeEdgeWork)) `
        "Runtime visibility edge service contains forbidden recurring work: $forbiddenRuntimeEdgeWork"
}
Assert-True ($mainCode -match `
        'kWidgetIsVisibleFunction\s*=\s*STR\("/Script/UMG\.Widget:IsVisible"\)' `
    -and $mainCode -match `
        'kWidgetIsVisibleParameterCapacity\s*=\s*16' `
    -and $onUnrealInit -match `
        'StaticFindObject<UFunction\*>\([\s\S]*?kWidgetIsVisibleFunction' `
    -and $initializeWidgetIsVisible -match `
        'GetParmsSize\(\)[\s\S]*?parameter_bytes\s*>\s*0[\s\S]*?parameter_bytes\s*<=\s*kWidgetIsVisibleParameterCapacity' `
    -and $initializeWidgetIsVisible -match `
        'CastField<FBoolProperty>\([\s\S]*?L"ReturnValue"' `
    -and $initializeWidgetIsVisible -match `
        'GetOffset_Internal\(\)\s*>=\s*0[\s\S]*?GetSize\(\)\s*>\s*0[\s\S]*?<=\s*parameter_bytes' `
    -and $worldMapLayerVisible -match `
        '!current_layer[\s\S]*?!widget_is_visible_schema_ready_[\s\S]*?!widget_is_visible_function_[\s\S]*?!widget_is_visible_return_property_' `
    -and $worldMapLayerVisible -match `
        'std::array<std::byte,[\s\S]*?kWidgetIsVisibleParameterCapacity>[\s\S]*?ProcessEvent\([\s\S]*?widget_is_visible_function_[\s\S]*?GetPropertyValue' `
    -and [regex]::Matches(
        $mainCode,
        'world_map_layer_visible_guarded\(').Count -eq 2) `
    'World-map visibility must use one ABI-validated reflected IsVisible read only at an explicit runtime-delta edge.'
Assert-True ($refreshAtlasForRuntimeDelta -match `
        'world_map_marker_snapshot_built_\s*=\s*false' `
    -and $refreshAtlasForRuntimeDelta -match `
        'current_layer\s*=\s*attached\s*&&\s*candidate_available[\s\S]*?current_world_map_layer_guarded\(\)' `
    -and $refreshAtlasForRuntimeDelta -match `
        'exact_attachment\s*=\s*current_layer[\s\S]*?attached_to\([\s\S]*?current_layer,[\s\S]*?map_id\(\)' `
    -and $refreshAtlasForRuntimeDelta -match `
        'visibly_open\s*=\s*exact_attachment[\s\S]*?world_map_layer_visible_guarded\(current_layer\)' `
    -and $refreshAtlasForRuntimeDelta -match `
        'if\s*\(attached\s*&&\s*visibly_open\)[\s\S]*?begin_activation\(\);[\s\S]*?reset_world_map_runtime\(true\)' `
    -and $refreshAtlasForRuntimeDelta -match `
        'else\s+if\s*\(attached\)[\s\S]*?begin_activation\(\);[\s\S]*?reset_world_map_runtime\(true\);[\s\S]*?world_map_session_pending_\s*=\s*false[\s\S]*?deferred_until_set_world_map_image' `
    -and $refreshAtlasForRuntimeDelta -match `
        'else\s+if\s*\(already_pending\)[\s\S]*?coalesced_pending_current_session' `
    -and $refreshAtlasForRuntimeDelta -notmatch `
        'collect_world_map_marker_snapshot|attach_once|service_world_map_atlas|FindFirstOf|FindAllOf' `
    -and $mainCode -match 'Clock::time_point\s+next_runtime_visibility_edge_probe_' `
    -and $mainCode -match 'std::int64_t\s+next_encounter_cooldown_edge_unix_seconds_' `
    -and $mainCode -match 'std::uint64_t\s+encounter_visibility_mask_' `
    -and $mainCode -match 'bool\s+encounter_visibility_mask_valid_') `
    'A runtime delta must rebuild only an exact visible current session and defer a hidden retained layer until SetWorldMapImage.'
Assert-True ($worldMapImagePost -match `
        '!game_thread\(\)[\s\S]*?!context\.Context[\s\S]*?!required_runtime_ready_\.load\(std::memory_order_acquire\)[\s\S]*?shutting_down_\.load\(std::memory_order_acquire\)' `
    -and $worldMapImagePost -match `
        'capture_world_map_candidate_unsafe\(context\.Context,\s*true\)' `
    -and $captureWorldMapCandidate -match `
        'renderer_ready_for_candidate_rearm\s*=[\s\S]*?WorldMapUmgRendererState::Ready[\s\S]*?\|\|[\s\S]*?WorldMapUmgRendererState::Attached[\s\S]*?!world_map_umg_renderer_\.attached_layer_matches\s*\(\s*current_layer\s*\)' `
    -and $captureWorldMapCandidate -match `
        'same_layer[\s\S]*?set_world_map_image_event[\s\S]*?enabled_[\s\S]*?!transition_active_[\s\S]*?!activity_suppressed_[\s\S]*?!world_map_set_image_rearm_consumed_[\s\S]*?world_map_serviced_serial_\s*==\s*world_map_candidate_serial_[\s\S]*?world_map_service_attempts_\s*>\s*0[\s\S]*?renderer_ready_for_candidate_rearm[\s\S]*?world_map_session_pending_[\s\S]*?retryable_not_ready' `
    -and $captureWorldMapCandidate -match `
        'retryable_not_ready\(\)[\s\S]*?world_map_service_attempts_[\s\S]*?>=\s*kWorldMapMaxServiceAttempts' `
    -and $captureWorldMapCandidate -match `
        'if\s*\(set_image_rearm_allowed\)[\s\S]*?begin_activation\(\);[\s\S]*?reset_world_map_runtime\(true\);[\s\S]*?world_map_set_image_rearm_consumed_\s*=\s*true;[\s\S]*?return' `
    -and $captureWorldMapCandidate -match `
        'if\s*\(same_layer[\s\S]*?world_map_session_pending_[\s\S]*?WorldMapUmgRendererState::Attached[\s\S]*?retry_budget_consumed[\s\S]*?return' `
    -and $captureWorldMapCandidate -match `
        'world_map_set_image_rearm_consumed_\s*=\s*false[\s\S]*?world_map_layer_candidate_\s*=\s*current_layer[\s\S]*?world_map_candidate_available_\s*=\s*true[\s\S]*?\+\+world_map_candidate_serial_[\s\S]*?world_map_service_retry_after_\s*=\s*\{\}[\s\S]*?world_map_session_pending_\s*=\s*true' `
    -and $resetWorldMapRuntime -match `
        'if\s*\(!preserve_candidate\)[\s\S]*?world_map_set_image_rearm_consumed_\s*=\s*false[\s\S]*?\+\+world_map_candidate_serial_' `
    -and $serviceWorldMapAtlas -match `
        'world_map_serviced_serial_\s*!=\s*world_map_candidate_serial_[\s\S]*?world_map_serviced_serial_\s*=\s*world_map_candidate_serial_[\s\S]*?world_map_service_attempts_\s*=\s*0' `
    -and $mainCode -match `
        'world_map_readiness=set_world_map_image_one_shot_serial_matched_budget_rearm') `
    'SetWorldMapImage must provide one serial-matched readiness rearm without replenishing an exhausted same-layer budget.'
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
    'Treasure and mini-game completion must reject duplicate events before matching, dirty only real visibility deltas, and use the exact-session world-map refresh owner.'
$captureEncounterCandidate = Get-MainFunction `
    $mainCode 'capture_created_encounter_actor_unsafe'
$consumeEncounterCandidates = Get-MainFunction `
    $mainCode 'consume_created_encounter_candidates'
$markEncounterDefeated = Get-MainFunction `
    $mainCode 'mark_encounter_defeated'
$applyEncounterDefeatState = Get-MainFunction `
    $mainCode 'apply_encounter_defeat_state'
$probeObservedObjects = Get-MainFunction `
    $mainCode 'probe_observed_objects'
$encounterDeathPre = Get-MainFunction `
    $mainCode 'encounter_death_pre'
$encounterDeathPreUnsafe = Get-MainFunction `
    $mainCode 'encounter_death_pre_unsafe'
$encounterDeathProcessPre = Get-MainFunction `
    $mainCode 'encounter_death_process_pre'
$encounterDeathProcessPreUnsafe = Get-MainFunction `
    $mainCode 'encounter_death_process_pre_unsafe'
$consumePendingEncounterDeaths = Get-MainFunction `
    $mainCode 'consume_pending_encounter_deaths'
$applyActivitySuppressionEdge = Get-MainFunction `
    $mainCode 'apply_activity_suppression_edge'
$clearActivityEncounterObservations = Get-MainFunction `
    $mainCode 'clear_encounter_observations_for_activity_suppression'
$activateLifecycle = Get-MainFunction $mainCode 'activate'
$disableLifecycle = Get-MainFunction $mainCode 'disable'
$engineTickGuarded = Get-MainFunction $mainCode 'engine_tick'
$uobjectArrayShutdown = Get-MainFunction $mainCode 'OnUObjectArrayShutdown'
$requiredRuntimeExpression = [regex]::Match(
    $onUnrealInit,
    '(?ms)const bool required_runtime_ready\s*=\s*(?<expression>.*?);').Groups[
        'expression'].Value
Assert-True ($requiredRuntimeExpression.Length -gt 0 `
    -and $requiredRuntimeExpression -notmatch `
        'encounter_death|monster_character_class_') `
    'The optional encounter-death hook must not become a global runtime-readiness dependency.'
Assert-True ($mainCode -match `
        'kEncounterDeathFunction\s*=\s*STR\("/Script/DS\.DsFieldCharacter:NetMulticastNotifyDeath"\)' `
    -and [regex]::Matches(
        $mainCode,
        'RegisterHook\(\s*encounter_death_function_').Count -eq 1 `
    -and [regex]::Matches(
        $onUnrealInit,
        'RegisterHook\(\s*encounter_death_function_').Count -eq 1 `
    -and $onUnrealInit -match `
        'encounter_death_function_->HasAnyFunctionFlags\(FUNC_Native\)' `
    -and $onUnrealInit -match `
        'encounter_death_pre\(context\)' `
    -and $onUnrealInit -match 'ENCOUNTER_DEATH_HOOK_READY' `
    -and $onUnrealInit -match 'ENCOUNTER_DEATH_HOOK_DISABLED' `
    -and [regex]::Matches(
        $mainCode,
        'UnregisterHook\(\s*encounter_death_function_').Count -eq 1 `
    -and [regex]::Matches(
        $unregisterCallbacks,
        'UnregisterHook\(\s*encounter_death_function_').Count -eq 1 `
    -and $unregisterCallbacks -match `
        'encounter_death_hook_registered_\s*=\s*false') `
    'The exact native encounter-death hook must be registered once, reported independently, and released symmetrically.'
Assert-True ($mainCode -match `
        'kEncounterDeathProcessFunction\s*=\s*STR\("/Script/DS\.DsFieldCharacter:NetMulticastSetDeathProcess"\)' `
    -and [regex]::Matches(
        $mainCode,
        'RegisterHook\(\s*encounter_death_process_function_').Count -eq 1 `
    -and $onUnrealInit -match `
        'encounter_death_process_function_->HasAnyFunctionFlags\([\s\S]*?FUNC_Native' `
    -and $onUnrealInit -match 'encounter_death_process_pre\(context\)' `
    -and $onUnrealInit -match 'ENCOUNTER_DEATH_PROCESS_HOOK_READY' `
    -and $onUnrealInit -match 'ENCOUNTER_DEATH_PROCESS_HOOK_DISABLED' `
    -and [regex]::Matches(
        $unregisterCallbacks,
        'UnregisterHook\(\s*encounter_death_process_function_').Count -eq 1 `
    -and $unregisterCallbacks -match `
        'encounter_death_process_hook_registered_\s*=\s*false' `
    -and $encounterDeathProcessPreUnsafe -match `
        'encounter_death_process_is_terminal\([\s\S]*?GetSignedIntPropertyValue' `
    -and $encounterDeathProcessPreUnsafe -match `
        'encounter_death_pre_unsafe\(context\.Context,\s*true\)') `
    'The exact native encounter death-process End hook must be schema-validated, registered once, and released symmetrically.'
$encounterDeathCallback = $encounterDeathPre + "`n" +
    $encounterDeathPreUnsafe
Assert-True ($encounterDeathPre -match `
        '!enabled_[\s\S]*?transition_active_[\s\S]*?!game_thread\(\)[\s\S]*?!context\.Context[\s\S]*?activity_suppressed_[\s\S]*?!position_valid_[\s\S]*?!encounter_state_ready_[\s\S]*?!monster_character_class_' `
    -and $encounterDeathPre -match `
        'encounter_death_pre_unsafe\(context\.Context,\s*false\)' `
    -and $encounterDeathPreUnsafe -match `
        'FWeakObjectPtr\s+weak(?:\{actor\}|\{\}[\s\S]*?weak\s*=\s*actor)[\s\S]*?WeakIdentity\s+identity[\s\S]*?identity\.valid\(\)' `
    -and $encounterDeathPreUnsafe -match `
        'observed_objects_\.find\(identity\.packed\(\)\)' `
    -and $encounterDeathPreUnsafe -match `
        'EventKind::EncounterDefeated[\s\S]*?observed\.activation\s*==\s*activation_[\s\S]*?observed\.epoch\s*==\s*epoch_[\s\S]*?!observed\.logical_end[\s\S]*?observed\.weak\.Get\(\)\s*==\s*actor' `
    -and $encounterDeathPreUnsafe -match `
        'actor->IsA\(monster_character_class_\)' `
    -and $encounterDeathPreUnsafe -match `
        '(?:object_class->GetFName\(\)\.ToUnstableInt\(\)|DSNWRPR_CLASS_NAME_KEY\(object_class\))' `
    -and $encounterDeathPreUnsafe -match `
        'std::find\([\s\S]*?encounter_class_name_keys_' `
    -and $encounterDeathPreUnsafe -match `
        'encounter_index\s*<\s*encounter_catalog_\.size\(\)[\s\S]*?encounter_catalog_\[encounter_index\]\.id\s*==\s*observed\.id' `
    -and $encounterDeathPreUnsafe -match `
        'read_actor_position\(actor,\s*&actor_position\)' `
    -and $encounterDeathPreUnsafe -match `
        'distance_squared\(player_,\s*actor_position\)[\s\S]*?<=\s*kEncounterObservationRadius[\s\S]*?\*\s*kEncounterObservationRadius' `
    -and $encounterDeathPreUnsafe -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $encounterDeathPreUnsafe -match `
        'observed\.visible_seen' `
    -and $encounterDeathPreUnsafe -match `
        'accept_encounter_death_notification\(evidence\)' `
    -and $encounterDeathPreUnsafe -match `
        'pending_mask\.fetch_or\(\s*1ULL\s*<<\s*encounter_index,\s*std::memory_order_release\)' `
    -and $mainCode -match `
        'std::atomic<std::uint64_t>\s+pending_encounter_death_mask_' `
    -and $mainCode -match `
        'std::atomic<std::uint64_t>\s+pending_encounter_death_process_mask_' `
    -and $mainCode -match `
        'kExpectedEncounterCount\s*=\s*49') `
    'Encounter death notification must publish one fixed catalog bit only after exact weak identity, class, lifecycle, availability, visibility, and 100-meter evidence.'
Assert-True ($encounterDeathCallback -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|GetFunctionByName|UObjectArray|sqlite|sqlcipher|\bSQL\b|save_reconcil|filesystem|fstream|std::this_thread::sleep|Sleep\s*\(|std::vector|\bnew\b|collect_world_map_marker_snapshot|service_world_map_atlas|refresh_world_map_atlas|attach_once|mark_encounter_defeated|compact_rebind_dirty_|world_map_umg_renderer_|compact_umg_renderer_' `
    -and $encounterDeathCallback -notmatch `
        'TheStack\.Locals\(\)|GetPropertyValue|GetObjectPropertyValue') `
    'The encounter-death callback must not parse parameters, enumerate objects, query saves/files, block, allocate a dynamic scan, or mutate render/atlas state.'
$encounterDeathLifecycleGuardPosition =
    $consumePendingEncounterDeaths.IndexOf(
        'can_consume_pending_encounter_death')
$encounterDeathLoadPosition =
    $consumePendingEncounterDeaths.IndexOf(
        'pending_encounter_death_mask_.load')
Assert-True ($consumePendingEncounterDeaths -match `
        'bool\s+lifecycle_boundary\s*=\s*false\)\s+noexcept' `
    -and $consumePendingEncounterDeaths -match `
        'PendingEncounterDeathConsumptionContext\s+context[\s\S]*?lifecycle_boundary[\s\S]*?enabled_[\s\S]*?encounter_state_ready_[\s\S]*?transition_active_[\s\S]*?activity_suppressed_[\s\S]*?position_valid_' `
    -and $consumePendingEncounterDeaths -match `
        'can_consume_pending_encounter_death\(context\)[\s\S]*?return' `
    -and $consumePendingEncounterDeaths -match `
        'pending_encounter_death_mask_\.load\(std::memory_order_acquire\)' `
    -and $consumePendingEncounterDeaths -notmatch `
        'pending_encounter_death_mask_\.exchange' `
    -and $consumePendingEncounterDeaths -match `
        'index\s*<\s*encounter_catalog_\.size\(\)[\s\S]*?index\s*<\s*64U' `
    -and $consumePendingEncounterDeaths -match `
        'apply_encounter_defeat_state\([\s\S]*?spec,[\s\S]*?lifecycle_boundary,[\s\S]*?evidence' `
    -and $consumePendingEncounterDeaths -match `
        'pending_encounter_death_mask_\.fetch_and\([\s\S]*?~bit,[\s\S]*?std::memory_order_acq_rel\)' `
    -and $consumePendingEncounterDeaths -match `
        'pending_encounter_death_process_mask_\.fetch_and\([\s\S]*?~bit,[\s\S]*?std::memory_order_acq_rel\)' `
    -and $consumePendingEncounterDeaths -match `
        'ENCOUNTER_DEATH_HANDOFF_RETAINED[\s\S]*?state_apply_exception' `
    -and $consumePendingEncounterDeaths -match `
        'ENCOUNTER_DEFEATED_NATIVE[\s\S]*?evidence=\{\}' `
    -and $applyEncounterDefeatState -notmatch `
        'encounter_time_condition_matches' `
    -and $applyEncounterDefeatState -match `
        'encounter_cooldown_write_allowed[\s\S]*?encounter_next_available_unix_seconds_\[spec\.id\]' `
    -and $applyEncounterDefeatState -match `
        'if\s*\(!lifecycle_boundary\)[\s\S]*?refresh_world_map_atlas_for_runtime_delta' `
    -and $markEncounterDefeated -match `
        'encounter_time_condition_matches\(\*spec\)[\s\S]*?apply_encounter_defeat_state\([\s\S]*?\*spec,\s*false' `
    -and $mainCode -match `
        'kDiscoveryInterval\s*=\s*std::chrono::milliseconds\{250\}' `
    -and $engineTickUnsafe -match `
        'if\s*\(position_valid_\s*&&\s*now\s*>=\s*next_discovery_\)[\s\S]*?next_discovery_\s*=\s*now\s*\+\s*kDiscoveryInterval;[\s\S]*?consume_pending_encounter_deaths\(\);[\s\S]*?consume_created_encounter_candidates\(\);[\s\S]*?probe_observed_objects\(\)' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'consume_pending_encounter_deaths\(\)').Count -eq 1 `
    -and $encounterDeathLifecycleGuardPosition -ge 0 `
    -and $encounterDeathLoadPosition -gt `
        $encounterDeathLifecycleGuardPosition `
    -and [regex]::Matches(
        $mainCode,
        'consume_pending_encounter_deaths\(true\)').Count -eq 4 `
    -and $activateLifecycle -match `
        'if\s*\(enabled_\)[\s\S]*?consume_pending_encounter_deaths\(true\)' `
    -and $disableLifecycle -match `
        'consume_pending_encounter_deaths\(true\)[\s\S]*?enabled_\s*=\s*false' `
    -and $transitionBeginLifecycle -match `
        'consume_pending_encounter_deaths\(true\)[\s\S]*?transition_active_\s*=\s*true' `
    -and $applyActivitySuppressionEdge -match `
        'consume_pending_encounter_deaths\(true\)[\s\S]*?clear_encounter_observations_for_activity_suppression' `
    -and $engineTickGuarded -match `
        'engine_tick_fault_was_enabled_[\s\S]*?engine_tick_fault_pending_\s*=\s*true[\s\S]*?enabled_\s*=\s*false[\s\S]*?transition_active_\s*=\s*true' `
    -and $serviceEngineTickFault -match `
        'engine_tick_fault_cleanup_in_progress_\s*=\s*true;[\s\S]*?disable\(\)' `
    -and [regex]::Matches(
        $mainCode,
        'pending_encounter_death_mask_\.store\(0,\s*std::memory_order_release\)').Count -eq 3 `
    -and $shutdownOwner -match `
        'pending_encounter_death_mask_\.store\(0,\s*std::memory_order_release\)' `
    -and $uobjectArrayShutdown -match `
        'pending_encounter_death_mask_\.store\(0,\s*std::memory_order_release\)' `
    -and $disableForMainMenu -match `
        'pending_encounter_death_mask_\.store\(0,\s*std::memory_order_release\)' `
    -and $mainCode -notmatch 'ENCOUNTER_DEATH_EVENT_DROPPED') `
    'The fixed death mask must remain pending until the existing 250 ms owner or an authoritative numeric-only boundary can apply each bit, including after Engine Tick disables itself.'
Assert-True ($objectState -match `
        'struct\s+EncounterDeathNotificationEvidence\s*\{[\s\S]*?lifecycle_valid[\s\S]*?encounter_currently_available[\s\S]*?exact_observed_actor[\s\S]*?exact_catalog_class[\s\S]*?monster_character[\s\S]*?visible_seen[\s\S]*?player_near_actor[\s\S]*?\};' `
    -and $objectState -match `
        'constexpr\s+bool\s+accept_encounter_death_notification\([\s\S]*?evidence\.lifecycle_valid[\s\S]*?evidence\.encounter_currently_available[\s\S]*?evidence\.exact_observed_actor[\s\S]*?evidence\.exact_catalog_class[\s\S]*?evidence\.monster_character[\s\S]*?evidence\.visible_seen[\s\S]*?evidence\.player_near_actor' `
    -and $objectState -match `
        'struct\s+PendingEncounterDeathConsumptionContext\s*\{[\s\S]*?lifecycle_boundary[\s\S]*?enabled[\s\S]*?encounter_state_ready[\s\S]*?transition_active[\s\S]*?activity_suppressed[\s\S]*?player_position_valid' `
    -and $objectState -match `
        'can_consume_pending_encounter_death\([\s\S]*?context\.encounter_state_ready[\s\S]*?context\.lifecycle_boundary[\s\S]*?context\.enabled[\s\S]*?!context\.transition_active[\s\S]*?!context\.activity_suppressed[\s\S]*?context\.player_position_valid' `
    -and $nativeTests -match `
        'an exact observed nearby monster death notification must complete' `
    -and $nativeTests -match `
        'only the exact death-process End state may publish completion' `
    -and $nativeTests -match `
        'travel or suppression must reject a death notification' `
    -and $nativeTests -match `
        'an unavailable encounter must reject a duplicate death notification' `
    -and $nativeTests -match `
        'an unobserved receiver must not complete an encounter' `
    -and $nativeTests -match `
        'a non-catalog receiver class must not complete an encounter' `
    -and $nativeTests -match `
        'a non-monster receiver must not complete an encounter' `
    -and $nativeTests -match `
        'an unseen receiver must not complete an encounter' `
    -and $nativeTests -match `
        'a death outside the observed streaming context must fail closed' `
    -and $nativeTests -match `
        'an authoritative boundary must preserve a bit after Engine Tick disables itself' `
    -and $nativeTests -match `
        'an uninitialized encounter state must reject even boundary consumption') `
    'The exact encounter-death evidence and pending-handoff helpers or their positive and fail-closed unit cases are missing.'
Assert-True ($mainCode -notmatch 'IsMoribundState' `
    -and [regex]::Matches(
        $probeObservedObjects,
        'observed\.position\s*=\s*\*current_live_position').Count -eq 1 `
    -and $probeObservedObjects -match `
        'const bool live_position_inside\s*=[\s\S]*?distance_squared\(player_,\s*\*current_live_position\)[\s\S]*?<=\s*radius\s*\*\s*radius' `
    -and $probeObservedObjects -match `
        'if\s*\(live_position_inside\)\s*\{[\s\S]*?observed\.position\s*=\s*\*current_live_position;[\s\S]*?\}' `
    -and $probeObservedObjects -notmatch `
        'read_actor_position\([^;]+\)\s*\{\s*observed\.position\s*=') `
    'Production must not poll the unreliable moribund flag or overwrite the last trusted in-range encounter position after a far pool relocation.'
Assert-True ($mainCode -notmatch '\bFindAllOf\s*\(' `
    -and $mainCode -match `
        'kEncounterObservationRadius\s*=\s*10000\.0' `
    -and $mainCode -match `
        'kEncounterCandidateProbeBudgetPerControlTick\s*=\s*8' `
    -and $mainCode -match `
        'std::array<FWeakObjectPtr,\s*kExpectedEncounterCount>[\s\S]*?created_encounter_candidates_' `
    -and $captureEncounterCandidate -match `
        '(?:object_class->GetFName\(\)\.ToUnstableInt\(\)|DSNWRPR_CLASS_NAME_KEY\(object_class\))' `
    -and $captureEncounterCandidate -match `
        'std::find\([\s\S]*?encounter_class_name_keys_' `
    -and $captureEncounterCandidate -match `
        'found\s*-\s*self->encounter_class_name_keys_\.begin\(\)' `
    -and $captureEncounterCandidate -notmatch `
        'to_string|to_wstring|std::string|encounter_class_indices_|new\s' `
    -and $notifyObjectCreated -match `
        'created_encounter_candidates_\[encounter_index\]\s*=\s*encounter_weak' `
    -and $consumeEncounterCandidates -match `
        'if\s*\(activity_suppressed_\s*\|\|\s*!position_valid_\)[\s\S]*?return' `
    -and $consumeEncounterCandidates -match `
        '!encounter_available\([\s\S]*?encounter_catalog_\[index\],\s*now_unix_seconds\)' `
    -and $consumeEncounterCandidates -match `
        'std::array<FWeakObjectPtr,\s*kExpectedEncounterCount>\s+candidates' `
    -and $consumeEncounterCandidates -match `
        'while\s*\(visited\s*<\s*candidates\.size\(\)[\s\S]*?position_queries\s*<\s*kEncounterCandidateProbeBudgetPerControlTick\)' `
    -and $consumeEncounterCandidates -match `
        'kEncounterObservationRadius\s*\*\s*kEncounterObservationRadius' `
    -and $consumeEncounterCandidates -match `
        'candidates\[index\]\.Get\(\)' `
    -and $consumeEncounterCandidates -match `
        'created_encounter_processed_activation_\[index\]' `
    -and $consumeEncounterCandidates -match `
        '\+\+position_queries;[\s\S]*?read_actor_position\(actor,\s*&actor_position\)' `
    -and $consumeEncounterCandidates -match `
        'distance_squared\(player_,\s*actor_position\)\s*>\s*radius_squared' `
    -and $consumeEncounterCandidates -match `
        'observe_encounter\([\s\S]*?candidates\[index\],[\s\S]*?identity,[\s\S]*?encounter_catalog_\[index\]\.class_name,[\s\S]*?actor_position' `
    -and $consumeEncounterCandidates -notmatch `
        'actor_begin_unsafe|spec\.position|catalog_\[index\]\.position|FindAllOf|FindFirstOf|StaticFindObject|std::vector|\bnew\b' `
    -and $engineTickUnsafe -match `
        'if\s*\(position_valid_\s*&&\s*now\s*>=\s*next_discovery_\)[\s\S]*?consume_created_encounter_candidates\(\)' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'consume_created_encounter_candidates\(\)').Count -eq 1 `
    -and $transitionBegin -match `
        'clear_created_encounter_candidates\(\)' `
    -and $activateLifecycle -notmatch `
        'clear_created_encounter_candidates\(\)' `
    -and $disableLifecycle -notmatch `
        'clear_created_encounter_candidates\(\)') `
    'Encounter discovery must use fixed create-event weak slots, at most eight current-actor position queries per 250 ms control tick, and no suppressed work or UObject enumeration.'

$initializeBirdEggClassKeys = Get-MainFunction `
    $mainCode 'initialize_bird_egg_class_name_keys'
$captureCreatedBirdEgg = Get-MainFunction `
    $mainCode 'capture_created_bird_egg_actor_unsafe'
$captureCreatedBirdEggGuarded = Get-MainFunction `
    $mainCode 'capture_created_bird_egg_actor_guarded'
$publishCreatedBirdEgg = Get-MainFunction `
    $mainCode 'publish_created_bird_egg_candidate'
$synchronizeBirdEggCandidates = Get-MainFunction `
    $mainCode 'synchronize_bird_egg_candidates'
$discoverBirdEggCandidates = Get-MainFunction `
    $mainCode 'discover_bird_egg_candidates'
$serviceActiveBirdEggs = Get-MainFunction `
    $mainCode 'service_active_bird_eggs'
$initializeBirdEggAvailability = Get-MainFunction `
    $mainCode 'initialize_bird_egg_availability_schema'
$readBirdEggAvailable = Get-MainFunction `
    $mainCode 'read_bird_egg_available'
$markBirdEggEnd = Get-MainFunction $mainCode 'mark_bird_egg_end'
$pruneBirdEggCandidates = Get-MainFunction `
    $mainCode 'prune_bird_egg_candidates_for_world'
$resetBirdEggVisibility = Get-MainFunction `
    $mainCode 'reset_bird_egg_active_visibility'
$clearBirdEggCandidates = Get-MainFunction `
    $mainCode 'clear_bird_egg_candidates'
$actorBeginUnsafe = Get-MainFunction $mainCode 'actor_begin_unsafe'
$actorEndUnsafe = Get-MainFunction $mainCode 'actor_end_unsafe'
$isBirdEggClass = Get-MainFunction $mainCode 'is_bird_egg_class'
$collectWorldMapSnapshot = Get-MainFunction `
    $mainCode 'collect_world_map_marker_snapshot'
$rebuildCompactSnapshot = Get-MainFunction `
    $mainCode 'rebuild_compact_snapshot'
$applyVisibilityHubResult = Get-MainFunction `
    $mainCode 'apply_visibility_hub_result'

Assert-True ($mainCode -match `
        'kBirdEggCandidateCapacity\s*=\s*512' `
    -and $mainCode -match `
        'kBirdEggActiveCapacity\s*=\s*16' `
    -and $mainCode -match `
        'kBirdEggPositionProbeBudgetPerControlTick\s*=\s*8' `
    -and $mainCode -notmatch `
        'kBirdEggActiveProbeInterval|next_bird_egg_active_probe_' `
    -and $mainCode -match `
        'bird_egg_active_interval_ms=250\s+bird_egg_active_schedule=shared_discovery_edge' `
    -and $mainCode -match `
        'std::array<FWeakObjectPtr,\s*kBirdEggCandidateCapacity>\s+created_bird_egg_candidates_' `
    -and $mainCode -match `
        'std::array<BirdEggRuntimeCandidate,\s*kBirdEggCandidateCapacity>\s+bird_egg_runtime_candidates_') `
    'Bird-egg discovery must retain the fixed 512-candidate, nearest-16, eight-query contract and share the existing 250 ms discovery edge without another timer.'
Assert-True ($initializeBirdEggClassKeys -match `
        'std::array<const wchar_t\*,\s*2>\s+class_names[\s\S]*?L"Bird_Egg01_C"\s*,\s*L"Bird_Egg02_C"' `
    -and $initializeBirdEggClassKeys -match `
        'DSNWRPR_NAME_KEY\(name\)[\s\S]*?bird_egg_class_name_keys_\[index\]' `
    -and $isBirdEggClass -match `
        'return\s+name\s*==\s*"Bird_Egg01_C"\s*\|\|\s*name\s*==\s*"Bird_Egg02_C"\s*;' `
    -and $captureCreatedBirdEgg -match `
        'bird_egg_class_name_keys_ready_[\s\S]*?unreal_object->IsA\(self->actor_class_\)' `
    -and $captureCreatedBirdEgg -match `
        'DSNWRPR_CLASS_NAME_KEY\(object_class\)[\s\S]*?std::find\([\s\S]*?bird_egg_class_name_keys_' `
    -and $captureCreatedBirdEggGuarded -match `
        'capture_created_bird_egg_actor_unsafe\(object, weak\)' `
    -and $captureCreatedBirdEgg -notmatch `
        'to_string|to_wstring|std::string|ProcessEvent|FindAllOf|FindFirstOf') `
    'The create listener must accept exactly Bird_Egg01_C and Bird_Egg02_C through allocation-free FName keys.'
Assert-True ($notifyObjectCreated -match `
        'capture_created_bird_egg_actor_guarded\([\s\S]*?publish_created_bird_egg_candidate\(bird_egg_weak\)' `
    -and [regex]::Matches(
        $notifyObjectCreated,
        'publish_created_bird_egg_candidate\(bird_egg_weak\)').Count -eq 1 `
    -and $publishCreatedBirdEgg -match `
        'WeakIdentity\s+identity[\s\S]*?existing_identity\.packed\(\)\s*==\s*identity\.packed\(\)' `
    -and $publishCreatedBirdEgg -match `
        'empty\s*==\s*created_bird_egg_candidates_\.size\(\)[\s\S]*?bird_egg_candidate_drop_count_' `
    -and $publishCreatedBirdEgg -notmatch `
        'Position|planar_distance|distance_squared|std::vector|\bnew\b') `
    'Bird-egg publication must use one fixed weak mailbox and identity-only deduplication, including overlapping eggs.'
Assert-True ($objectState -match `
        'kBirdEggRequiredInteractableValue\s*=\s*2' `
    -and $objectState -match `
        'kBirdEggRequiredInteractTypeValue\s*=\s*2' `
    -and $objectState -match `
        'bird_egg_interaction_available\([\s\S]*?interactable_value\s*==\s*kBirdEggRequiredInteractableValue[\s\S]*?interact_type_value\s*==\s*kBirdEggRequiredInteractTypeValue' `
    -and $initializeBirdEggAvailability -match `
        'GetPropertyByNameInChain\(L"InteractComponent"\)[\s\S]*?GetPropertyByNameInChain\(L"InteractableValue"\)[\s\S]*?GetPropertyByNameInChain\(L"InteractTypeValue"\)' `
    -and [regex]::Matches(
        $initializeBirdEggAvailability,
        'CastField<FEnumProperty>').Count -eq 2 `
    -and [regex]::Matches(
        $initializeBirdEggAvailability,
        'GetUnderlyingProperty\(\)').Count -eq 2 `
    -and $initializeBirdEggAvailability -match `
        'component->GetOuterPrivate\(\)\s*!=\s*actor[\s\S]*?!interactable_underlying_property->IsInteger\(\)[\s\S]*?interactable_property->GetSize\(\)\s*!=\s*sizeof\(std::uint8_t\)[\s\S]*?!interact_type_underlying_property->IsInteger\(\)[\s\S]*?interact_type_property->GetSize\(\)\s*!=\s*sizeof\(std::uint8_t\)' `
    -and $initializeBirdEggAvailability -match `
        'bird_egg_interact_component_property_\s*=\s*component_property[\s\S]*?bird_egg_interactable_value_property_\s*=\s*interactable_property[\s\S]*?bird_egg_interactable_value_underlying_property_\s*=[\s\S]*?interactable_underlying_property[\s\S]*?bird_egg_interact_type_property_\s*=\s*interact_type_property[\s\S]*?bird_egg_interact_type_underlying_property_\s*=[\s\S]*?interact_type_underlying_property[\s\S]*?bird_egg_availability_schema_ready_\s*=\s*true' `
    -and $readBirdEggAvailable -match `
        'initialize_bird_egg_availability_schema\(actor\)[\s\S]*?GetObjectPropertyValue\(component_value\)[\s\S]*?component->GetOuterPrivate\(\)\s*!=\s*actor' `
    -and $readBirdEggAvailable -match `
        'bird_egg_interactable_value_underlying_property_[\s\S]*?GetUnsignedIntPropertyValue\(interactable_value\)[\s\S]*?bird_egg_interact_type_underlying_property_[\s\S]*?GetUnsignedIntPropertyValue\(interact_type_value\)[\s\S]*?dswros::bird_egg_interaction_available\([\s\S]*?interactable,[\s\S]*?interact_type\)' `
    -and $initializeBirdEggAvailability -notmatch `
        'CastField<FNumericProperty>' `
    -and ($initializeBirdEggAvailability + "`n" + $readBirdEggAvailable) `
        -notmatch 'ProcessEvent|read_actor_hidden|FindAllOf|FindFirstOf|std::vector|\bnew\b') `
    'Bird-egg availability must read the actor-owned exact DInteractableComponent one-byte enum state through cached FEnumProperty underlyings, with no Actor::IsHidden fallback.'
Assert-True ($discoverBirdEggCandidates -match `
        'while\s*\(visited\s*<\s*bird_egg_runtime_candidates_\.size\(\)[\s\S]*?position_queries\s*<\s*kBirdEggPositionProbeBudgetPerControlTick\)' `
    -and $discoverBirdEggCandidates -match `
        '\+\+position_queries;[\s\S]*?read_bird_egg_available\(actor,\s*&available\)[\s\S]*?if\s*\(!availability_known\)[\s\S]*?continue[\s\S]*?if\s*\(!available\)[\s\S]*?continue[\s\S]*?read_actor_position\(actor,\s*&position\)' `
    -and [regex]::Matches(
        $discoverBirdEggCandidates,
        'read_actor_position\(').Count -eq 1 `
    -and [regex]::Matches(
        $discoverBirdEggCandidates,
        'read_bird_egg_available\(').Count -eq 1 `
    -and $discoverBirdEggCandidates -notmatch `
        'read_actor_hidden\(' `
    -and $engineTickUnsafe -match `
        'if\s*\(position_valid_\s*&&\s*now\s*>=\s*next_discovery_\)[\s\S]*?next_discovery_\s*=\s*now\s*\+\s*kDiscoveryInterval;[\s\S]*?discover_bird_egg_candidates\(\)' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'discover_bird_egg_candidates\(\)').Count -eq 1 `
    -and $mainCode -match `
        'kDiscoveryInterval\s*=\s*std::chrono::milliseconds\{250\}') `
    'Unknown bird-egg positions must consume at most eight exact interaction-state and position attempts on the existing 250 ms control edge; unknown or not-yet-live components remain bounded retry candidates.'
Assert-True ($serviceActiveBirdEggs -notmatch `
        'next_bird_egg_active_probe_|kBirdEggActiveProbeInterval' `
    -and $serviceActiveBirdEggs -match `
        'std::array<StaticRenderCandidate,\s*kBirdEggActiveCapacity>\s+nearest' `
    -and $serviceActiveBirdEggs -match `
        'candidate\.weak\.Get\(\)[\s\S]*?weak_identity_valid[\s\S]*?object_world_guarded\(actor\)\s*==\s*current_world[\s\S]*?read_bird_egg_available\(actor,\s*&available\)' `
    -and $serviceActiveBirdEggs -match `
        'dswros::bird_egg_runtime_present\(\s*weak_identity_valid,\s*same_world,\s*availability_known,\s*available\)' `
    -and $serviceActiveBirdEggs -match `
        'candidate\.presence\.sample\(\s*present,\s*now_milliseconds\)' `
    -and $serviceActiveBirdEggs -match `
        '!weak_identity_valid\s*\|\|\s*!same_world[\s\S]*?\(!visible\s*&&\s*availability_known\s*&&\s*!available\)[\s\S]*?retire_bird_egg_candidate\(index\)' `
    -and $serviceActiveBirdEggs -notmatch `
        'read_actor_position\(|read_actor_hidden\(' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'service_active_bird_eggs\(now\)').Count -eq 1 `
    -and $engineTickUnsafe -match `
        'if\s*\(position_valid_\s*&&\s*now\s*>=\s*next_discovery_\)[\s\S]*?next_discovery_\s*=\s*now\s*\+\s*kDiscoveryInterval;[\s\S]*?discover_bird_egg_candidates\(\);[\s\S]*?service_active_bird_eggs\(now\);[\s\S]*?probe_observed_objects\(\)') `
    'The nearest-16 active set must share the existing 250 ms discovery edge for exact DInteractableComponent/world presence probes, retain immediate EndPlay retirement and the 400 ms unavailable debounce, and perform no repeated position or Actor::IsHidden query.'
$birdEggSteadyWork = $publishCreatedBirdEgg + "`n" +
    $synchronizeBirdEggCandidates + "`n" +
    $discoverBirdEggCandidates + "`n" + $serviceActiveBirdEggs
Assert-True ($birdEggSteadyWork -notmatch `
        'FindAllOf|FindFirstOf|StaticFindObject|UObjectArray|sqlite|sqlcipher|\bSQL\b|save_reconcil|filesystem|fstream|ifstream|ofstream|std::vector|\bnew\b|ProcessEvent|collect_world_map_marker_snapshot|world_map_umg_renderer_|append_log') `
    'Bird-egg steady work must not enumerate UObjects, query saves, perform file I/O, allocate dynamic queues, or enter the expanded-map path.'
Assert-True ($objectState -match `
        'class\s+LiveMarkerPresenceGate[\s\S]*?kMissingDebounceMilliseconds\s*=\s*400' `
    -and $objectState -match `
        'bird_egg_runtime_present\([\s\S]*?weak_identity_valid\s*&&\s*same_world\s*&&\s*availability_known[\s\S]*?&&\s*interactable' `
    -and $serviceActiveBirdEggs -match `
        'candidate\.presence\.sample\(' `
    -and $nativeTests -match `
        'runtime marker must survive the first 399 missing milliseconds' `
    -and $nativeTests -match `
        'runtime marker must disappear after 400 continuous missing milliseconds' `
    -and $nativeTests -match `
        'restarted missing window must retain the marker for 399 milliseconds' `
    -and $nativeTests -match `
        'restarted missing window must expire at exactly 400 milliseconds' `
    -and $nativeTests -match `
        'live marker presence sampling must not allocate' `
    -and $nativeTests -match `
        'the exact enum-backed On and NormalGather values must be available' `
    -and $nativeTests -match `
        'an Off bird-egg interaction switch must be unavailable' `
    -and $nativeTests -match `
        'a non-NormalGather bird-egg interaction type must be unavailable' `
    -and $nativeTests -match `
        'bird-egg enum-state classification must not allocate' `
    -and $nativeTests -match `
        'an exact same-world interactable bird egg must be present' `
    -and $nativeTests -match `
        'an unknown bird-egg interaction state must fail closed' `
    -and $nativeTests -match `
        'a non-interactable bird egg must be absent' `
    -and $nativeTests -match `
        'an invalid bird-egg weak identity must be absent' `
    -and $nativeTests -match `
        'a bird egg from another world must be absent') `
    'Bird-egg disappearance must combine tested exact interaction-state presence with the allocation-free 400 ms continuous-missing debounce.'
$birdEggBeginBranch = [regex]::Match(
    $actorBeginUnsafe,
    '(?s)if\s*\(is_bird_egg_class\(class_name\)\)\s*\{.*?return;\s*\}').Value
Assert-True ($birdEggBeginBranch.Length -gt 0 `
    -and $birdEggBeginBranch -match `
        'publish_created_bird_egg_candidate\(weak\)' `
    -and $birdEggBeginBranch -notmatch `
        'read_actor_position|read_bird_egg_available' `
    -and $actorEndUnsafe -match `
        'is_bird_egg_class\([\s\S]*?mark_bird_egg_end\(identity\)' `
    -and $markBirdEggEnd -match `
        'candidate\.identity\.packed\(\)\s*==\s*identity\.packed\(\)[\s\S]*?retire_bird_egg_candidate\(index\)[\s\S]*?bird_egg_end_event_count_' `
    -and $resetBirdEggVisibility -match `
        'candidate\.presence\.reset\(\)[\s\S]*?bird_egg_active_count_\s*=\s*0' `
    -and $clearBirdEggCandidates -match `
        'created_bird_egg_candidates_\.fill\(FWeakObjectPtr\{\}\)[\s\S]*?bird_egg_runtime_candidates_\.fill\(BirdEggRuntimeCandidate\{\}\)' `
    -and $transitionBeginLifecycle -match `
        'reset_bird_egg_active_visibility\(\)' `
    -and $transitionEndLifecycle -match `
        'prune_bird_egg_candidates_for_world\(current_world\)[\s\S]*?clear_bird_egg_candidates\(\)' `
    -and $pruneBirdEggCandidates -match `
        'std::array<FWeakObjectPtr,\s*kBirdEggCandidateCapacity>\s+retained[\s\S]*?object_world_guarded\(object\)\s*!=\s*current_world' `
    -and $activateLifecycle -match `
        'reset_bird_egg_active_visibility\(\)' `
    -and $disableLifecycle -match `
        'reset_bird_egg_active_visibility\(\)' `
    -and $uobjectArrayShutdown -match `
        'clear_bird_egg_candidates\(\)') `
    'Bird-egg BeginPlay must publish only a weak candidate until its owned interaction component is ready; exact EndPlay, F7/F8, travel, and shutdown must retire, reset, or prune only weak current-world state.'
Assert-True ($visibilityHubHeader -match `
        'Clock,[\s\S]*?Treasure,[\s\S]*?Boss,[\s\S]*?Assault,[\s\S]*?MiniGames,[\s\S]*?AreaQuests,[\s\S]*?BirdEggs,[\s\S]*?Count' `
    -and $visibilityHubHeader -match `
        'kRadarVisibilityAllCategories\s*=\s*0x7FU' `
    -and $visibilityHubHeader -match `
        'kRadarVisibilityWorldCategories\s*=\s*0x3EU' `
    -and $visibilityConfig -match '(?m)^\[radar\]$' `
    -and $visibilityConfig -match '(?m)^clock=true$' `
    -and $visibilityConfig -match '(?m)^bird_eggs=true$' `
    -and $visibilityConfig -match '(?m)^\[map\]$' `
    -and $visibilityConfig -match '(?m)^\[modes\]$' `
    -and $visibilityConfig -match '(?m)^area_quests=available$' `
    -and $visibilityConfig -match '(?m)^assault=available$' `
    -and $visibilityParser -match 'kMaximumVisibilityConfigBytes\s*=\s*4096U' `
    -and $visibilityParser -match `
        'VisibilityConfigFormat::LegacySchema1[\s\S]*?VisibilityConfigFormat::LegacySchema4' `
    -and $mainCode -match 'parse_visibility_config\(' `
    -and $mainCode -match 'format_visibility_config\(' `
    -and $mainCode -match 'area_quest_mode=' `
    -and $mainCode -match 'assault_mode=' `
    -and $visibilityHubHeader -match `
        'enum class AssaultDisplayMode[\s\S]*?Current,[\s\S]*?All' `
    -and $visibilityHubHeader -match `
        'RadarVisibilityHubResult[\s\S]*?AssaultDisplayMode\s+assault_mode\{AssaultDisplayMode::Current\}' `
    -and $visibilityHub -match `
        'column\s*==\s*1[\s\S]*?definition\.category\s*==\s*RadarVisibilityCategory::Clock[\s\S]*?definition\.category[\s\S]*?==\s*RadarVisibilityCategory::BirdEggs[\s\S]*?add_text\([\s\S]*?L"-"' `
    -and $visibilityHub -match `
        'ends_radar_section\s*=\s*[\s\S]*?RadarVisibilityCategory::BirdEggs[\s\S]*?ends_radar_section\s*\?\s*3\.0\s*:\s*1\.0[\s\S]*?ends_radar_section\s*\?\s*kPanelAccent\s*:\s*kRowDivider' `
    -and $visibilityHub -match `
        'column\s*==\s*1[\s\S]*?category\s*==\s*static_cast<std::size_t>\([\s\S]*?RadarVisibilityCategory::Clock\)[\s\S]*?category\s*==\s*static_cast<std::size_t>\([\s\S]*?RadarVisibilityCategory::BirdEggs\)[\s\S]*?continue' `
    -and $applyVisibilityHubResult -match `
        'bird_egg_visibility_changed[\s\S]*?RadarVisibilityCategory::BirdEggs' `
    -and $applyVisibilityHubResult -match `
        'if\s*\(bird_egg_visibility_changed\)[\s\S]*?reset_bird_egg_active_visibility\(\)' `
    -and $applyVisibilityHubResult -match `
        'if\s*\(world_changed\)[\s\S]*?visibility_hub_\.is_open\(\)[\s\S]*?visibility_hub_world_map_baseline_mask_[\s\S]*?visibility_hub_world_map_baseline_area_quest_mode_[\s\S]*?visibility_hub_world_map_baseline_assault_mode_' `
    -and $applyVisibilityHubResult -match `
        'assault_display_mode_\s*=\s*result\.assault_mode[\s\S]*?assault_mode_changed[\s\S]*?compact_changed[\s\S]*?world_changed' `
    -and $applyVisibilityHubResult -match `
        'persist_visibility_settings\([\s\S]*?visibility_masks_,\s*area_quest_display_mode_,[\s\S]*?assault_display_mode_' `
    -and $visibilityHub -match `
        'L"ASSAULT MODE"[\s\S]*?L"AVAILABLE"[\s\S]*?L"ALL"' `
    -and $visibilityHub -match `
        'assault_current_checked[\s\S]*?assault_all_checked[\s\S]*?pending_assault_mode_[\s\S]*?set_checked' `
    -and $visibilityHub -match `
        'pending_masks_\s*==\s*source_masks_[\s\S]*?pending_area_quest_mode_\s*==\s*source_area_quest_mode_[\s\S]*?pending_assault_mode_\s*==\s*source_assault_mode_' `
    -and $visibilityHub -notmatch `
        'std::chrono|Clock::|sqlite|sqlcipher|execute_optional_sql|request_area_quest_scan|service_runtime_visibility_edges' `
    -and $mainCode -match `
        'VISIBILITY_HUB_WORLD_MAP_REFRESH[\s\S]*?action=single_coalesced_rebuild' `
    -and $mainCode -match `
        'RadarVisibilityHubAction::Opened[\s\S]*?visibility_hub_world_map_baseline_valid_\s*=\s*true[\s\S]*?visibility_hub_world_map_baseline_mask_[\s\S]*?visibility_hub_world_map_baseline_area_quest_mode_[\s\S]*?visibility_hub_world_map_baseline_assault_mode_' `
    -and [regex]::Matches(
        $mainCode,
        'flush_visibility_hub_world_map_refresh\("(?:x_close|f6_close)"\)').Count -eq 2) `
    'F6 must preserve the schema-4 AVAILABLE public default, expose mutually exclusive Assault AVAILABLE/ALL controls without new timed work, and coalesce only net expanded-map changes to one close-edge transaction.'
Assert-True ($rebuildCompactSnapshot -match `
        'encounter_visible_for_selected_mode\([\s\S]*?spec,\s*now_unix_seconds\)' `
    -and $collectWorldMapSnapshot -match `
        'encounter_visible_for_selected_mode\([\s\S]*?spec,\s*now_unix_seconds\)' `
    -and $encounterVisibilityMask -match `
        'encounter_visible_for_selected_mode_at_hour\([\s\S]*?spec,\s*now_unix_seconds,\s*world_hour\)' `
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
    'Assault ALL may affect only compact, expanded-map, and fixed 49-bit presentation; it must expose the static Assault catalog while preserving AVAILABLE, Boss-time, completion, and cooldown authority.'
Assert-True ($collectWorldMapSnapshot -notmatch '\bBirdEgg\b' `
    -and $mainCode -notmatch `
        'world_visibility_enabled\(\s*dsnwr::RadarVisibilityCategory::BirdEggs' `
    -and $rendererHeader -match `
        'kCompactUmgMarkerCapacity\s*=\s*80' `
    -and $model -match `
        'kMaximumSelectedTreasures\s*=\s*80') `
    'Bird eggs must remain absent from the world-map snapshot without expanding either accepted 80-slot compact bound.'
Assert-True ($main -notmatch `
        'SharedPublisher|CreateFileMappingW|MapViewOfFile|publisher_|DIAGNOSTIC_MAPPING_DISABLED') `
    'The retired external diagnostic mapping must not remain in the production runtime.'
Assert-True ($main -match 'area_quest_visible_for_selected_mode\(index\)' `
    -and $main -match 'area_quest_compact=nearby_prerequisite_proven_plus_runtime' `
    -and $main -match 'area_quest_triggerability=fail_closed_main_group_conditions') `
    'Compact area quests must route through the selected AVAILABLE or ALL presentation policy.'
Assert-True ($areaQuestVisibility -match 'AreaQuestState::Acceptable' `
    -and $areaQuestVisibility -match 'AreaQuestState::Progress' `
    -and $areaQuestVisibility -match 'AreaQuestEligibilityProof::Eligible' `
    -and $areaQuestVisibility -match 'completion_observed' `
    -and $areaQuestVisibility -match 'current_state\s*==\s*AreaQuestState::End' `
    -and $nativeTests -match 'fail and non-progress transitions must not latch completion' `
    -and $nativeTests -match 'a proven eligible unfinished quest must be globally visible' `
    -and $nativeTests -match 'without prerequisite proof must fail closed' `
    -and $nativeTests -match 'unconfirmed terminal samples must not hide an unfinished proven quest' `
    -and $nativeTests -match 'a confirmed runtime completion must hide the quest' `
    -and $areaQuestVisibility -match 'area_quest_visible_for_display_mode' `
    -and $nativeTests -match 'ALL mode must still hide saved or runtime-confirmed completions') `
    'Area-quest compact/world visibility precedence lacks production-used tests.'
Assert-True ($saveReconciler -match 'SELECT USER_DBID,QUEST_ID,COMPLETE_CNT' `
    -and $saveReconciler -match 'FROM tb_dynamic_quest_complete' `
    -and $saveReconciler -match 'execute_optional_sql' `
    -and $saveReconciler -match 'user_dbid' `
    -and $saveReconciler -match 'user_identity_ambiguous' `
    -and $saveReconciler -match '!quest_fields->completion_identity_ambiguous' `
    -and $saveReconciler -notmatch 'tb_dynamic_quest_group' `
    -and $main -match 'dynamic_completion_user_dbid=' `
    -and $main -match 'dynamic_completion_identity_ambiguous=' `
    -and $main -match 'completion\.quest_id' `
    -and $main -match 'area_quest_save_completion_' `
    -and $main -match 'DDynamicQuestDataTable' `
    -and $main -match 'capture_area_quest_definitions_guarded' `
    -and $main -match 'normal_quest_completion_proofs_' `
    -and $main -match 'completed_dynamic_quest_ids_' `
    -and $main -match 'rebuild_area_quest_world_map_eligibility\(\)') `
    'The one-shot save and Main/Group snapshots do not provide fail-isolated prerequisite eligibility.'
$areaQuestStep = [regex]::Match(
    $main,
    '(?ms)^\s{4}void\s+process_one_area_quest\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($areaQuestStep.Length -gt 0) `
    'The one-item area quest scan step was not found.'
Assert-True ([regex]::Matches($areaQuestStep, 'query_area_quest_state_guarded\(').Count -eq 1 `
    -and $areaQuestStep -notmatch '\b(?:for|while)\s*\(' `
    -and $areaQuestStep -notmatch 'FindAllOf|FindFirstOf|StaticFindObject') `
    'One game frame may query at most one area quest and perform no discovery.'
Assert-True ($areaQuestStep -match 'compact_rebind_dirty_\s*=\s*true' `
    -and $areaQuestStep -match `
        'refresh_world_map_atlas_for_runtime_delta\(\s*"area_quest_scan_failed"') `
    'A structured quest fault may leave stale compact or expanded-map pixels attached.'
Assert-True ($main -match 'fly_count != 33U' -and $main -match 'mole_count != 40U' `
    -and $main -match 'wave_count != 10U') `
    'Mini-game catalog validation must enforce 33 Fly, 40 Mole, and 10 Wave records.'
Assert-True ($main -match 'cooldown_minutes=120' `
    -and $main -match 'mark_encounter_defeated\(observed\.id\)') `
    'Encounter save and native delta paths must enforce the two-hour cooldown.'
Assert-True ($markEncounterDefeated -match `
        'encounter_spec_for_id\(id\)' `
    -and $markEncounterDefeated -match `
        '!encounter_state_ready_[\s\S]*?!encounter_time_condition_matches\(\*spec\)' `
    -and $markEncounterDefeated -match `
        'reason=not_currently_available' `
    -and $probeObservedObjects -match `
        'encounter_available\([\s\S]*?\*current_encounter,\s*now_unix_seconds\)' `
    -and $probeObservedObjects -match `
        'if\s*\(!lifecycle_valid\s*\|\|\s*!encounter_currently_available\)' `
    -and $probeObservedObjects -match `
        'clear_processed_encounter_identity\(observed\.id\)' `
    -and $probeObservedObjects -match `
        'reason=encounter_not_currently_available') `
    'Encounter completion must recheck current time/cooldown availability and clear processed identity on every invalid lifecycle context.'
$observeEncounter = [regex]::Match(
    $main,
    '(?ms)^\s{4}void\s+observe_encounter\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($observeEncounter.Length -gt 0) `
    'The encounter observation function was not found.'
Assert-True ($observeEncounter -match 'encounter_spec_for_class\(class_name\)' `
    -and $observeEncounter -match 'encounter_state_ready_' `
    -and $observeEncounter -match 'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $observeEncounter -notmatch '600\.0\s*\*\s*600\.0') `
    'Encounter observation must retain exact unique class identity after the caller proves the current-actor 100-meter bound.'
Assert-True ($main -match 'unique_classes\.insert\(spec\.class_name\)' `
    -and $main -match 'encounter_class_indices_\.contains\(name\)') `
    'Encounter class identity must remain load-time unique and use the fixed class index.'
$recoverEncounterEnd = [regex]::Match(
    $main,
    '(?ms)^\s{4}\[\[nodiscard\]\]\s+bool\s+recover_unobserved_encounter_end\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($recoverEncounterEnd.Length -gt 0 `
    -and $recoverEncounterEnd -match 'EEndPlayReason' `
    -and $recoverEncounterEnd -match 'EEndPlayReason::Destroyed' `
    -and $recoverEncounterEnd -notmatch 'EEndPlayReason::RemovedFromWorld' `
    -and $recoverEncounterEnd -notmatch `
        'spec->kind\s*!=\s*EncounterKind::(?:Assault|Boss)' `
    -and $recoverEncounterEnd -match 'encounter_spec_for_class\(class_name\)' `
    -and $recoverEncounterEnd -match 'read_actor_position\(actor,\s*&actor_position\)' `
    -and $recoverEncounterEnd -match `
        'activity_suppressed_\s*\|\|\s*!position_valid_' `
    -and $recoverEncounterEnd -match 'encounter_available\(\*spec,\s*now_unix_seconds\)' `
    -and $recoverEncounterEnd -match 'accept_unobserved_encounter_end\(evidence\)' `
    -and $recoverEncounterEnd -match 'kEncounterObservationRadius\s*\*\s*kEncounterObservationRadius' `
    -and $recoverEncounterEnd -match `
        'distance_squared\(player_,\s*actor_position\)[\s\S]*?<=\s*radius_squared' `
    -and $recoverEncounterEnd -match 'mark_eligible_end\(\)' `
    -and $recoverEncounterEnd -match 'recovered_from_end_play' `
    -and $recoverEncounterEnd -match 'OBJECT_END_RECOVERED_NATIVE' `
    -and $recoverEncounterEnd -notmatch `
        'distance_squared\(player_,\s*spec->position\)|FindAllOf|FindFirstOf|StaticFindObject|while\s*\(') `
    'Unobserved Boss and Assault recovery must require Destroyed, exact actor/player proximity, current availability, and event-driven work only.'
$evaluateAreaQuestCondition = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}\[\[nodiscard\]\]\s+dswros::AreaQuestEligibilityProof\s+evaluate_area_quest_condition\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
Assert-True ($evaluateAreaQuestCondition.Length -gt 0 `
    -and $consumeEncounterCandidates -match `
        'encounter_available\([\s\S]*?encounter_catalog_\[index\],\s*now_unix_seconds\)' `
    -and $observeEncounter -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $probeObservedObjects -match `
        'encounter_available\([\s\S]*?\*current_encounter,\s*now_unix_seconds\)' `
    -and $encounterDeathPreUnsafe -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $recoverEncounterEnd -match `
        'encounter_available\(\*spec,\s*now_unix_seconds\)' `
    -and $evaluateAreaQuestCondition -match `
        'DynamicQuestConditionType::MonsterAlive[\s\S]*?encounter_available\(\*encounter,\s*unix_seconds\(\)\)' `
    -and ($consumeEncounterCandidates + $observeEncounter `
        + $probeObservedObjects + $encounterDeathPreUnsafe `
        + $recoverEncounterEnd + $evaluateAreaQuestCondition) -notmatch `
        'encounter_visible_for_(?:selected_mode|display_mode)' `
    -and $markEncounterDefeated -match `
        '!encounter_state_ready_[\s\S]*?!encounter_time_condition_matches\(\*spec\)' `
    -and $applyEncounterDefeatState -match `
        'encounter_cooldown_write_allowed\(') `
    'Assault ALL must never weaken completion, object binding, observation, death/recovery, cooldown writes, or area-quest MONSTER_ALIVE eligibility; those paths must retain strict live availability.'
Assert-True ($main -match 'encounter_type_for_id\(observed\.id\)' `
    -and $main -match 'destroyed_exact_unique_class_player_to_actor_proximity_current_available') `
    'Encounter runtime logs must identify Boss versus Assault and the recovered evidence path.'
Assert-True ($objectState -match 'accept_unobserved_encounter_end' `
    -and $objectState -match 'class\s+EncounterDisappearanceConfirmation' `
    -and $objectState -match 'kRequiredPresentMilliseconds\s*=\s*1''000' `
    -and $objectState -match 'kRequiredMissingMilliseconds\s*=\s*10''000' `
    -and $objectState -match 'kRequiredMissingSamples\s*=\s*40' `
    -and $objectState -match 'encounter_available_now' `
    -and $objectState -match 'encounter_cooldown_write_allowed' `
    -and $main -match 'dswros::accept_unobserved_encounter_end\(evidence\)' `
    -and $main -match 'dswros::encounter_available_now\(' `
    -and $main -match 'dswros::encounter_cooldown_write_allowed\(') `
    'The tested fail-closed encounter and cooldown gates must be used by production code.'
Assert-True ($nativeTests -match 'streaming removal must not recover an unobserved encounter' `
    -and $nativeTests -match 'distant actor destruction must not recover an encounter' `
    -and $nativeTests -match 'expired time-window encounter must reject completion' `
    -and $nativeTests -match 'unready encounter state must reject completion' `
    -and $nativeTests -match 'activity suppression entry must reset encounter observations' `
    -and $nativeTests -match 'unchanged activity suppression must not repeat encounter cleanup' `
    -and $nativeTests -match 'open-world return must preserve fresh encounter candidates' `
    -and $nativeTests -match 'future cooldown must reject duplicate cooldown write' `
    -and $nativeTests -match 'less than ten missing seconds must fail closed' `
    -and $nativeTests -match 'leaving range must cancel a pending encounter disappearance') `
    'Native behavior tests must cover streaming, position, current availability, and duplicate-cooldown rejection.'
Assert-True ($objectState -match 'preserve_observed_encounter_removal' `
    -and $objectState -match 'arm_observed_end_fallback' `
    -and $objectState -match 'encounter_cursor_context_allowed' `
    -and $main -match 'preserve_observed_encounter_removal' `
    -and $main -match 'arm_observed_end_fallback' `
    -and $main -match `
        'encounter_cursor_context_allowed\(\s*mouse_cursor_visible_,\s*observed\.logical_end\)' `
    -and $main -match `
        'found->second\.weak(?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})' `
    -and $main -match 'observed\.destroyed_end' `
    -and $nativeTests -match `
        'observed EndPlay fallback must complete after forty missing samples and ten seconds' `
    -and $nativeTests -match `
        'a returned encounter must cancel the missing sequence' `
    -and $nativeTests -match `
        'an exact ended numeric observation must survive opening the map') `
    'Observed RemovedFromWorld encounters must start the numeric ten-second fallback even after a short Assault, survive a map-menu cursor without retaining the UObject, and still cancel on actor return.'
Assert-True ($objectState -match 'encounter_activity_edge_requires_reset' `
    -and $applyActivitySuppressionEdge -match `
        'encounter_activity_edge_requires_reset\([\s\S]*?changed[\s\S]*?activity_suppressed_' `
    -and $applyActivitySuppressionEdge -match `
        'clear_encounter_observations_for_activity_suppression\(\)' `
    -and $clearActivityEncounterObservations -match `
        'EventKind::EncounterDefeated' `
    -and $clearActivityEncounterObservations -match `
        'created_encounter_processed_identity_\.fill\(0\)' `
    -and $clearActivityEncounterObservations -match `
        'created_encounter_processed_activation_\.fill\(0\)' `
    -and $applyActivitySuppressionEdge -notmatch 'position_valid_') `
    'Activity-suppression entry must clear encounter evidence and processed identities independently of Pawn sampling.'
Assert-True ($main -match '!activity_suppressed_' `
    -and $main -match 'if \(!observed\.logical_end\)' `
    -and $main -match `
        'lifecycle_valid\s*&&\s*dswros::encounter_cursor_context_allowed' `
    -and $main -match 'observed\.encounter_disappearance\.sample\(' `
    -and $main -match 'observed\.destroyed_end' `
    -and $main -match `
        'found->second\.weak(?:\.Reset\(\)|\s*=\s*FWeakObjectPtr\{\})' `
    -and $main -match 'clear_processed_encounter_identity\(observed\.id\)' `
    -and $main -match 'reason=completion_state_rejected' `
    -and $main -match 'state_applied\s*=\s*mark_encounter_defeated\(observed\.id\)') `
    'Observed-object probing must fail closed during activities, avoid ended UObject calls, and honor idempotent encounter state.'
Assert-True ($saveReconciler -match 'FROM tb_actor_respawn') `
    'The one-shot F7 snapshot must include encounter respawn state.'
Assert-True ($deploy -match 'moles\.lua' -and $deploy -match 'Count = 83' `
    -and $deploy -match 'bosses\.lua' -and $deploy -match 'Count = 9' `
    -and $deploy -match 'assaults\.lua' -and $deploy -match 'Count = 40') `
    'Deployment must validate every restored compact render catalog.'
Assert-True ($main -match 'kTownCompactRenderRadius\s*=\s*12500\.0') `
    'Town minimap projection must preserve the accepted 125-meter radius.'
Assert-True ($main -match 'kFieldCompactRenderRadius\s*=\s*22500\.0') `
    'Field minimap projection must preserve the accepted 225-meter radius.'
Assert-True ($main -match 'kMinimapScaleThreshold\s*=\s*2\.7') `
    'Live minimap projection must preserve the accepted scale threshold.'
Assert-True ($main -match 'kMinimapScaleSampleInterval\s*=\s*std::chrono::seconds\{1\}') `
    'Live minimap scale must not be sampled more frequently than once per second.'
Assert-True ($main -match 'player_, true, compact_render_radius_') `
    'Low-frequency treasure selection must use the live minimap radius.'
Assert-True ($main -match `
        'spec\.kind\s*==\s*EncounterKind::Boss\s*\?\s*30\.0\s*:\s*27\.0' `
    -and $renderer -match 'kMaximumReferenceMarkerSize\s*=\s*65\.0' `
    -and $main -match `
        'for\s*\([^)]*index\s*=\s*0[^)]*index\s*<\s*encounter_keep[\s\S]*?for\s*\([^)]*index\s*=\s*1[^)]*index\s*<\s*treasure_keep') `
    'Compact Boss/Assault sizing or encounter-below-treasure order no longer matches the world-map visual contract.'
Assert-True ($main -match '/ compact_render_radius_') `
    'High-frequency root translation must use the same live minimap radius.'
Assert-True ($main -notmatch 'kCompactRenderRadius') `
    'The incorrect fixed compact minimap radius must not return.'
Assert-True ($translateCode -match 'root_panel->ProcessEvent\(set_render_translation_') `
    'Chest motion must target the root Canvas, not the Blueprint host.'

Write-Host 'Native compact renderer source gates passed.'
