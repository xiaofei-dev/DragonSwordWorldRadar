[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $projectRoot 'metadata\source-manifest.json'
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$expected = @($manifest.files | Sort-Object)
$excludedRoots = @(
    '.sdk',
    '.tmp',
    'build',
    'build-native',
    'build-native-experimental',
    'build-native-stable',
    'build-audit-experimental',
    'build-audit-stable',
    'build-installer',
    'dist',
    'out',
    'package',
    'runtime',
    'staging'
)
$actual = @(Get-ChildItem -LiteralPath $projectRoot -Recurse -File -Force | Where-Object {
    $relative = $_.FullName.Substring($projectRoot.Length).TrimStart('\')
    $first = $relative.Split('\')[0]
    $isGeneratedBuildRoot = $first -like 'build-*'
    $isNestedInstallerBuild = $relative -like 'installer\build\*'
    $isLocalPublishingMaterial = $relative -like 'assets\nexus\*' -or
        $relative -like 'assets\screenshots\*'
    $excludedRoots -notcontains $first -and
        -not $isGeneratedBuildRoot -and
        -not $isLocalPublishingMaterial -and
        -not $isNestedInstallerBuild
} | ForEach-Object {
    $_.FullName.Substring($projectRoot.Length).TrimStart('\').Replace('\', '/')
} | Sort-Object)

$difference = Compare-Object -ReferenceObject $expected -DifferenceObject $actual
if ($difference) {
    throw "Source manifest mismatch:`n$($difference | Format-Table | Out-String)"
}
Write-Host "Source manifest passed: $($actual.Count) files."
