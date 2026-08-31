[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerExe,
    [Parameter(Mandatory)][string]$SupportedGameExecutable,
    [Parameter(Mandatory)][string]$ExperimentalUE4SSDll,
    [string]$ExperimentalDwmapiDll,
    [string]$WorkingDirectory,
    [switch]$KeepArtifacts
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$testRunner = Join-Path $projectRoot 'installer\tests\InstallerIntegration.Tests.ps1'
if (-not (Test-Path -LiteralPath $testRunner -PathType Leaf)) {
    throw "Installer integration test runner was not found: $testRunner"
}

$runnerOutput = @(& $testRunner `
    -InstallerExe $InstallerExe `
    -SupportedGameExecutable $SupportedGameExecutable `
    -ExperimentalUE4SSDll $ExperimentalUE4SSDll `
    -ExperimentalDwmapiDll $ExperimentalDwmapiDll `
    -WorkingDirectory $WorkingDirectory `
    -KeepArtifacts:$KeepArtifacts)

$results = @($runnerOutput | Where-Object {
    $_ -is [psobject] -and $_.PSObject.Properties.Name -contains 'release_gate'
})
if ($results.Count -ne 1) {
    throw "Installer integration runner returned $($results.Count) release-gate result objects; expected exactly one."
}
$result = $results[0]
if ($result.expected -ne 20 -or $result.passed -ne 20 -or
    $result.failed -ne 0 -or $result.skipped -ne 0 -or
    $result.release_gate -ne 'PASSED') {
    throw "Installer integration release gate did not return 20 passed, 0 failed, 0 skipped."
}
$result
