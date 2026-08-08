param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'
$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')
$ctx=Import-ModuleContext -ContextPath $ContextPath
$fingerprintInfo=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)
try{
 $suite=Read-JsonUtf8Strict -Path ([string]$ctx.suite_path)
 $layout=Resolve-GameLayout -ProbeRoot ([string]$ctx.root) -Suite $suite
 $dump=Join-Path ([string]$layout.win64) 'UE4SS_ObjectDump.txt'
 $parser=Join-Path $toolsDir 'parsers\Analyze-AssaultQuestConditions.py'
 $python=(Get-Command python -ErrorAction SilentlyContinue)
 if($null-eq$python){throw 'python not found'}
 Write-ModuleProgress -Context $ctx -Step 1 -Total 2 -Stage 'scan_objectdump' -Detail 'Quest/Trigger/Weather/DaySwitch/Respawn evidence.'
 & $python.Source $parser --object-dump $dump --output ([string]$ctx.output_dir)
 if($LASTEXITCODE -ne 0){throw 'parser failed'}
 Write-ModuleProgress -Context $ctx -Step 2 -Total 2 -Stage 'finalize' -Detail 'ObjectDump condition evidence written.'
 $outs=@(Get-ChildItem ([string]$ctx.output_dir)-Force|ForEach-Object{$_.FullName})
 Write-ModuleResult -Context $ctx -Status success -Metrics @{unknown_functions_invoked=0} -Outputs $outs -Blockers @() -NextAction 'Correlate trigger registration, environment state and RespawnCycle 105.' -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
}catch{$m=Get-ExceptionSummary -ErrorObject $_;Write-ModuleResult -Context $ctx -Status error -Blockers @($m) -NextAction 'Review quest/condition analysis.'|Out-Null;exit 1}
