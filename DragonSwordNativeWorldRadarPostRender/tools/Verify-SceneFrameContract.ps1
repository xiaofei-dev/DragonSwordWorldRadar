[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
function Get-Sg09Block([string]$Code, [string]$Pattern) {
    $match = [regex]::Match($Code, $Pattern)
    if (-not $match.Success) { return '' }
    $start = $Code.IndexOf('{', $match.Index + $match.Length)
    if ($start -lt 0) { return '' }
    $depth = 0
    for ($i = $start; $i -lt $Code.Length; ++$i) {
        if ($Code[$i] -eq '{') { ++$depth }
        if ($Code[$i] -eq '}') {
            --$depth
            if ($depth -eq 0) { return $Code.Substring($start, $i - $start + 1) }
        }
    }
    return ''
}
function Read-Sg09Source([string]$Path) {
    $text = Get-Content -LiteralPath (Join-Path $projectRoot $Path) -Raw
    return [regex]::Replace([regex]::Replace($text, '/\*[\s\S]*?\*/', ''), '//[^\r\n]*', '')
}
function Test-Sg09Contract([string]$Main, [string]$Renderer, [string]$Menu, [string]$Hub) {
    $tick = Get-Sg09Block $Main 'void\s+engine_tick_unsafe\(UEngine\* engine\)'
    $control = Get-Sg09Block $tick 'if\s*\(now\s*>=\s*next_position_\)'
    $scene = Get-Sg09Block $Main 'bool\s+scene_render_suppressed\(\)\s+const\s+noexcept'
    $compact = Get-Sg09Block $Main 'bool\s+compact_render_suppressed\(\)\s+const\s+noexcept'
    $update = Get-Sg09Block $Renderer 'bool\s+SceneUmgRenderer::update_unsafe\([^)]*\)'
    $apply = Get-Sg09Block $Main 'void\s+apply_visibility_hub_result\([^)]*\)\s+noexcept'
    if (-not $control -or -not $update -or -not $apply) { return $false }
    if ([regex]::Matches($tick, 'now\s*>=\s*next_position_').Count -ne 1 -or
        [regex]::Matches($tick, 'service_scene_guidance\(engine,\s*now\)').Count -ne 1 -or
        $control -match 'service_scene_guidance' -or
        $tick -notmatch 'RegisterEngineTickPostCallback|profile_stage\(EngineTickProfileStage::SceneUmg') { return $false }
    if ($update -notmatch 'refresh_scene_frame_selection\(player,\s*markers,\s*settings_\)' -or
        $update -match 'select_scene_markers|CanvasPanelSlot|vector_call\(slot,\s*position_' -or
        $update -notmatch 'vector_call\(group,\s*render_translation_,' -or
        $Renderer -notmatch 'resolve\(render_translation_,\s*L"/Script/UMG.Widget:SetRenderTranslation",\s*16\)') { return $false }
    foreach ($body in @($scene, $compact)) {
        if ($body -notmatch 'position_valid_' -or $body -notmatch 'game_paused_' -or
            $body -notmatch 'activity_suppressed_' -or $body -notmatch 'world_map_compact_suppressed_' -or
            $body -notmatch 'native_minimap_paint_\s*==\s*dswros::NativeMinimapPaint::Hidden' -or
            $body -notmatch 'visibility_hub_\.owns_gameplay_cursor\(\)\s*&&\s*native_minimap_paint_\s*==\s*dswros::NativeMinimapPaint::Visible' -or
            $body -match 'visibility_hub_\.is_open\(') { return $false }
    }
    return $Menu -match '\(state\.mouse_cursor_visible\s*&&\s*!state\.settings_cursor_only\)' -and
        $Hub -match 'owns_gameplay_cursor\(\)\s+const\s+noexcept\s*\{\s*return is_open\(\)\s*&&\s*owns_input_mode_;' -and
        $apply -match 'scene_umg_renderer_\.set_menu_suppressed\(scene_render_suppressed\(\)\);'
}
$main = Read-Sg09Source 'src/native/main.cpp'
$renderer = Read-Sg09Source 'src/native/scene_umg_renderer.cpp'
$menu = Read-Sg09Source 'include/dswros/compact_menu_state.hpp'
$hub = Read-Sg09Source 'src/native/radar_visibility_hub.hpp'
if (-not (Test-Sg09Contract $main $renderer $menu $hub)) { throw 'SG09 frame scheduling / live settings contract failed' }
$mutants = @(
    @{Part='Main'; From='service_scene_guidance(engine, now);'; To='if (now >= next_position_) service_scene_guidance(engine, now);'},
    @{Part='Renderer'; From='refresh_scene_frame_selection(player, markers, settings_)'; To='select_scene_markers(player, markers, selection_, settings_)'},
    @{Part='Renderer'; From='vector_call(group, render_translation_,'; To='vector_call(slot, position_,'},
    @{Part='Main'; From='visibility_hub_.owns_gameplay_cursor()'; To='visibility_hub_.is_open()'},
    @{Part='Main'; From='native_minimap_paint_ == dswros::NativeMinimapPaint::Visible'; To='native_minimap_paint_ != dswros::NativeMinimapPaint::Hidden'},
    @{Part='Main'; From='scene_umg_renderer_.set_menu_suppressed(scene_render_suppressed());'; To='scene_umg_renderer_.set_menu_suppressed(true);'},
    @{Part='Menu'; From='(state.mouse_cursor_visible && !state.settings_cursor_only)'; To='state.mouse_cursor_visible'},
    @{Part='Menu'; From='(state.mouse_cursor_visible && !state.settings_cursor_only)'; To='false'},
    @{Part='Hub'; From='return is_open() && owns_input_mode_;'; To='return is_open();'},
    @{Part='Main'; From='position_valid_, mouse_cursor_visible_, world_map_compact_suppressed_,'; To='position_valid_, mouse_cursor_visible_, false,'}
)
foreach ($mutant in $mutants) {
    $parts = @{Main=$main; Renderer=$renderer; Menu=$menu; Hub=$hub}
    if (-not $parts[$mutant.Part].Contains($mutant.From)) { throw "Missing SG09 negative witness: $($mutant.From)" }
    $parts[$mutant.Part] = $parts[$mutant.Part].Replace($mutant.From, $mutant.To)
    if (Test-Sg09Contract $parts.Main $parts.Renderer $parts.Menu $parts.Hub) { throw "SG09 negative witness accepted: $($mutant.From)" }
}
Write-Host 'Scene frame/live-settings contract passed; 10 regressions rejected.'
