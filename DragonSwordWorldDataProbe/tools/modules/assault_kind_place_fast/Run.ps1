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
    $runtimeReportDir=Join-Path ([string]$ctx.root) 'runtime\reports'
    $placeRuntime=Join-Path $runtimeReportDir 'assault-runtime-place.tsv'
    $kindRuntime=Join-Path $runtimeReportDir 'assault-runtime-kind.tsv'

    # Runtime table wins. Do not touch the PAK if both snapshots already exist
    # and have at least one data row.
    $runtimeReady=$false
    if((Test-Path -LiteralPath $placeRuntime -PathType Leaf) -and
       (Test-Path -LiteralPath $kindRuntime -PathType Leaf))
    {
        $placeLines=@(Get-Content -LiteralPath $placeRuntime -ErrorAction SilentlyContinue).Count
        $kindLines=@(Get-Content -LiteralPath $kindRuntime -ErrorAction SilentlyContinue).Count
        $runtimeReady=($placeLines -gt 1 -and $kindLines -gt 1)
    }

    if($runtimeReady){
        Write-ModuleResult -Context $ctx -Status 'success' `
            -Metrics @{runtime_table_ready=$true;pak_started=$false} `
            -Outputs @($placeRuntime,$kindRuntime) `
            -Blockers @() `
            -NextAction 'Runtime Place/Kind snapshots already exist; fast PAK fallback skipped.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 0
    }

    # This fast fallback is build-pinned. Unknown builds must not use stored
    # compact-entry assumptions.
    $supportedBuild='85d072028086faac03bd39f783126254e624405cdb2784da04395d829c17a25f'
    if([string]$fingerprintInfo.build_fingerprint -ne $supportedBuild){
        Write-ModuleResult -Context $ctx -Status 'blocked' `
            -Metrics @{runtime_table_ready=$false;pak_started=$false} `
            -Blockers @('fast_kind_place_build_mismatch') `
            -NextAction 'Use runtime DUnexpectedMissionTable on this game build; fast compact fallback is pinned to the prior verified build.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 0
    }

    Write-Host '[Kind/Place fast] Runtime table unavailable; starting two-file fallback (hard timeout 60s)...'

    $probeOut=Join-Path $moduleDir 'kind_place_fast_probe'
    Remove-Item -LiteralPath $probeOut -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $moduleDir 'kind_place_fast.log') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $moduleDir 'kind_place_fast_last_stage.txt') -Force -ErrorAction SilentlyContinue

    $scriptPath=Join-Path $moduleDir 'KindPlaceFastProbe.ps1'
    $stdout=Join-Path ([string]$ctx.output_dir) 'fast-child.stdout.log'
    $stderr=Join-Path ([string]$ctx.output_dir) 'fast-child.stderr.log'
    $args='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "{0}"' -f $scriptPath

    $child=Start-Process `
        -FilePath 'powershell.exe' `
        -ArgumentList $args `
        -WindowStyle Hidden `
        -PassThru `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr

    $completed=$child.WaitForExit(60000)
    if(-not$completed){
        try{$child.Kill()}catch{}

        Write-ModuleResult -Context $ctx -Status 'timeout' `
            -Metrics @{runtime_table_ready=$false;pak_started=$true;hard_timeout_seconds=60} `
            -Outputs @($stdout,$stderr) `
            -Blockers @('Fast Kind/Place fallback exceeded 60 seconds and was terminated.') `
            -NextAction 'Continue using runtime reflection evidence; old full PAK module remains manual-only.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 2
    }

    foreach($name in @(
        'kind_place_fast.log',
        'kind_place_fast_last_stage.txt'
    )){
        $source=Join-Path $moduleDir $name
        if(Test-Path -LiteralPath $source -PathType Leaf){
            Copy-Item -LiteralPath $source -Destination ([string]$ctx.output_dir) -Force
        }
    }

    $manifestSource=Join-Path $probeOut 'extraction_manifest.txt'
    if(Test-Path -LiteralPath $manifestSource -PathType Leaf){
        Copy-Item -LiteralPath $manifestSource `
            -Destination (Join-Path ([string]$ctx.output_dir) 'extraction_manifest.txt') `
            -Force
    }

    $xorDebugSource=Join-Path $probeOut 'xor16_debug'
    if(Test-Path -LiteralPath $xorDebugSource -PathType Container){
        Copy-Item -LiteralPath $xorDebugSource `
            -Destination (Join-Path ([string]$ctx.output_dir) 'xor16_debug') `
            -Recurse -Force
    }

    $evidenceSource=Join-Path $probeOut 'decoder_evidence'
    if(Test-Path -LiteralPath $evidenceSource -PathType Container){
        Copy-Item -LiteralPath $evidenceSource `
            -Destination (Join-Path ([string]$ctx.output_dir) 'decoder_evidence') `
            -Recurse -Force
    }

    $entriesSource=Join-Path $probeOut 'all_game_data_xml_entries.csv'
    if(Test-Path -LiteralPath $entriesSource -PathType Leaf){
        Copy-Item -LiteralPath $entriesSource `
            -Destination (Join-Path ([string]$ctx.output_dir) 'all_game_data_xml_entries.csv') `
            -Force
    }

    $dest=Join-Path ([string]$ctx.output_dir) 'xml'
    Remove-Item -LiteralPath $dest -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $dest | Out-Null

    $sourceXml=Join-Path $probeOut 'xml'
    if(Test-Path -LiteralPath $sourceXml -PathType Container){
        foreach($file in Get-ChildItem -LiteralPath $sourceXml -File -Filter '*.xml'){
            $logical=$file.Name -replace '^\d+_',''
            if($logical -in @('UnexpectedMissionPlaceData.xml','UnexpectedMissionKindData.xml')){
                Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $dest $logical) -Force
            }
        }
    }

    $place=Join-Path $dest 'UnexpectedMissionPlaceData.xml'
    $kind=Join-Path $dest 'UnexpectedMissionKindData.xml'
    $gotPlace=Test-Path -LiteralPath $place -PathType Leaf
    $gotKind=Test-Path -LiteralPath $kind -PathType Leaf

    $status=if($gotPlace -and $gotKind){'success'}elseif($gotPlace -or $gotKind){'partial'}else{'blocked'}
    $blockers=@()
    if(-not$gotPlace){$blockers+='UnexpectedMissionPlaceData.xml'}
    if(-not$gotKind){$blockers+='UnexpectedMissionKindData.xml'}

    $outputs=@($stdout,$stderr)
    foreach($extra in @(
        (Join-Path ([string]$ctx.output_dir) 'kind_place_fast.log'),
        (Join-Path ([string]$ctx.output_dir) 'kind_place_fast_last_stage.txt'),
        (Join-Path ([string]$ctx.output_dir) 'extraction_manifest.txt'),
        (Join-Path ([string]$ctx.output_dir) 'decoder_evidence'),
        (Join-Path ([string]$ctx.output_dir) 'xor16_debug'),
        (Join-Path ([string]$ctx.output_dir) 'all_game_data_xml_entries.csv')
    )){
        if(Test-Path -LiteralPath $extra){$outputs+=$extra}
    }
    if($gotPlace){$outputs+=$place}
    if($gotKind){$outputs+=$kind}

    if($gotPlace -and $gotKind){
        $nextAction='Fast Kind/Place fallback succeeded; merge with build-pinned support tables.'
    }else{
        $nextAction='Use runtime reflection/access diagnostics. Full PAK verifier remains manual-only.'
    }

    Write-ModuleResult -Context $ctx -Status $status `
        -Metrics @{
            runtime_table_ready=$false
            pak_started=$true
            child_exit=$child.ExitCode
            place=$gotPlace
            kind=$gotKind
        } `
        -Outputs $outputs `
        -Blockers $blockers `
        -NextAction $nextAction `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $message
    Write-ModuleResult -Context $ctx -Status 'error' `
        -Blockers @($message) `
        -NextAction 'Review fast Kind/Place fallback logs.' | Out-Null
    exit 1
}
