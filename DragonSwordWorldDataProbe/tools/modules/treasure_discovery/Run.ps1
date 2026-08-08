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
    $suite=Read-JsonUtf8Strict -Path ([string]$ctx.suite_path)
    $layout=Resolve-GameLayout -ProbeRoot ([string]$ctx.root) -Suite $suite

    $objectDump=Join-Path ([string]$layout.win64) 'UE4SS_ObjectDump.txt'
    $runtimeFunctions=Join-Path ([string]$ctx.root) 'runtime\reports\treasure-runtime-functions.tsv'
    $bundledCatalog=Join-Path ([string]$ctx.root) 'reference\treasure\treasures.lua'

    $radarRoot=Join-Path ([string]$layout.mods_root) 'DragonSwordWorldRadar'
    $radarCatalogCandidates=@(
        (Join-Path $radarRoot 'data\treasures.lua'),
        (Join-Path $radarRoot 'data\generated\treasures.lua'),
        $bundledCatalog
    )

    $catalog=$bundledCatalog
    foreach($candidate in $radarCatalogCandidates){
        if(Test-Path -LiteralPath $candidate -PathType Leaf){
            $catalog=$candidate
            break
        }
    }

    $bridgeA=Join-Path $radarRoot 'runtime\bridge\radar_state_a.json'
    $bridgeB=Join-Path $radarRoot 'runtime\bridge\radar_state_b.json'
    $radarLog=Join-Path $radarRoot 'runtime\logs\DragonSwordWorldRadar.Overlay.log'

    $parser=Join-Path $toolsDir 'parsers\Build-TreasureDiscovery.py'
    $console=Join-Path ([string]$ctx.run_output_dir) 'treasure-parser-console.log'
    $python=Resolve-Python

    $arguments=@($python.Prefix)+@(
        $parser,
        '--catalog',$catalog,
        '--output',([string]$ctx.output_dir)
    )

    if(Test-Path -LiteralPath $objectDump -PathType Leaf){
        $arguments+=@('--object-dump',$objectDump)
    }
    if(Test-Path -LiteralPath $runtimeFunctions -PathType Leaf){
        $arguments+=@('--runtime-functions',$runtimeFunctions)
    }
    if(Test-Path -LiteralPath $bridgeA -PathType Leaf){
        $arguments+=@('--bridge-a',$bridgeA)
    }
    if(Test-Path -LiteralPath $bridgeB -PathType Leaf){
        $arguments+=@('--bridge-b',$bridgeB)
    }
    if(Test-Path -LiteralPath $radarLog -PathType Leaf){
        $arguments+=@('--radar-log',$radarLog)
    }

    & $python.File @arguments 2>&1 |
        Set-Content -LiteralPath $console -Encoding UTF8

    if($LASTEXITCODE -ne 0){
        throw ('treasure_parser_exit_code='+$LASTEXITCODE)
    }

    if(Test-Path -LiteralPath $objectDump -PathType Leaf){
        $graphParser=Join-Path $toolsDir 'parsers\Build-TreasureReferenceGraph.py'
        $graphConsole=Join-Path ([string]$ctx.run_output_dir) 'treasure-reference-graph-console.log'

        & $python.File @($python.Prefix) `
            $graphParser `
            --object-dump $objectDump `
            --output ([string]$ctx.output_dir) 2>&1 |
                Set-Content -LiteralPath $graphConsole -Encoding UTF8

        if($LASTEXITCODE -ne 0){
            throw ('treasure_reference_graph_exit_code='+$LASTEXITCODE)
        }

        $openStateParser=Join-Path $toolsDir 'parsers\Build-TreasureOpenStateDiscovery.py'
        $openStateConsole=Join-Path ([string]$ctx.run_output_dir) 'treasure-openstate-console.log'
        & $python.File @($python.Prefix) `
            $openStateParser `
            --object-dump $objectDump `
            --output ([string]$ctx.output_dir) 2>&1 |
                Set-Content -LiteralPath $openStateConsole -Encoding UTF8
        if($LASTEXITCODE -ne 0){
            throw ('treasure_openstate_parser_exit_code='+$LASTEXITCODE)
        }

        $actorChainParser=Join-Path $toolsDir 'parsers\Build-TreasureActorStateChain.py'
        $actorChainConsole=Join-Path ([string]$ctx.run_output_dir) 'treasure-actor-chain-console.log'
        & $python.File @($python.Prefix) `
            $actorChainParser `
            --object-dump $objectDump `
            --output ([string]$ctx.output_dir) 2>&1 |
                Set-Content -LiteralPath $actorChainConsole -Encoding UTF8
        if($LASTEXITCODE -ne 0){
            throw ('treasure_actor_chain_parser_exit_code='+$LASTEXITCODE)
        }
    }

    $summaryPath=Join-Path ([string]$ctx.output_dir) 'treasure-discovery-summary.json'
    if(-not(Test-Path -LiteralPath $summaryPath -PathType Leaf)){
        throw 'treasure-discovery-summary.json was not produced.'
    }

    $summary=Read-JsonUtf8Strict -Path $summaryPath

    $graphSummaryPath=Join-Path ([string]$ctx.output_dir) 'treasure-reference-summary.json'
    $graphSummary=$null
    if(Test-Path -LiteralPath $graphSummaryPath -PathType Leaf){
        $graphSummary=Read-JsonUtf8Strict -Path $graphSummaryPath
    }

    $status=
        if([int]$summary.safe_bool_query_count -gt 0){'success'}
        elseif($null-ne$graphSummary -and [int]$graphSummary.function_candidate_count -gt 0){'partial'}
        elseif([int]$summary.function_candidate_count -gt 0){'partial'}
        else{'blocked'}

    $blockers=@()
    if(-not(Test-Path -LiteralPath $objectDump -PathType Leaf)){
        $blockers+='UE4SS_ObjectDump.txt was not found.'
    }
    if([int]$summary.safe_bool_query_count -eq 0){
        $blockers+='No exact safe bool query has been confirmed yet.'
    }
    if($summary.validation_ready -ne $true){
        $blockers+='Both opened and unopened ground-truth samples are not yet available.'
    }

    $outputs=@(
        Get-ChildItem -LiteralPath ([string]$ctx.output_dir) -Force -ErrorAction SilentlyContinue |
            ForEach-Object {$_.FullName}
    )

    Write-ModuleResult -Context $ctx -Status $status `
        -Metrics @{
            candidates=[int]$summary.function_candidate_count
            safe_bool_queries=[int]$summary.safe_bool_query_count
            catalog_records=[int]$summary.catalog_record_count
            opened_samples=[int]$summary.opened_delta_sample_count
            unopened_samples=[int]$summary.visible_unopened_sample_count
            unknown_functions_invoked=0
            reference_graph_nodes=if($null-ne$graphSummary){[int]$graphSummary.graph_node_count}else{0}
            owner_function_candidates=if($null-ne$graphSummary){[int]$graphSummary.function_candidate_count}else{0}
            query_shaped_owner_functions=if($null-ne$graphSummary){[int]$graphSummary.query_shaped_count}else{0}
        } `
        -Outputs $outputs `
        -Blockers $blockers `
        -NextAction ([string]$summary.next_action) `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $message

    Write-ModuleResult -Context $ctx -Status error `
        -Blockers @($message) `
        -NextAction 'Review treasure-parser-console.log and the full UE4SS ObjectDump path.' | Out-Null
    exit 1
}
