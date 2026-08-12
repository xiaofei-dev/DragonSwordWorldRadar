[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $projectRoot 'src\overlay'
$sources = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -Filter '*.cs' -File | Sort-Object FullName | Select-Object -ExpandProperty FullName)
if ($sources.Count -lt 40) { throw "Incomplete overlay snapshot: $($sources.Count)" }
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$refs = @(
    [System.Windows.Forms.Form].Assembly.Location,
    [System.Drawing.Graphics].Assembly.Location,
    [System.Linq.Enumerable].Assembly.Location,
    [System.Uri].Assembly.Location
) | Select-Object -Unique
Add-Type -Path $sources -ReferencedAssemblies $refs -ErrorAction Stop
if ([DragonSwordWorldRadar.Program]::InstanceMutexName -ne 'Local\DragonSwordNativeWorldRadar.Overlay') {
    throw 'Overlay mutex isolation failed'
}
Write-Host "OVERLAY_COMPILE_OK sources=$($sources.Count) mutex=$([DragonSwordWorldRadar.Program]::InstanceMutexName)"
