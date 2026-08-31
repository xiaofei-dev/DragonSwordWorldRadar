[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepakPath,
    [string]$SourceRoot,
    [string]$DropSourceRoot,
    [string]$UsmapPath,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$ReleaseVersion = '1.3.0',
    [switch]$SkipVariantBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$distRoot = Join-Path $projectRoot 'dist'
$inventoryPath = Join-Path $projectRoot 'metadata\targets-x5.json'
$dropInventoryPath = Join-Path $projectRoot 'metadata\drop-targets-x5.json'
$policyPath = Join-Path $projectRoot 'metadata\variants.json'
$readmePath = Join-Path $projectRoot 'docs\RANGE_VARIANTS_README.md'
$legacyCanary = Join-Path $distRoot 'DS_PickupRangeX3Canary_P.pak'
$legacyCanaryManifest = Join-Path $distRoot 'DS_PickupRangeX3Canary_P.build.json'
$bundleName = "DragonSwordPickupRangeExpansion-v$ReleaseVersion.zip"
$bundlePath = Join-Path $distRoot $bundleName
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$workRoot = Join-Path $projectRoot ("runtime\bundle\" + $stamp)
$reproBundlePath = Join-Path $workRoot ($bundleName + '.repro.zip')

foreach ($required in @($inventoryPath, $dropInventoryPath, $policyPath, $readmePath, $legacyCanary, $legacyCanaryManifest)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required bundle input was not found: $required"
    }
}
$repak = (Resolve-Path -LiteralPath $RepakPath).Path
$policy = Get-Content -LiteralPath $policyPath -Raw | ConvertFrom-Json
$inventory = Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
$dropInventory = Get-Content -LiteralPath $dropInventoryPath -Raw | ConvertFrom-Json
if ([string]$policy.release_version -ne $ReleaseVersion -or
    @($policy.supported_multipliers).Count -ne 5 -or
    (Compare-Object -ReferenceObject @(3, 5, 10, 15, 20) -DifferenceObject @($policy.supported_multipliers | ForEach-Object { [int]$_ }))) {
    throw 'The variant policy does not match this release request.'
}
if ([int]$inventory.target_count -ne 50 -or @($inventory.targets).Count -ne 50) {
    throw 'The reviewed inventory must contain exactly 50 targets.'
}
$dropInventoryCount = @($dropInventory.targets).Count
if ([int]$dropInventory.target_count -ne 19 -or $dropInventoryCount -ne 19) {
    throw 'The reviewed drop inventory must contain exactly 19 targets.'
}
$inventoryHash = (Get-FileHash -LiteralPath $inventoryPath -Algorithm SHA256).Hash
$dropInventoryHash = (Get-FileHash -LiteralPath $dropInventoryPath -Algorithm SHA256).Hash
if ($inventoryHash -ne [string]$policy.reviewed_inventory_sha256) {
    throw "The reviewed inventory changed: expected $($policy.reviewed_inventory_sha256), found $inventoryHash"
}
if ($dropInventoryHash -ne [string]$policy.reviewed_drop_inventory_sha256) {
    throw "The reviewed drop inventory changed: expected $($policy.reviewed_drop_inventory_sha256), found $dropInventoryHash"
}

function Assert-ExactHash {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Expected,
        [Parameter(Mandatory)][string]$Description
    )
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "$Description changed: expected $Expected, found $actual"
    }
}

Assert-ExactHash $legacyCanary ([string]$policy.legacy_canary.artifact_sha256) 'Historical x3 canary artifact'
Assert-ExactHash $legacyCanaryManifest ([string]$policy.legacy_canary.build_manifest_sha256) 'Historical x3 canary build manifest'

$buildScript = Join-Path $PSScriptRoot 'Build-Release.ps1'
if (-not $SkipVariantBuild) {
    foreach ($multiplier in @(3, 5, 10, 15, 20)) {
        $arguments = @{
            RepakPath = $repak
            Multiplier = $multiplier
            ReleaseVersion = $ReleaseVersion
        }
        if ($SourceRoot) { $arguments.SourceRoot = $SourceRoot }
        if ($DropSourceRoot) { $arguments.DropSourceRoot = $DropSourceRoot }
        if ($UsmapPath) { $arguments.UsmapPath = $UsmapPath }
        & $buildScript @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "The x$multiplier production build failed."
        }
    }
}

$expectedEntries = @($inventory.targets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
}) + @($dropInventory.targets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
})
$expectedEntries = @($expectedEntries | Sort-Object)
$variantRecords = @()
$bundleFiles = [ordered]@{}
foreach ($multiplier in @(3, 5, 10, 15, 20)) {
    $artifactName = "DS_PickupRangeX${multiplier}_P.pak"
    $manifestName = "DS_PickupRangeX${multiplier}_P.build.json"
    $artifactPath = Join-Path $distRoot $artifactName
    $manifestPath = Join-Path $distRoot $manifestName
    $buildManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $artifactHash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash
    if ([int]$buildManifest.range_multiplier -ne $multiplier -or
        [int]$buildManifest.target_count -ne 69 -or
        [int]$buildManifest.authored_target_count -ne 50 -or
        [int]$buildManifest.drop_item_target_count -ne 19 -or
        [int]$buildManifest.pak_entry_count -ne 138 -or
        [string]$buildManifest.artifact_sha256 -ne $artifactHash -or
        [string]$buildManifest.static_validation -ne 'PASSED') {
        throw "The x$multiplier artifact does not match its build manifest."
    }
    $entries = @(& $repak list $artifactPath | ForEach-Object { $_.Trim().Replace('\', '/') } | Sort-Object)
    if ($LASTEXITCODE -ne 0 -or
        (Compare-Object -ReferenceObject $expectedEntries -DifferenceObject $entries)) {
        throw "The x$multiplier PAK inventory differs from the reviewed target set."
    }
    if (@($entries | Where-Object { $_ -match '(?i)treasure|chest|box|/Art/' }).Count -ne 0) {
        throw "The x$multiplier PAK contains a denied path."
    }
    $bundleFiles[$artifactName] = $artifactPath
    $variantRecords += [ordered]@{
        multiplier = $multiplier
        artifact = $artifactName
        artifact_size_bytes = (Get-Item -LiteralPath $artifactPath).Length
        artifact_sha256 = $artifactHash
        build_manifest = $manifestName
        build_manifest_sha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
        target_count = 69
        authored_target_count = 50
        drop_item_target_count = 19
        pak_entry_count = 138
        treasure_targets = 0
        static_validation = 'PASSED'
        gameplay_acceptance = 'NOT_VALIDATED'
    }
}
$null = New-Item -ItemType Directory -Path $workRoot -Force
$bundleFiles['README.md'] = $readmePath
$checksumPath = Join-Path $workRoot 'SHA256SUMS.txt'
$checksumLines = @(foreach ($multiplier in @(3, 5, 10, 15, 20)) {
    $record = @($variantRecords | Where-Object { [int]$_.multiplier -eq $multiplier })
    if ($record.Count -ne 1) { throw "Could not resolve checksum record for x$multiplier." }
    ([string]$record[0].artifact_sha256) + '  ' + ([string]$record[0].artifact)
})
$checksumLines += (Get-FileHash -LiteralPath $readmePath -Algorithm SHA256).Hash + '  README.md'
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText($checksumPath, (($checksumLines -join "`n") + "`n"), $utf8)
$bundleFiles['SHA256SUMS.txt'] = $checksumPath

function New-DeterministicZip {
    param(
        [Parameter(Mandatory)][System.Collections.IDictionary]$Files,
        [Parameter(Mandatory)][string]$Destination
    )
    Add-Type -AssemblyName System.IO.Compression
    if (Test-Path -LiteralPath $Destination) { Remove-Item -LiteralPath $Destination -Force }
    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    $stream = [IO.File]::Open($Destination, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create, $false)
        try {
            foreach ($entryName in @($Files.Keys | Sort-Object)) {
                $entry = $archive.CreateEntry([string]$entryName, [IO.Compression.CompressionLevel]::NoCompression)
                $entry.LastWriteTime = [DateTimeOffset]::new(1980, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
                $input = [IO.File]::OpenRead([string]$Files[$entryName])
                try {
                    $output = $entry.Open()
                    try { $input.CopyTo($output) } finally { $output.Dispose() }
                } finally { $input.Dispose() }
            }
        } finally { $archive.Dispose() }
    } finally { $stream.Dispose() }
}

New-DeterministicZip $bundleFiles $bundlePath
New-DeterministicZip $bundleFiles $reproBundlePath
$bundleHash = (Get-FileHash -LiteralPath $bundlePath -Algorithm SHA256).Hash
$reproHash = (Get-FileHash -LiteralPath $reproBundlePath -Algorithm SHA256).Hash
if ($bundleHash -ne $reproHash) {
    throw "Two deterministic ZIP builds differ: $bundleHash vs $reproHash"
}

$zipStream = [IO.File]::OpenRead($bundlePath)
try {
    $zip = [IO.Compression.ZipArchive]::new($zipStream, [IO.Compression.ZipArchiveMode]::Read, $false)
    try {
        $actualZipEntries = @($zip.Entries | ForEach-Object FullName | Sort-Object)
    } finally { $zip.Dispose() }
} finally { $zipStream.Dispose() }
$expectedZipEntries = @($bundleFiles.Keys | Sort-Object)
if (Compare-Object -ReferenceObject $expectedZipEntries -DifferenceObject $actualZipEntries) {
    throw 'The standalone ZIP entry set differs from the release plan.'
}

$bundleManifest = [ordered]@{
    schema_version = 1
    version = $ReleaseVersion
    built_at_utc = [DateTime]::UtcNow.ToString('O')
    reviewed_inventory = [IO.Path]::GetFileName($inventoryPath)
    reviewed_inventory_sha256 = (Get-FileHash -LiteralPath $inventoryPath -Algorithm SHA256).Hash
    reviewed_drop_inventory = [IO.Path]::GetFileName($dropInventoryPath)
    reviewed_drop_inventory_sha256 = (Get-FileHash -LiteralPath $dropInventoryPath -Algorithm SHA256).Hash
    target_count_per_variant = 69
    authored_target_count_per_variant = 50
    drop_item_target_count_per_variant = 19
    pak_entry_count_per_variant = 138
    variants = $variantRecords
    bundle = [IO.Path]::GetFileName($bundlePath)
    bundle_size_bytes = (Get-Item -LiteralPath $bundlePath).Length
    bundle_sha256 = $bundleHash
    bundle_entries = $actualZipEntries
    deterministic_zip = $true
    legacy_canary_preserved = $true
    legacy_canary_sha256 = (Get-FileHash -LiteralPath $legacyCanary -Algorithm SHA256).Hash
    treasure_targets = 0
    static_validation = 'PASSED'
    gameplay_acceptance = 'NOT_VALIDATED'
}
$bundleManifestPath = Join-Path $distRoot "DragonSwordPickupRangeExpansion-v$ReleaseVersion.build.json"
[IO.File]::WriteAllText($bundleManifestPath, ($bundleManifest | ConvertTo-Json -Depth 10), $utf8)

Write-Output "BUNDLE $bundlePath"
Write-Output "SHA256 $bundleHash"
Write-Output 'VARIANTS 3,5,10,15,20'
Write-Output 'TARGETS_PER_VARIANT 69'
Write-Output 'AUTHORED_TARGETS_PER_VARIANT 50'
Write-Output 'DROP_ITEM_TARGETS_PER_VARIANT 19'
Write-Output 'ENTRIES_PER_VARIANT 138'
Write-Output "BUILD_MANIFEST $bundleManifestPath"
Write-Output 'STATIC_VALIDATION PASSED; GAMEPLAY_ACCEPTANCE NOT_VALIDATED'
