param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'
$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
$ctx=Import-ModuleContext -ContextPath $ContextPath
try {
    $output=Join-Path $ctx.output_dir 'example.json'
    Write-JsonUtf8NoBom -Path $output -Value ([ordered]@{ module=$ctx.module_id; run=$ctx.run_id; trigger=$ctx.trigger }) -Depth 4
    Write-ModuleResult -Context $ctx -Status success -Outputs @($output) -NextAction 'Replace the example collector with a read-only method.' | Out-Null
} catch {
    $m=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error -Blockers @($m) | Out-Null
    exit 1
}
