[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepakPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$distRoot = Join-Path $projectRoot 'dist'
$releasePath = Join-Path $projectRoot 'metadata\release.json'
$policyPath = Join-Path $projectRoot 'metadata\variants.json'
$inventoryPath = Join-Path $projectRoot 'metadata\targets-x5.json'
$dropInventoryPath = Join-Path $projectRoot 'metadata\drop-targets-x5.json'
$release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
$policy = Get-Content -LiteralPath $policyPath -Raw | ConvertFrom-Json
$inventory = Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
$dropInventory = Get-Content -LiteralPath $dropInventoryPath -Raw | ConvertFrom-Json
$repak = (Resolve-Path -LiteralPath $RepakPath).Path

if ((Get-FileHash -LiteralPath $repak -Algorithm SHA256).Hash -ne [string]$policy.repak_sha256) {
    throw "The exact reviewed repak build is required: $repak"
}

if ([string]$release.version -ne [string]$policy.release_version -or
    [string]$policy.authored_target_multiplier_policy -ne 'selected_variant' -or
    [string]$policy.drop_item_multiplier_policy -ne 'min_selected_variant_and_cap' -or
    [int]$policy.drop_item_multiplier_cap -ne 10 -or
    [string]$release.authored_target_multiplier_policy -ne 'selected_variant' -or
    [string]$release.drop_item_multiplier_policy -ne 'min_selected_variant_and_cap' -or
    [int]$release.drop_item_multiplier_cap -ne 10 -or
    [string]$release.repak_sha256 -ne [string]$policy.repak_sha256 -or
    [int]$release.target_count -ne 69 -or
    [int]$release.authored_target_count -ne 50 -or
    [int]$release.drop_item_target_count -ne 19 -or
    [int]$release.pak_entry_count -ne 138 -or
    @($release.artifacts).Count -ne 5) {
    throw 'Release metadata does not describe the expected five-variant release.'
}
$expectedEntries = @($inventory.targets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
}) + @($dropInventory.targets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
})
$expectedEntries = @($expectedEntries | Sort-Object)
$buildManifests = @{}
$pakEntryHashes = @{}

foreach ($stableMultiplier in @(3, 5, 10)) {
    $stableRecord = @($release.artifacts | Where-Object { [int]$_.multiplier -eq $stableMultiplier })
    $stableHashProperty = $policy.unchanged_artifact_hashes.PSObject.Properties[[string]$stableMultiplier]
    if ($stableRecord.Count -ne 1 -or $null -eq $stableHashProperty -or
        [string]$stableRecord[0].sha256 -ne [string]$stableHashProperty.Value -or
        [string]$stableRecord[0].reused_from_version -ne [string]$policy.unchanged_artifact_source_version) {
        throw "The immutable x$stableMultiplier artifact was not preserved from the approved release."
    }
}

foreach ($artifactRecord in @($release.artifacts | Sort-Object multiplier)) {
    $multiplier = [int]$artifactRecord.multiplier
    if ($multiplier -notin @(3, 5, 10, 15, 20)) { throw "Unsupported release multiplier: $multiplier" }
    $artifactPath = Join-Path $distRoot ([string]$artifactRecord.name)
    $buildManifestPath = [IO.Path]::ChangeExtension($artifactPath, '.build.json')
    $actualHash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash
    if ($actualHash -ne [string]$artifactRecord.sha256 -or
        (Get-Item -LiteralPath $artifactPath).Length -ne [long]$artifactRecord.size_bytes) {
        throw "Release metadata mismatch for x$multiplier."
    }
    $buildManifest = Get-Content -LiteralPath $buildManifestPath -Raw | ConvertFrom-Json
    $expectedDropMultiplier = [Math]::Min($multiplier, [int]$policy.drop_item_multiplier_cap)
    $authoredMultiplierProperty = $buildManifest.PSObject.Properties['authored_range_multiplier']
    $dropMultiplierProperty = $buildManifest.PSObject.Properties['drop_item_range_multiplier']
    $manifestAuthoredMultiplier = if ($null -ne $authoredMultiplierProperty) {
        [int]$authoredMultiplierProperty.Value
    } else {
        [int]$buildManifest.range_multiplier
    }
    $manifestDropMultiplier = if ($null -ne $dropMultiplierProperty) {
        [int]$dropMultiplierProperty.Value
    } else {
        [int]$buildManifest.range_multiplier
    }
    $authoredTargets = @($buildManifest.targets | Where-Object { [string]$_.class -ne 'drop_item' })
    $dropTargets = @($buildManifest.targets | Where-Object { [string]$_.class -eq 'drop_item' })
    if ([int]$buildManifest.range_multiplier -ne $multiplier -or
        [int]$artifactRecord.authored_range_multiplier -ne $multiplier -or
        [int]$artifactRecord.drop_item_range_multiplier -ne $expectedDropMultiplier -or
        $manifestAuthoredMultiplier -ne $multiplier -or
        $manifestDropMultiplier -ne $expectedDropMultiplier -or
        $authoredTargets.Count -ne 50 -or
        $dropTargets.Count -ne 19 -or
        @($authoredTargets | Where-Object { [int]$_.range_multiplier -ne $multiplier }).Count -ne 0 -or
        @($dropTargets | Where-Object { [int]$_.range_multiplier -ne $expectedDropMultiplier }).Count -ne 0 -or
        [string]$buildManifest.artifact_sha256 -ne $actualHash -or
        [string]$buildManifest.static_validation -ne 'PASSED') {
        throw "Build-manifest mismatch for x$multiplier."
    }
    $actualEntries = @(& $repak list $artifactPath | ForEach-Object { $_.Trim().Replace('\', '/') } | Sort-Object)
    if ($LASTEXITCODE -ne 0 -or
        (Compare-Object -ReferenceObject $expectedEntries -DifferenceObject $actualEntries)) {
        throw "PAK inventory mismatch for x$multiplier."
    }
    if (@($actualEntries | Where-Object { $_ -match '(?i)treasure|chest|box|/Art/' }).Count -ne 0) {
        throw "Denied path found in x$multiplier."
    }
    $entryHashMap = @{}
    foreach ($hashLine in @(& $repak hash-list $artifactPath)) {
        if ($hashLine -notmatch '^([0-9a-fA-F]{64})\s+(.+)$') {
            throw "The x$multiplier PAK returned an unrecognized entry-hash record: $hashLine"
        }
        $entryHashMap[$Matches[2].Trim().Replace('\', '/')] = $Matches[1].ToUpperInvariant()
    }
    if ($LASTEXITCODE -ne 0 -or $entryHashMap.Count -ne 138) {
        throw "The x$multiplier PAK entry hashes could not be verified."
    }
    $buildManifests[$multiplier] = $buildManifest
    $pakEntryHashes[$multiplier] = $entryHashMap
}

$x10DropTargets = @{}
foreach ($target in @($buildManifests[10].targets | Where-Object { [string]$_.class -eq 'drop_item' })) {
    $x10DropTargets[[string]$target.path] = $target
}
foreach ($highMultiplier in @(15, 20)) {
    foreach ($target in @($buildManifests[$highMultiplier].targets | Where-Object { [string]$_.class -eq 'drop_item' })) {
        $reference = $x10DropTargets[[string]$target.path]
        if ($null -eq $reference -or
            [string]$target.patched_uasset_sha256 -ne [string]$reference.patched_uasset_sha256 -or
            [string]$target.patched_uexp_sha256 -ne [string]$reference.patched_uexp_sha256) {
            throw "The x$highMultiplier drop manifest differs from the reviewed x10 cap: $($target.path)"
        }
    }
    foreach ($dropTarget in $dropInventory.targets) {
        foreach ($extension in @('.uasset', '.uexp')) {
            $entry = 'DS/Content/' + ([string]$dropTarget.path) + $extension
            if ([string]$pakEntryHashes[$highMultiplier][$entry] -ne
                [string]$pakEntryHashes[10][$entry]) {
                throw "The x$highMultiplier packed drop entry differs from the reviewed x10 cap: $entry"
            }
        }
    }
}

$canaryPath = Join-Path $distRoot ([string]$policy.legacy_canary.artifact)
$canaryManifestPath = Join-Path $distRoot ([string]$policy.legacy_canary.build_manifest)
if ((Get-FileHash -LiteralPath $canaryPath -Algorithm SHA256).Hash -ne [string]$policy.legacy_canary.artifact_sha256 -or
    (Get-FileHash -LiteralPath $canaryManifestPath -Algorithm SHA256).Hash -ne [string]$policy.legacy_canary.build_manifest_sha256) {
    throw 'Historical x3 canary evidence changed.'
}

$bundlePath = Join-Path $distRoot ([string]$release.standalone_bundle.name)
if ((Get-FileHash -LiteralPath $bundlePath -Algorithm SHA256).Hash -ne [string]$release.standalone_bundle.sha256 -or
    (Get-Item -LiteralPath $bundlePath).Length -ne [long]$release.standalone_bundle.size_bytes) {
    throw 'Standalone bundle does not match release metadata.'
}
Add-Type -AssemblyName System.IO.Compression
$stream = [IO.File]::OpenRead($bundlePath)
try {
    $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Read, $false)
    try {
        $zipEntries = @($archive.Entries | ForEach-Object FullName | Sort-Object)
        foreach ($artifactRecord in $release.artifacts) {
            $entry = $archive.GetEntry([string]$artifactRecord.name)
            if ($null -eq $entry) { throw "Bundle artifact is missing: $($artifactRecord.name)" }
            $entryStream = $entry.Open()
            try {
                $sha = [Security.Cryptography.SHA256]::Create()
                try { $entryHash = [BitConverter]::ToString($sha.ComputeHash($entryStream)).Replace('-', '') }
                finally { $sha.Dispose() }
            } finally { $entryStream.Dispose() }
            if ($entryHash -ne [string]$artifactRecord.sha256) {
                throw "Bundled artifact hash mismatch: $($artifactRecord.name)"
            }
        }
        $checksumEntry = $archive.GetEntry('SHA256SUMS.txt')
        if ($null -eq $checksumEntry) { throw 'Bundle checksum file is missing.' }
        $reader = [IO.StreamReader]::new($checksumEntry.Open(), [Text.UTF8Encoding]::new($false))
        try { $actualChecksums = $reader.ReadToEnd().Replace("`r`n", "`n") }
        finally { $reader.Dispose() }
        $expectedChecksumLines = @(foreach ($multiplier in @(3, 5, 10, 15, 20)) {
            $record = @($release.artifacts | Where-Object { [int]$_.multiplier -eq $multiplier })
            if ($record.Count -ne 1) { throw "Release checksum record is ambiguous for x$multiplier." }
            ([string]$record[0].sha256) + '  ' + ([string]$record[0].name)
        })
        $readmeEntry = $archive.GetEntry('README.md')
        if ($null -eq $readmeEntry) { throw 'Bundle README is missing.' }
        $readmeStream = $readmeEntry.Open()
        try {
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $readmeHash = [BitConverter]::ToString($sha.ComputeHash($readmeStream)).Replace('-', '') }
            finally { $sha.Dispose() }
        } finally { $readmeStream.Dispose() }
        $expectedChecksumLines += $readmeHash + '  README.md'
        $expectedChecksums = ($expectedChecksumLines -join "`n") + "`n"
        if ($actualChecksums -ne $expectedChecksums) {
            throw 'SHA256SUMS.txt does not match the declared release artifacts.'
        }
    }
    finally { $archive.Dispose() }
} finally { $stream.Dispose() }
$declaredEntries = @($release.standalone_bundle.entries | ForEach-Object { [string]$_ } | Sort-Object)
if (Compare-Object -ReferenceObject $declaredEntries -DifferenceObject $zipEntries) {
    throw 'Standalone bundle entry set does not match release metadata.'
}

Write-Output 'VARIANT_RELEASE_VALIDATION PASSED'
Write-Output 'MULTIPLIERS 3,5,10,15,20'
Write-Output 'TARGETS_PER_VARIANT 69'
Write-Output 'AUTHORED_TARGETS_PER_VARIANT 50'
Write-Output 'DROP_ITEM_TARGETS_PER_VARIANT 19'
Write-Output 'ENTRIES_PER_VARIANT 138'
Write-Output 'TREASURE_TARGETS 0'
Write-Output 'LEGACY_X3_CANARY PRESERVED'
Write-Output 'GAMEPLAY_ACCEPTANCE NOT_VALIDATED'
