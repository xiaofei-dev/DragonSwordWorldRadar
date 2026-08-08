param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'
$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')
$fingerprintInfo=$null
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

try {
    $python=Resolve-Python
    $runtimeReportDir=Join-Path $ctx.root 'runtime\reports'
    $referenceRoot=Join-Path $ctx.root 'reference\assault-support'
    $referenceMetaPath=Join-Path $referenceRoot 'reference.json'
    $referenceXmlDir=Join-Path $referenceRoot 'xml'
    $fastXmlDir=Join-Path $ctx.root 'runtime\modules\assault_kind_place_fast\current\xml'

    $xmlDir=Join-Path $ctx.run_output_dir 'merged-xml'
    Remove-Item -LiteralPath $xmlDir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $xmlDir | Out-Null

    # Support XML is accepted only for the exact build it came from.
    if((Test-Path -LiteralPath $referenceMetaPath -PathType Leaf) -and
       (Test-Path -LiteralPath $referenceXmlDir -PathType Container))
    {
        $referenceMeta=Read-JsonUtf8Strict -Path $referenceMetaPath
        if([string]$referenceMeta.game_build_fingerprint -eq [string]$fingerprintInfo.build_fingerprint){
            Copy-Item -Path (Join-Path $referenceXmlDir '*.xml') -Destination $xmlDir -Force -ErrorAction SilentlyContinue
            Write-ModuleLog -Context $ctx -Event 'REFERENCE_SUPPORT_SELECTED' `
                -Message ('build_fingerprint='+[string]$fingerprintInfo.build_fingerprint)
        }
        else{
            Write-ModuleLog -Context $ctx -Event 'REFERENCE_SUPPORT_REJECTED' `
                -Message ('reference='+[string]$referenceMeta.game_build_fingerprint+' current='+[string]$fingerprintInfo.build_fingerprint)
        }
    }

    # Fast two-file fallback overrides reference absence for Kind/Place.
    if(Test-Path -LiteralPath $fastXmlDir -PathType Container){
        Copy-Item -Path (Join-Path $fastXmlDir '*.xml') -Destination $xmlDir -Force -ErrorAction SilentlyContinue
    }

    $placeSnapshot=Join-Path $runtimeReportDir 'assault-runtime-place.tsv'
    $kindSnapshot=Join-Path $runtimeReportDir 'assault-runtime-kind.tsv'
    $hasRuntimePlace=Test-Path -LiteralPath $placeSnapshot -PathType Leaf
    $hasRuntimeKind=Test-Path -LiteralPath $kindSnapshot -PathType Leaf
    $hasFastPlace=Test-Path -LiteralPath (Join-Path $xmlDir 'UnexpectedMissionPlaceData.xml') -PathType Leaf
    $hasFastKind=Test-Path -LiteralPath (Join-Path $xmlDir 'UnexpectedMissionKindData.xml') -PathType Leaf

    if((-not($hasRuntimePlace -or $hasFastPlace)) -or
       (-not($hasRuntimeKind -or $hasFastKind)))
    {
        Write-ModuleResult -Context $ctx -Status blocked `
            -Blockers @('Kind/Place unavailable from both runtime snapshot and fast fallback') `
            -NextAction 'Review assault-runtime-access.tsv/schema.tsv and assault_kind_place_fast logs.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 0
    }


    # Canonicalize build-pinned support XML names. The reference package retains
    # extraction sequence prefixes (e.g. 006_SectionMonsterData.xml), while the
    # catalog parser intentionally addresses tables by canonical basename.
    $canonicalTables=@(
        'UnexpectedMissionWorldData',
        'SectionMonsterData',
        'SpawnMonsterGroupData',
        'ActorSpawnConditionData',
        'NPCSpawnConditionData',
        'MonsterCharacterData',
        'ActorPositionData',
        'ActionMonsterLinkData',
        'ObjectActionMonsterLinkData',
        'MiniGameData'
    )
    foreach($table in $canonicalTables){
        $canonical=Join-Path $xmlDir ($table+'.xml')
        if(Test-Path -LiteralPath $canonical -PathType Leaf){continue}

        $matches=@(
            Get-ChildItem -LiteralPath $xmlDir -File -ErrorAction SilentlyContinue |
                Where-Object {$_.Name -match ('^\d+_'+[regex]::Escape($table)+'\.xml$')}
        )
        if($matches.Count -eq 1){
            Copy-Item -LiteralPath $matches[0].FullName -Destination $canonical -Force
            Write-ModuleLog -Context $ctx -Event 'SUPPORT_XML_CANONICALIZED' `
                -Message ($matches[0].Name+' -> '+($table+'.xml'))
        }
        elseif($matches.Count -gt 1){
            throw ('Ambiguous numbered support XML for '+$table+': '+(($matches|ForEach-Object{$_.Name}) -join ','))
        }
    }

$parser=Join-Path $toolsDir 'parsers\Build-AssaultCatalog.py'
    $console=Join-Path $ctx.run_output_dir 'analyzer-console.log'
    & $python.File @($python.Prefix) $parser --xml-dir $xmlDir --target-profile $ctx.target_profile_path --output $ctx.output_dir --runtime-report-dir $runtimeReportDir 2>&1 | Set-Content -LiteralPath $console -Encoding UTF8
    if($LASTEXITCODE -ne 0){throw ('analyzer_exit_code=' + $LASTEXITCODE)}
    $statusPath=Join-Path $ctx.output_dir 'pipeline-status.json'
    if(-not (Test-Path -LiteralPath $statusPath -PathType Leaf)){throw 'pipeline-status.json was not produced'}
    $pipeline=Read-JsonUtf8Strict -Path $statusPath
    $status=[string]$pipeline.status
    if($status -notin @('success','partial','blocked')){$status='error'}
    $outputs=@(Get-ChildItem -LiteralPath $ctx.output_dir -File -ErrorAction SilentlyContinue | ForEach-Object {$_.FullName})
    Write-ModuleResult -Context $ctx -Status $status -Metrics @{ kind_records=[int]$pipeline.kind_records; place_kind_links=[int]$pipeline.place_kind_links; unresolved=[int]$pipeline.unresolved_count } -Outputs $outputs -Blockers @($pipeline.blockers) -NextAction ([string]$pipeline.next_action) -Fingerprint ((Get-Sha256Safe -Path $statusPath)+':assault_catalog_v110') | Out-Null
}
catch{
    $m=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $m
    Write-ModuleResult -Context $ctx -Status error -Blockers @($m) -NextAction 'Review analyzer-console.log and source XML roots.' | Out-Null
    exit 1
}
