[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Get-Content -LiteralPath (Join-Path $root 'src/native/main.cpp') -Raw
$source = [regex]::Replace([regex]::Replace($source, '/\*[\s\S]*?\*/', ''), '(?m)//[^\r\n]*', '')
function Get-PreviewMethod([string]$code, [string]$name) {
    $match = [regex]::Match($code, '(?m)^    (?:\[\[nodiscard\]\]\s+)?(?:void|bool) ' + $name + '\(')
    if (-not $match.Success) { return '' }
    $start = $code.IndexOf('{', $match.Index)
    $depth = 0
    for ($i = $start; $i -lt $code.Length; $i++) {
        if ($code[$i] -eq '{') { $depth++ }
        if ($code[$i] -eq '}') {
            $depth--
            if ($depth -eq 0) { return $code.Substring($start, $i - $start + 1) }
        }
    }
    return ''
}
function Test-PreviewContract([string]$code) {
    $apply = Get-PreviewMethod $code 'apply_visibility_hub_result'
    $service = Get-PreviewMethod $code 'service_visibility_hub'
    $flush = Get-PreviewMethod $code 'flush_visibility_hub_world_map_refresh'
    $refresh = Get-PreviewMethod $code 'refresh_world_map_after_visibility_hub_change'
    $collect = Get-PreviewMethod $code 'collect_world_map_marker_snapshot'
    return ($apply -match 'const bool already_pending = visibility_hub_world_map_refresh_pending_;' `
        -and $apply -match 'visibility_hub_world_map_refresh_pending_\s*=\s*!visibility_hub_world_map_baseline_valid_[\s\S]*?visibility_hub_world_map_baseline_mask_[\s\S]*?visibility_hub_world_map_baseline_area_quest_mode_[\s\S]*?visibility_hub_world_map_baseline_assault_mode_' `
        -and $apply -match 'if \(visibility_hub_world_map_refresh_pending_ && !already_pending\)\s*\{\s*visibility_hub_world_map_refresh_after_ = Clock::now\(\)\s*\+ std::chrono::milliseconds\{100\};' `
        -and $apply -match 'if \(!world_map_content_visibility_intent\(\)\)\s*\{\s*flush_visibility_hub_world_map_refresh\("all_disabled"\);' `
        -and $service -match 'handle_visibility_hub_result\([\s\S]*?if \(visibility_hub_\.is_open\(\)\s*&& visibility_hub_world_map_refresh_pending_\s*&& now >= visibility_hub_world_map_refresh_after_\)\s*\{\s*flush_visibility_hub_world_map_refresh\("live_preview"\);' `
        -and $flush -match 'refresh_world_map_after_visibility_hub_change\(\);\s*if \(visibility_hub_\.is_open\(\)\)\s*\{\s*establish_visibility_hub_baseline\(\);' `
        -and $refresh -match 'visibility_hub_world_map_refresh_pending_ = false;\s*visibility_hub_world_map_refresh_after_ = \{\};' `
        -and $refresh -match 'if \(!enabled_ \|\| transition_active_\)\s*\{\s*return;' `
        -and $refresh -match 'if \(visibly_open \|\| attached\)' `
        -and $collect -match 'if \(visibility_hub_\.is_open\(\) && visibility_hub_world_map_refresh_pending_\)\s*\{\s*visibility_hub_world_map_baseline_valid_ = false;\s*\}\s*world_map_umg_markers_\.fill' `
        -and $code -notmatch 'deferred_until_close')
}
if (-not (Test-PreviewContract $source)) { throw 'F6 live map preview contract failed.' }
$mutants = @(
    @('&& !already_pending', '&& already_pending'),
    @('std::chrono::milliseconds{100};', 'std::chrono::milliseconds{1000};'),
    @('&& now >= visibility_hub_world_map_refresh_after_', '&& now < visibility_hub_world_map_refresh_after_'),
    @('flush_visibility_hub_world_map_refresh("live_preview");', ';'),
    @('flush_visibility_hub_world_map_refresh("all_disabled");', ';'),
    @('establish_visibility_hub_baseline();', ';'),
    @('if (!enabled_ || transition_active_)', 'if (transition_active_)'),
    @('if (visibly_open || attached)', 'if (attached)'),
    @('if (visibility_hub_.is_open() && visibility_hub_world_map_refresh_pending_)', 'if (false)')
)
foreach ($mutation in $mutants) {
    if (-not $source.Contains($mutation[0])) { throw "Missing preview regression witness: $($mutation[0])" }
    if (Test-PreviewContract ($source.Replace($mutation[0], $mutation[1]))) {
        throw "Live map preview regression accepted: $($mutation[0])"
    }
}
Write-Host 'F6 live map preview passed; 9 regressions rejected; bounded 100 ms preview, external snapshot invalidation, baseline cancellation and immediate all-off verified.'
