param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'

$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$ctx=Import-ModuleContext -ContextPath $ContextPath
$fingerprintInfo=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)

try{
    $probeRoot=[IO.Path]::GetFullPath([string]$ctx.root).TrimEnd('\')
    $modsRoot=Split-Path -Parent $probeRoot
    $win64=Split-Path -Parent $modsRoot

    $hits=@()
    $runtimeStatePath=Join-Path ([string]$ctx.root) 'runtime\reports\mole-completion-state.tsv'
    $runtimeRows=0
    if(Test-Path -LiteralPath $runtimeStatePath -PathType Leaf){$runtimeRows=@(Get-Content -LiteralPath $runtimeStatePath | Select-Object -Skip 1 | Where-Object{$_ -match '^120\d{2}\t'}).Count}
    $dump=Join-Path $win64 'UE4SS_ObjectDump.txt'

    if(Test-Path -LiteralPath $dump -PathType Leaf){
        $patterns=@(
            'MiniGame.*(Complete|Clear|Finish|State|Save)',
            '(Complete|Clear|Finish).*MiniGame',
            'DsMiniGame',
            'DMiniGameTable',
            'MiniGame_Mole_12',
            'Mole.*(Complete|Clear|Finish|State)'
        )

        foreach($pattern in $patterns){
            $hits += @(
                Select-String -LiteralPath $dump -Pattern $pattern -ErrorAction SilentlyContinue |
                    Select-Object -First 5000 |
                    ForEach-Object {[string]$_.Line}
            )
        }
    }

    $hits=@($hits|Select-Object -Unique)

    $out=Join-Path ([string]$ctx.output_dir) 'mole-completion-state-candidates.txt'
    if($hits.Count-gt0){
        $hits|Set-Content -LiteralPath $out -Encoding UTF8
    }

    $summary=[ordered]@{
        schema_version=1
        known_nodes=40
        id_range='12001-12040'
        completion_state_candidate_lines=$hits.Count
        runtime_completion_rows=$runtimeRows
        confirmed_persistent_store=if($runtimeRows -ge 40){'DETUtil.CIsClearMiniGameInStandAlone'}else{''}
        desired_rule='unfinished => show; completed => hide'
        production_ready=($runtimeRows -ge 40)
        generated_utc=(Get-Date).ToUniversalTime().ToString('o')
    }

    $summaryPath=Join-Path ([string]$ctx.output_dir) 'mole-state-summary.json'
    Write-JsonUtf8NoBom -Path $summaryPath -Value $summary -Depth 8

    $outputs=@($summaryPath)
    if(Test-Path -LiteralPath $runtimeStatePath){$outputs+=$runtimeStatePath}
    if(Test-Path $out){$outputs+=$out}

    $status=if($hits.Count-gt0){'partial'}else{'blocked'}
    $blockers=@()
    if($hits.Count-eq0){
        $blockers+='Persistent completion-state field/table not yet identified.'
    }

    Write-ModuleResult -Context $ctx -Status $status `
        -Metrics @{candidate_lines=$hits.Count;known_nodes=40;runtime_completion_rows=$runtimeRows} `
        -Outputs $outputs `
        -Blockers $blockers `
        -NextAction 'Correlate MiniGame 12001-12040 with one persistent completion state; do not use nearby Actor existence.' `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error `
        -Blockers @($message) `
        -NextAction 'Review mole state discovery logs.' | Out-Null
    exit 1
}
