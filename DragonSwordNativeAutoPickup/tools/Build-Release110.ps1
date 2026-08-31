[CmdletBinding()]
param(
    [switch]$SkipNativeBuild,
    [string]$UE4SSRoot,
    [string]$RuntimeZip,
    [string]$InstallerTestGameExecutable,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'

Write-Warning 'Build-Release110.ps1 is a compatibility wrapper. Use Build-Release.ps1 for the canonical 1.3.0 release gate.'
& (Join-Path $PSScriptRoot 'Build-Release.ps1') @PSBoundParameters
