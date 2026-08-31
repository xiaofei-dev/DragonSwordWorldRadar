[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InstallerExe,
    [Parameter(Mandatory = $true)][string]$SupportedGameExecutable,
    [Parameter(Mandatory = $true)][string]$ExperimentalUE4SSDll,
    [Parameter(Mandatory = $true)][string]$ExperimentalDwmapiDll,
    [Parameter(Mandatory = $true)][string]$WorkingDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$testScript = Join-Path $projectRoot 'installer\tests\InstallerExperimentalConversion.Tests.ps1'
if (-not (Test-Path -LiteralPath $testScript -PathType Leaf)) {
    throw "Installer integration test script is missing: $testScript"
}

$result = @(& $testScript `
    -InstallerExe $InstallerExe `
    -SupportedGameExecutable $SupportedGameExecutable `
    -ExperimentalUE4SSDll $ExperimentalUE4SSDll `
    -ExperimentalDwmapiDll $ExperimentalDwmapiDll `
    -WorkingDirectory $WorkingDirectory)

if ($result.Count -ne 1) {
    throw "Installer integration runner returned $($result.Count) pipeline objects; expected one result."
}
$summary = $result[0]
if ([int]$summary.expected -ne 20 -or
    [int]$summary.passed -ne 20 -or
    [int]$summary.failed -ne 0 -or
    [int]$summary.skipped -ne 0 -or
    [string]$summary.release_gate -ne 'PASSED' -or
    -not [bool]$summary.sources_unchanged -or
    -not [bool]$summary.fixtures_cleaned) {
    $failedNames = @($summary.details |
        Where-Object { $_.status -eq 'FAILED' } |
        ForEach-Object { $_.name })
    $failureText = if ($failedNames.Count -eq 0) {
        'No individual failure name was returned; inspect source-state and cleanup fields.'
    } else {
        'Failed cases: ' + ($failedNames -join '; ')
    }
    throw ('Installer release gate requires exactly 20 passed, 0 failed, and 0 skipped. ' +
        "Actual: $($summary.passed) passed, $($summary.failed) failed, " +
        "$($summary.skipped) skipped. $failureText")
}

$summary
