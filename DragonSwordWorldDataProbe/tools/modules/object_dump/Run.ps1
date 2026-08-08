param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'
$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1'); . (Join-Path $toolsDir 'core\ModuleApi.ps1')
$ctx=Import-ModuleContext -ContextPath $ContextPath
try {
 $suite=Read-JsonUtf8Strict -Path $ctx.suite_path; $layout=Resolve-GameLayout -ProbeRoot $ctx.root -Suite $suite
 $dump=Join-Path $layout.win64 'UE4SS_ObjectDump.txt'
 if(-not(Test-Path -LiteralPath $dump -PathType Leaf)){ Write-ModuleResult -Context $ctx -Status blocked -Blockers @('UE4SS_ObjectDump.txt not found') -NextAction 'Generate an object dump only when runtime schema discovery is needed.' | Out-Null; exit 0 }
 $patterns='UnexpectedMission|DETMonsterSpawn|SpawnCondition|Weather|Climate|RespawnCycle|SectionMonster|DGameDBTableManager'
 $out=Join-Path $ctx.output_dir 'assault-schema-candidates.txt'
 Select-String -LiteralPath $dump -Pattern $patterns -AllMatches|Select-Object -First 50000|ForEach-Object{$_.Line}|Set-Content -LiteralPath $out -Encoding UTF8
 Write-ModuleResult -Context $ctx -Status success -Metrics @{ source_size=(Get-Item $dump).Length } -Outputs @($out) -NextAction 'Use only for class/property schema discovery, not as a full static catalog.' | Out-Null
} catch { $m=Get-ExceptionSummary -ErrorObject $_; Write-ModuleResult -Context $ctx -Status error -Blockers @($m)|Out-Null; exit 1 }
