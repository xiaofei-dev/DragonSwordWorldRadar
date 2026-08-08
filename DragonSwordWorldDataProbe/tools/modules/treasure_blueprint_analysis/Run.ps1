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
 Write-ModuleProgress -Context $ctx -Step 1 -Total 5 `
  -Stage 'resolve_inputs' `
  -Detail 'Locating runtime BlueprintPath catalog and full UE4SS ObjectDump.'

 $suite=Read-JsonUtf8Strict -Path ([string]$ctx.suite_path)
 $layout=Resolve-GameLayout -ProbeRoot ([string]$ctx.root) -Suite $suite
 $blueprints=Join-Path ([string]$ctx.root) 'runtime\modules\treasure_static_catalog\current\treasure-blueprint-types.tsv'
 $objectDump=Join-Path ([string]$layout.win64) 'UE4SS_ObjectDump.txt'

 if(-not(Test-Path -LiteralPath $blueprints -PathType Leaf)){
  throw 'treasure-blueprint-types.tsv is missing; runtime table enumeration did not complete.'
 }
 if(-not(Test-Path -LiteralPath $objectDump -PathType Leaf)){
  throw 'UE4SS_ObjectDump.txt is missing.'
 }

 Write-ModuleProgress -Context $ctx -Step 2 -Total 5 `
  -Stage 'resolve_python' -Detail 'Locating Python 3.'

 $python=Resolve-Python
 $parser=Join-Path $toolsDir 'parsers\Analyze-TreasureBlueprintActors.py'
 $console=Join-Path ([string]$ctx.run_output_dir) 'treasure-blueprint-analysis-console.log'

 Write-ModuleProgress -Context $ctx -Step 3 -Total 5 `
  -Stage 'analyze_blueprint_classes' `
  -Detail 'Matching exact BlueprintPath basenames/classes against ObjectDump.'

 & $python.File @($python.Prefix) `
  $parser `
  --object-dump $objectDump `
  --blueprints $blueprints `
  --output ([string]$ctx.output_dir) 2>&1 |
   Set-Content -LiteralPath $console -Encoding UTF8

 if($LASTEXITCODE -ne 0){
  throw ('treasure_blueprint_analysis_exit_code='+$LASTEXITCODE)
 }

 Write-ModuleProgress -Context $ctx -Step 4 -Total 5 `
  -Stage 'read_summary' `
  -Detail 'Reading class/function/property match counts.'

 $summaryPath=Join-Path ([string]$ctx.output_dir) 'treasure-blueprint-analysis-summary.json'
 $summary=Read-JsonUtf8Strict -Path $summaryPath

 Write-ModuleProgress -Context $ctx -Step 5 -Total 5 `
  -Stage 'finalize' `
  -Detail ('classes='+$summary.class_count+
   '; functions='+$summary.function_count+
   '; properties='+$summary.property_count)

 $outputs=@(Get-ChildItem -LiteralPath ([string]$ctx.output_dir) -Force |
  ForEach-Object{$_.FullName})

 Write-ModuleResult -Context $ctx `
  -Status ($(if([int]$summary.class_count -gt 0){'success'}else{'partial'})) `
  -Metrics @{
   blueprint_records=[int]$summary.blueprint_record_count
   objectdump_matches=[int]$summary.objectdump_match_count
   classes=[int]$summary.class_count
   functions=[int]$summary.function_count
   properties=[int]$summary.property_count
   unknown_functions_invoked=0
  } `
  -Outputs $outputs `
  -Blockers @() `
  -NextAction ([string]$summary.next_action) `
  -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
}
catch{
 $message=Get-ExceptionSummary -ErrorObject $_
 Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $message
 Write-ModuleResult -Context $ctx -Status 'error' `
  -Blockers @($message) `
  -NextAction 'Review treasure BlueprintPath runtime access and analysis console.'|Out-Null
 exit 1
}
