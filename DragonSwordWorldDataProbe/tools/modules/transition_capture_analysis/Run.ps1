param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'

$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$ctx=Import-ModuleContext -ContextPath $ContextPath
$fingerprintInfo=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)

function Resolve-Python {
    $python=Get-Command python -ErrorAction SilentlyContinue
    if($null-ne$python){
        return [pscustomobject]@{File=[string]$python.Source;Prefix=@()}
    }

    $py=Get-Command py -ErrorAction SilentlyContinue
    if($null-ne$py){
        return [pscustomobject]@{File=[string]$py.Source;Prefix=@('-3')}
    }

    throw 'Python 3 was not found.'
}

try{
    Write-ModuleProgress -Context $ctx -Step 1 -Total 6 `
        -Stage 'resolve_python' `
        -Detail 'Locating Python 3.'
    $python=Resolve-Python
    Write-ModuleProgress -Context $ctx -Step 2 -Total 6 `
        -Stage 'locate_runtime_reports' `
        -Detail 'Locating Treasure and Assault runtime TSV reports.'

    $parser=Join-Path $toolsDir 'parsers\Analyze-TransitionCapture.py'
    $reports=Join-Path ([string]$ctx.root) 'runtime\reports'
    $console=Join-Path ([string]$ctx.run_output_dir) 'transition-analysis-console.log'

    Write-ModuleProgress -Context $ctx -Step 3 -Total 6 `
        -Stage 'run_transition_parser' `
        -Detail 'Analyzing Treasure actor/hook and Assault bool transitions.'

    & $python.File @($python.Prefix) `
        $parser `
        --reports $reports `
        --output ([string]$ctx.output_dir) 2>&1 |
            Set-Content -LiteralPath $console -Encoding UTF8

    if($LASTEXITCODE -ne 0){
        throw ('transition_analysis_exit_code='+$LASTEXITCODE)
    }

    Write-ModuleProgress -Context $ctx -Step 4 -Total 6 `
        -Stage 'validate_parser_output' `
        -Detail 'Checking transition-capture-summary.json.'

    $summaryPath=Join-Path ([string]$ctx.output_dir) 'transition-capture-summary.json'
    if(-not(Test-Path -LiteralPath $summaryPath -PathType Leaf)){
        throw 'transition-capture-summary.json was not produced.'
    }

    $summary=Read-JsonUtf8Strict -Path $summaryPath

    Write-ModuleProgress -Context $ctx -Step 5 -Total 6 `
        -Stage 'read_transition_metrics' `
        -Detail ('treasure_candidates='+$summary.treasure.current_candidates+
            '; loaded_treasure_actors='+$summary.treasure.loaded_actor_rows+
            '; observed_open_transitions='+$summary.treasure.opened_after_observed_presence+
            '; treasure_events='+$summary.treasure.hook_event_rows+
            '; assault_changes='+$summary.assault.changed_rows)

    Write-ModuleProgress -Context $ctx -Step 6 -Total 6 `
        -Stage 'finalize_transition_analysis' `
        -Detail 'Writing module result.'

    Write-ModuleResult -Context $ctx -Status 'success' `
        -Metrics @{
            treasure_candidates=[int]$summary.treasure.current_candidates
            treasure_loaded_actors=[int]$summary.treasure.loaded_actor_rows
            treasure_exact_actor_matches=[int]$summary.treasure.exact_loaded_actor_matches
            treasure_observed_open_transitions=[int]$summary.treasure.opened_after_observed_presence
            treasure_selective_absence=[int]$summary.treasure.selective_absence_observed
            treasure_reappearances=[int]$summary.treasure.reappeared_rows
            treasure_hook_events=[int]$summary.treasure.hook_event_rows
            treasure_save_ids=[int]$summary.treasure.unique_save_ids
            assault_current_rows=[int]$summary.assault.current_rows
            assault_changed_rows=[int]$summary.assault.changed_rows
        } `
        -Outputs @($summaryPath) `
        -Blockers @() `
        -NextAction 'Correlate the user-opened chest and user-completed assault with the recorded transition IDs.' `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $message

    Write-ModuleResult -Context $ctx -Status 'error' `
        -Blockers @($message) `
        -NextAction 'Review transition-analysis-console.log and runtime reports.' | Out-Null
    exit 1
}
