[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$guideRoot = Split-Path -Parent $PSScriptRoot

# This is a source contract, not an assertion of in-game Slate/input behavior.
# Inspect only Guide lifecycle and transactions; unrelated UI text and styling
# can change without requiring edits to this safety gate.
function Remove-GuideComments([string]$Text) {
    $withoutBlocks = [regex]::Replace($Text, '/\*[\s\S]*?\*/', '')
    return [regex]::Replace($withoutBlocks, '(?m)^\s*//[^\r\n]*|(?<=;)\s*//[^\r\n]*', '')
}

function Get-GuideBracedBlock([string]$Text, [string]$Anchor) {
    $start = $Text.IndexOf($Anchor, [StringComparison]::Ordinal)
    if ($start -lt 0) { throw "Guide contract anchor missing: $Anchor" }
    $open = $Text.IndexOf('{', $start)
    if ($open -lt 0) { throw "Guide contract block missing: $Anchor" }
    $depth = 1
    for ($end = $open + 1; $end -lt $Text.Length; ++$end) {
        if ($Text[$end] -eq '{') { ++$depth }
        elseif ($Text[$end] -eq '}') { --$depth }
        if ($depth -eq 0) { return $Text.Substring($start, $end - $start + 1) }
    }
    throw "Guide contract block incomplete: $Anchor"
}

function Get-GuideFunction([string]$Text, [string]$Name) {
    $pattern = '(?m)^[^\r\n;{}]*\b' + [regex]::Escape($Name) + '\s*\('
    $match = [regex]::Match($Text, $pattern)
    if (-not $match.Success) { throw "Guide function missing: $Name" }
    return Get-GuideBracedBlock $Text.Substring($match.Index) $Name
}

function Test-GuideContract([string]$Hub, [string]$Header) {
    $h = Remove-GuideComments $Hub
    $headerSource = Remove-GuideComments $Header
    try {
        $atlas = Get-GuideFunction $h 'valid_guide_atlas_file'
        $switch = Get-GuideFunction $h 'RadarVisibilityHub::set_guide_visibility_unsafe'
        $refresh = Get-GuideFunction $h 'RadarVisibilityHub::refresh_guide_unsafe'
        $confirm = Get-GuideFunction $h 'RadarVisibilityHub::set_confirmation_visibility_unsafe'
        $resize = Get-GuideFunction $h 'RadarVisibilityHub::apply_viewport_layout_unsafe'
        $service = Get-GuideFunction $h 'RadarVisibilityHub::service_unsafe'
        $opening = Get-GuideFunction $h 'RadarVisibilityHub::open_unsafe'
        $reset = Get-GuideFunction $h 'RadarVisibilityHub::reset_runtime_handles'
        $detach = Get-GuideFunction $h 'RadarVisibilityHub::detach_unsafe'
        $travel = Get-GuideFunction $h 'RadarVisibilityHub::release_for_travel'
        $languageEdge = Get-GuideBracedBlock $refresh 'if (guide_language_ != resolved_ui_language_)'
        $toggle = Get-GuideBracedBlock $service 'if (guide_toggle_requested && !confirmation_.active())'
        $requiredControls = Get-GuideBracedBlock $opening 'if (!global_reset_control_.Get() || !endorsement_control_.Get()'
        $guideSetupStart = $opening.IndexOf('guide_control_ = add_control(', [StringComparison]::Ordinal)
        $guideSetupEnd = $opening.IndexOf('if (!global_reset_control_.Get() || !endorsement_control_.Get()', [StringComparison]::Ordinal)
        if ($guideSetupStart -lt 0 -or $guideSetupEnd -le $guideSetupStart) { throw 'Guide setup boundary missing' }
        $guideSetup = $opening.Substring($guideSetupStart, $guideSetupEnd - $guideSetupStart)
    } catch {
        Write-Verbose $_.Exception.Message
        return $false
    }
    $checks = @(
        @{Name='atlas extent and RGBA header'; Pass=(
            $h -match 'constexpr double kGuideContentHeight = 1120\.0;' -and
            $h -match 'constexpr double kGuideAtlasHeight = 1184\.0;' -and
            $atlas -match 'width == 1520U && height == 2368U' -and
            $atlas -match 'header\[16\] == 32' -and
            $atlas -match '\(header\[17\] & 0x0FU\) == 8')},
        @{Name='separate saved offsets'; Pass=(
            $headerSource -match 'float settings_scroll_offset_\{\};' -and
            $headerSource -match 'float guide_scroll_offset_\{\};' -and
            $switch -match 'if \(guide_open_\) guide_scroll_offset_ = old;\s*else settings_scroll_offset_ = old;\s*guide_open_ = visible;' -and
            $switch -match 'visible \? guide_scroll_offset_ : settings_scroll_offset_' -and
            $switch -match 'std::isfinite\(offset.value\) \? std::max\(0\.0F, offset.value\) : 0\.0F' -and
            $switch -match 'std::clamp\(static_cast<double>\([\s\S]*?0\.0, maximum\)' -and
            $switch -match 'scroll->ProcessEvent\(set_scroll_offset_, &offset\)')},
        @{Name='page extent and settings input visibility'; Pass=(
            $switch -match 'content_height = visible \? kGuideContentHeight : kFooterTop - kMarkerTop' -and
            $switch -match 'content_height - viewport_layout_\.body_viewport_reference_height' -and
            $switch -match 'content_height \* authored_unit_scale_' -and
            $switch -match 'size->ProcessEvent\(set_height_override_, &height\)' -and
            $switch -match 'set_visibility\(settings, set_visibility_, visible \? kCollapsed : 4\)' -and
            $switch -match 'set_visibility\(guide, set_visibility_, visible \? kHitTestInvisible : kCollapsed\)')},
        @{Name='resize clamps to active page'; Pass=(
            $resize -match 'content_height = guide_open_ \? kGuideContentHeight : kFooterTop - kMarkerTop' -and
            $resize -match 'content_height - layout\.body_viewport_reference_height\) \* authored' -and
            $resize -match 'std::clamp\(static_cast<double>\(offset.value\), 0\.0, maximum\)' -and
            $resize -match 'scroll->ProcessEvent\(set_scroll_offset_, &offset\)')},
        @{Name='guide toggle is a page-only transaction'; Pass=(
            $toggle -match 'set_guide_visibility_unsafe\(!guide_open_\)' -and
            $toggle -notmatch 'pending_|source_|[Rr]eset|[Dd]efault|return\b' -and
            $service -match 'guide_toggle_requested = !close_requested && guide_ready_ && is_checked\(guide, is_checked_\)' -and
            $service -match 'global_reset_requested = !close_requested && confirmed_reset;' -and
            ([regex]::Matches($service, 'set_guide_visibility_unsafe\(!guide_open_\)').Count -eq 1))},
        @{Name='confirmation drains and disables Guide input'; Pass=(
            $confirm -match 'block_background = visible \|\| guard_background' -and
            $confirm -match 'guide_control_\.Get\(\)\)\s*\{\s*set_checked\(guide, set_is_checked_, false\);\s*enable\(guide, !block_background && guide_ready_\)' -and
            $refresh -match 'guide_ready_ && !confirmation_\.active\(\) && !confirmation_dismiss_guard_' -and
            $service -match 'guide_toggle_requested =[^;]+;\s*set_checked\(guide, set_is_checked_, false\)')},
        @{Name='atlas import only at language edge'; Pass=(
            $languageEdge -match 'guide_language_ = resolved_ui_language_;' -and
            $languageEdge -match 'valid_guide_atlas_file\(path\)' -and
            $languageEdge -match 'import_text_overlay_unsafe\(host_\.Get\(\), path\)' -and
            ([regex]::Matches($refresh,'import_text_overlay_unsafe\(').Count -eq 1) -and
            ($refresh.Replace($languageEdge,'') -notmatch 'import_text_overlay_unsafe\('))},
        @{Name='missing guide safely returns to settings'; Pass=(
            $switch -match 'visible = visible && guide_ready_;' -and
            $refresh -match 'guide_ready_ = guide_ready_ && texture;' -and
            $refresh -match 'guide_texture_ = guide_ready_ \? texture : nullptr;' -and
            $refresh -match 'if \(!guide_ready_\)[\s\S]*?SetBrushFromTextureParameters clear\{\};[\s\S]*?set_brush_from_texture_, &clear' -and
            $refresh -match 'guide_ready_ \? kCollapsed : kHitTestInvisible' -and
            $refresh -match 'control->ProcessEvent\(set_is_enabled_, &value\)' -and
            $refresh -match 'set_guide_visibility_unsafe\(guide_open_\);' -and
            $refresh -notmatch 'last_failure_|Rejected|detach_|throw\b')},
        @{Name='optional guide allocation cannot reject F6'; Pass=(
            $guideSetup -notmatch 'last_failure_|Rejected|return\b|throw\b' -and
            $requiredControls -notmatch 'guide_' -and
            $opening -match '\brefresh_guide_unsafe\(\);' -and
            $opening -notmatch 'if\s*\([^{}]*!guide_ready_[^{}]*\)\s*\{[^{}]*(?:last_failure_|Rejected)')},
        @{Name='close and travel clear retained guide state'; Pass=(
            $detach -match 'reset_runtime_handles\(\);' -and
            $travel -match 'reset_runtime_handles\(\);' -and
            $reset -match 'guide_language_ = dswros::RadarUiLanguage::Count;' -and
            $reset -match 'guide_open_ = false;' -and $reset -match 'guide_ready_ = false;' -and
            $reset -match 'settings_scroll_offset_ = 0\.0F;' -and $reset -match 'guide_scroll_offset_ = 0\.0F;')}
    )
    # The actual switch must follow all pending settings reads and precede the
    # comparison/commit. Merely finding both tokens somewhere is insufficient.
    $switchAt = $service.IndexOf('set_guide_visibility_unsafe(!guide_open_);', [StringComparison]::Ordinal)
    $reads = @('pending_scene_settings_.range_meters =', 'pending_scene_settings_.marker_limit =',
        'pending_scene_settings_.distance_mode =', 'pending_masks_ = pack_radar_visibility_masks(',
        'pending_area_quest_mode_ = selected_mode;', 'pending_assault_mode_ = selected_assault_mode;')
    $readsBefore = $switchAt -ge 0
    foreach ($read in $reads) {
        $at = $service.IndexOf($read, [StringComparison]::Ordinal)
        $readsBefore = $readsBefore -and $at -ge 0 -and $at -lt $switchAt
    }
    $changedAt = $service.IndexOf('const bool changed =', [StringComparison]::Ordinal)
    $checks += @{Name='pending settings read before page switch'; Pass=($readsBefore -and $changedAt -gt $switchAt)}
    foreach ($name in @('guide_body_', 'guide_control_', 'guide_fallback_text_', 'guide_texture_')) {
        $checks += @{Name="weak reference $name cleared"; Pass=(
            $headerSource -match ('FWeakObjectPtr ' + [regex]::Escape($name) + '\{\};') -and
            $reset -match ([regex]::Escape($name) + ' = FWeakObjectPtr\{\};'))}
    }
    foreach ($name in @('guide_images_', 'guide_label_canvases_')) {
        $checks += @{Name="weak array $name cleared"; Pass=(
            $reset -match ([regex]::Escape($name) + '\.fill\(FWeakObjectPtr\{\}\);'))}
    }
    $valid = $true
    foreach ($check in $checks) {
        if (-not $check.Pass) { Write-Verbose "Guide clause failed: $($check.Name)"; $valid = $false }
    }
    return $valid
}

$guideSources = @{
    Hub = Get-Content -LiteralPath (Join-Path $guideRoot 'src/native/radar_visibility_hub.cpp') -Raw
    Header = Get-Content -LiteralPath (Join-Path $guideRoot 'src/native/radar_visibility_hub.hpp') -Raw
}
if (-not (Test-GuideContract @guideSources)) { throw 'Guide page/lifecycle source contract failed' }
$guideMutants = @(
    @{From='kGuideContentHeight = 1120.0'; To='kGuideContentHeight = 1024.0'; Name='guide body clips translated content'},
    @{From='visible ? guide_scroll_offset_ : settings_scroll_offset_'; To='0.0F'; Name='switch discards saved offsets'},
    @{From='guide_open_ ? kGuideContentHeight : kFooterTop - kMarkerTop'; To='kFooterTop - kMarkerTop'; Name='resize clamps Guide to Settings extent'},
    @{From='if (index == 0) pending_scene_settings_.range_meters ='; To='set_guide_visibility_unsafe(!guide_open_); if (index == 0) pending_scene_settings_.range_meters ='; Name='page switches during settings sampling'},
    @{From='enable(guide, !block_background && guide_ready_)'; To='enable(guide, guide_ready_)'; Name='guide input remains active behind confirmation'},
    @{From='if (guide_language_ != resolved_ui_language_)'; To='if (true)'; Name='atlas reimports on each refresh'},
    @{From='visible = visible && guide_ready_;'; To='visible = visible;'; Name='missing atlas keeps empty Guide visible'},
    @{From='guide_ready_ = false;'; To='guide_ready_ = true;'; Name='close retains stale ready flag'}
)
foreach ($guideMutant in $guideMutants) {
    if (-not $guideSources.Hub.Contains($guideMutant.From)) { throw "Guide mutation anchor missing: $($guideMutant.Name)" }
    $guideChanged = @{} + $guideSources
    $guideChanged.Hub = $guideChanged.Hub.Replace($guideMutant.From, $guideMutant.To)
    if (Test-GuideContract @guideChanged) { throw "Guide regression accepted: $($guideMutant.Name)" }
}
Write-Output "Guide page/lifecycle contract passed; $($guideMutants.Count) regressions rejected."
