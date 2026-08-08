param([Parameter(Mandatory=$true)][string]$Root)
$ErrorActionPreference='Stop'

$toolsDir=Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $toolsDir 'Common.ps1')

$Root=Resolve-ProbeRoot -PathValue $Root
Assert-ProbeLayout -ProbeRoot $Root

if(@(Get-GameProcessesCompat|Select-Object -First 1).Count -gt 0){
    Write-Host 'PAK verification is disabled while the game is running.' -ForegroundColor Yellow
    Write-Host 'Exit the game first. This step is optional.'
    exit 3
}

$runId='pak-manual-'+(Get-Date -Format 'yyyyMMdd-HHmmss')

Write-Host 'Optional PAK verification only.'
Write-Host 'Normal diagnostics do not run this step.'
Write-Host 'Hard timeout: 180 seconds.'

& (Join-Path $toolsDir 'ModuleHost.ps1') `
    -Root $Root `
    -Trigger 'manual' `
    -RunId $runId
