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
    if($null-ne$python){return [pscustomobject]@{File=[string]$python.Source;Prefix=@()}}
    $py=Get-Command py -ErrorAction SilentlyContinue
    if($null-ne$py){return [pscustomobject]@{File=[string]$py.Source;Prefix=@('-3')}}
    throw 'Python 3 was not found.'
}

try{
    Write-ModuleProgress -Context $ctx -Step 1 -Total 4 `
        -Stage 'locate_treasure_static_xml' `
        -Detail 'Looking for PropTreasureBoxData and SectionTreasureBoxData output.'

    $fastRoot=Join-Path ([string]$ctx.root) 'runtime\modules\treasure_static_fast\current\xml'
    $prop=Join-Path $fastRoot 'PropTreasureBoxData.xml'
    $section=Join-Path $fastRoot 'SectionTreasureBoxData.xml'

    if(-not(Test-Path -LiteralPath $prop -PathType Leaf)){
        throw 'PropTreasureBoxData.xml missing.'
    }
    if(-not(Test-Path -LiteralPath $section -PathType Leaf)){
        throw 'SectionTreasureBoxData.xml missing.'
    }

    Write-ModuleProgress -Context $ctx -Step 2 -Total 4 `
        -Stage 'resolve_python' -Detail 'Locating Python 3.'

    $python=Resolve-Python
    $parser=Join-Path $toolsDir 'parsers\Build-TreasureStaticBlueprintCatalog.py'
    $console=Join-Path ([string]$ctx.run_output_dir) 'treasure-static-catalog-console.log'

    Write-ModuleProgress -Context $ctx -Step 3 -Total 4 `
        -Stage 'parse_treasure_static_tables' `
        -Detail 'Building BlueprintPath catalog and explicit Section/Prop joins.'

    & $python.File @($python.Prefix) `
        $parser `
        --prop $prop `
        --section $section `
        --output ([string]$ctx.output_dir) 2>&1 |
            Set-Content -LiteralPath $console -Encoding UTF8

    if($LASTEXITCODE -ne 0){
        throw ('treasure_static_catalog_exit_code='+$LASTEXITCODE)
    }

    $summaryPath=Join-Path ([string]$ctx.output_dir) 'treasure-static-blueprint-summary.json'
    $summary=Read-JsonUtf8Strict -Path $summaryPath

    Write-ModuleProgress -Context $ctx -Step 4 -Total 4 `
        -Stage 'finalize_treasure_static_catalog' `
        -Detail ('prop='+$summary.prop_rows+
            '; section='+$summary.section_rows+
            '; blueprints='+$summary.unique_blueprints+
            '; unique_joins='+$summary.section_unique_join_count)

    $outputs=@(Get-ChildItem -LiteralPath ([string]$ctx.output_dir) -Force |
        ForEach-Object{$_.FullName})

    Write-ModuleResult -Context $ctx -Status 'success' `
        -Metrics @{
            prop_rows=[int]$summary.prop_rows
            section_rows=[int]$summary.section_rows
            unique_blueprints=[int]$summary.unique_blueprints
            section_unique_joins=[int]$summary.section_unique_join_count
            section_unresolved=[int]$summary.section_unresolved_count
        } `
        -Outputs $outputs `
        -Blockers @() `
        -NextAction ([string]$summary.next_action) `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error `
        -Blockers @($message) `
        -NextAction 'Review Treasure static catalog parser inputs/log.'|Out-Null
    exit 1
}
