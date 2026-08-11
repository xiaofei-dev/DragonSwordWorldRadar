[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $projectRoot 'metadata\source-manifest.json'
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$expected = @($manifest.files | Sort-Object)
$excludedRoots = @('.sdk', 'build', 'build-native', 'dist', 'out', 'package', 'runtime', 'staging')
$actual = @(Get-ChildItem -LiteralPath $projectRoot -Recurse -File -Force | Where-Object {
    $relative = $_.FullName.Substring($projectRoot.Length).TrimStart('\')
    $first = $relative.Split('\')[0]
    $excludedRoots -notcontains $first
} | ForEach-Object {
    $_.FullName.Substring($projectRoot.Length).TrimStart('\').Replace('\', '/')
} | Sort-Object)

$difference = Compare-Object -ReferenceObject $expected -DifferenceObject $actual
if ($difference) {
    throw "Source manifest mismatch:`n$($difference | Format-Table | Out-String)"
}
Write-Host "Source manifest passed: $($actual.Count) files."
