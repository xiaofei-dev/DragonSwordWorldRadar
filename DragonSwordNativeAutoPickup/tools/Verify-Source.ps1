[CmdletBinding()]
param([switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Verify-SelectorFirst.ps1')
& (Join-Path $PSScriptRoot 'Verify-Manifest.ps1')
& (Join-Path $PSScriptRoot 'Test-PackageLayout.ps1')
& (Join-Path $PSScriptRoot 'Test-ModsTxtControl.ps1')
if ($SkipBuild) {
    Write-Host 'DragonSwordNativeAutoPickup 1.3.1 ExperimentalNested source, manifest, configurable-hotkey, mods.txt, and package layout gates passed; core build was explicitly skipped.'
} else {
    & (Join-Path $PSScriptRoot 'Build-Core.ps1')
    Write-Host 'DragonSwordNativeAutoPickup 1.3.1 ExperimentalNested source, manifest, configurable-hotkey, mods.txt, package layout, and core tests passed.'
}
Write-Host 'Built-artifact, installer integration, release-package, local-deployment, and exact-candidate gameplay checks are separate gates.'
