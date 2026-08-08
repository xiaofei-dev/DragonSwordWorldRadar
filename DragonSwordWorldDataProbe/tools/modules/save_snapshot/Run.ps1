param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'
$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1'); . (Join-Path $toolsDir 'core\ModuleApi.ps1')
$ctx=Import-ModuleContext -ContextPath $ContextPath
try {
 $suite=Read-JsonUtf8Strict -Path $ctx.suite_path; $layout=Resolve-GameLayout -ProbeRoot $ctx.root -Suite $suite
 $files=@(Get-ChildItem -LiteralPath $layout.saved_root -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '\.(db|bak|sav)$' -or $_.Name -match '\.db-(wal|shm)$' } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 100)
 $out=Join-Path $ctx.output_dir 'files'; Remove-Item -LiteralPath $out -Recurse -Force -ErrorAction SilentlyContinue; New-Item -ItemType Directory -Force -Path $out | Out-Null
 $rows=New-Object System.Collections.Generic.List[object]
 foreach($file in $files){ $target=Join-Path $out $file.Name; Copy-Item -LiteralPath $file.FullName -Destination $target -Force; $rows.Add([pscustomobject]@{source=$file.FullName; output=$target; size=$file.Length; modified_utc=$file.LastWriteTimeUtc.ToString('o')}) }
 $rows|Export-Csv -LiteralPath (Join-Path $ctx.output_dir 'inventory.csv') -NoTypeInformation -Encoding UTF8
 Write-ModuleResult -Context $ctx -Status success -Metrics @{ file_count=$rows.Count } -Outputs @($out) -NextAction 'Compare snapshots only when a save-state question requires it.' | Out-Null
} catch { $m=Get-ExceptionSummary -ErrorObject $_; Write-ModuleResult -Context $ctx -Status error -Blockers @($m) | Out-Null; exit 1 }
