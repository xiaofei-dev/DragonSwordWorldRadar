[CmdletBinding()]
param(
    [string]$SourceRoot,
    [string]$DropSourceRoot,
    [string]$UsmapPath,
    [Parameter(Mandatory)][string]$RepakPath,
    [ValidateSet(3, 5, 10, 15, 20)][int]$Multiplier = 5,
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$ReleaseVersion = '1.3.0'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $projectRoot 'metadata\targets-x5.json'
$dropManifestPath = Join-Path $projectRoot 'metadata\drop-targets-x5.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$dropManifest = Get-Content -LiteralPath $dropManifestPath -Raw | ConvertFrom-Json
if (-not $SourceRoot) {
    $SourceRoot = Join-Path $projectRoot ([string]$manifest.source_root)
}
if (-not $DropSourceRoot) {
    $DropSourceRoot = Join-Path $projectRoot ([string]$dropManifest.source_root)
}
if (-not $UsmapPath) {
    $UsmapPath = Join-Path $projectRoot ([string]$dropManifest.engine_mapping)
}
$sourceContentRoot = [IO.Path]::GetFullPath($SourceRoot)
$dropSourceContentRoot = [IO.Path]::GetFullPath($DropSourceRoot)
$usmap = [IO.Path]::GetFullPath($UsmapPath)
$repak = [IO.Path]::GetFullPath($RepakPath)
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$variantName = "X$Multiplier"
$artifactName = "DS_PickupRange${variantName}_P.pak"
$workRoot = Join-Path $projectRoot ("runtime\build-x$Multiplier\" + $stamp)
$stagedContentRoot = Join-Path $workRoot 'DS\Content'
$unpackRoot = Join-Path $workRoot 'unpacked'
$distRoot = Join-Path $projectRoot 'dist'
$artifact = Join-Path $distRoot $artifactName
$secondArtifact = Join-Path $workRoot ("DS_PickupRange${variantName}_P.repro.pak")
$dropPatcherProject = Join-Path $projectRoot 'tools\DropItemRangePatcher\DropItemRangePatcher.csproj'
$dropPatcherDll = Join-Path $projectRoot 'tools\DropItemRangePatcher\bin\Release\net8.0\DropItemRangePatcher.dll'

if (-not (Test-Path -LiteralPath $sourceContentRoot -PathType Container)) {
    throw "Source content root was not found: $sourceContentRoot"
}
if (-not (Test-Path -LiteralPath $dropSourceContentRoot -PathType Container)) {
    throw "Drop-item source content root was not found: $dropSourceContentRoot"
}
if (-not (Test-Path -LiteralPath $usmap -PathType Leaf) -or
    (Get-FileHash -LiteralPath $usmap -Algorithm SHA256).Hash -ne [string]$dropManifest.engine_mapping_sha256) {
    throw "The exact reviewed UE5 mappings file is required: $usmap"
}
if (-not (Test-Path -LiteralPath $repak -PathType Leaf)) {
    throw "repak was not found: $repak"
}
if ([int]$manifest.range_multiplier -ne 5 -or
    [int]$manifest.target_count -ne 50 -or
    @($manifest.targets).Count -ne 50) {
    throw 'The reviewed inventory manifest must identify exactly 50 approved x5 baseline targets.'
}
if ([int]$dropManifest.range_multiplier -ne 5 -or
    [int]$dropManifest.target_count -ne 19 -or
    @($dropManifest.targets).Count -ne 19 -or
    [int]$dropManifest.runtime_interact_type -ne 7 -or
    [string]$dropManifest.component -ne 'SphereOverlapComp') {
    throw 'The reviewed drop-item manifest must identify exactly 19 class-proven type-7 targets.'
}

$targetPaths = @($manifest.targets | ForEach-Object { [string]$_.path })
if (@($targetPaths | Sort-Object -Unique).Count -ne 50) {
    throw 'The release manifest contains duplicate target paths.'
}
$dropTargetPaths = @($dropManifest.targets | ForEach-Object { [string]$_.path })
if (@($dropTargetPaths | Sort-Object -Unique).Count -ne 19 -or
    @(Compare-Object -ReferenceObject $targetPaths -DifferenceObject $dropTargetPaths -IncludeEqual |
        Where-Object SideIndicator -eq '==').Count -ne 0) {
    throw 'The drop-item manifest contains duplicate or overlapping target paths.'
}
foreach ($target in $manifest.targets) {
    $path = [string]$target.path
    if ($path -match '(^|/|\\)\.\.($|/|\\)' -or
        $path.StartsWith('/') -or
        $path.StartsWith('\') -or
        $path -match '(?i)treasure|chest|box') {
        throw "Unsafe or treasure-like target path: $path"
    }
    if ([string]$target.status -ne 'approved_x5_patch_candidate' -or
        [string]$target.component -notin @('Capsule_GEN_VARIABLE', 'Capsule1_GEN_VARIABLE') -or
        [string]$target.class -notin @('normal_gather', 'normal_gather_inherited', 'animal')) {
        throw "Target is not an approved dedicated interaction capsule: $path"
    }
}
foreach ($target in $dropManifest.targets) {
    $path = [string]$target.path
    if ($path -match '(^|/|\\)\.\.($|/|\\)' -or
        $path.StartsWith('/') -or
        $path.StartsWith('\\') -or
        $path -notmatch '^Design/Item/Drop_Item/' -or
        $path -match '(?i)treasure|chest|box') {
        throw "Unsafe or non-drop target path: $path"
    }
}

function ConvertTo-DoubleBytes {
    param([Parameter(Mandatory)][object[]]$Values)
    $bytes = [Collections.Generic.List[byte]]::new()
    foreach ($value in $Values) {
        $bytes.AddRange([BitConverter]::GetBytes([double]$value))
    }
    return $bytes.ToArray()
}

function Find-BytePattern {
    param(
        [Parameter(Mandatory)][byte[]]$Data,
        [Parameter(Mandatory)][byte[]]$Pattern
    )
    $hits = [Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le $Data.Length - $Pattern.Length; $offset++) {
        $matches = $true
        for ($index = 0; $index -lt $Pattern.Length; $index++) {
            if ($Data[$offset + $index] -ne $Pattern[$index]) {
                $matches = $false
                break
            }
        }
        if ($matches) { $hits.Add($offset) }
    }
    return $hits.ToArray()
}

function Assert-Hash {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Expected,
        [Parameter(Mandatory)][string]$Description
    )
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "$Description SHA-256 mismatch: expected $Expected, found $actual"
    }
}

& dotnet restore $dropPatcherProject --locked-mode
if ($LASTEXITCODE -ne 0) {
    throw 'The structured drop-item patcher locked restore failed.'
}
& dotnet build $dropPatcherProject -c Release --no-restore
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $dropPatcherDll -PathType Leaf)) {
    throw 'The structured drop-item patcher did not build successfully.'
}

$buildTargets = @()
foreach ($target in $manifest.targets) {
    $path = [string]$target.path
    $relativeBase = $path.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $sourceUasset = Join-Path $sourceContentRoot ($relativeBase + '.uasset')
    $sourceUexp = Join-Path $sourceContentRoot ($relativeBase + '.uexp')
    foreach ($required in @($sourceUasset, $sourceUexp)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Required source asset was not found: $required"
        }
    }
    Assert-Hash $sourceUasset ([string]$target.source_uasset_sha256) "$path uasset"
    Assert-Hash $sourceUexp ([string]$target.source_uexp_sha256) "$path uexp"

    $oldScale = @($target.old_scale | ForEach-Object { [double]$_ })
    $reviewedX5Scale = @($target.new_scale | ForEach-Object { [double]$_ })
    if ($oldScale.Count -ne 3 -or $reviewedX5Scale.Count -ne 3) {
        throw "Scale vectors must have three values: $path"
    }
    for ($axis = 0; $axis -lt 3; $axis++) {
        if ([Math]::Abs($reviewedX5Scale[$axis] - (5.0 * $oldScale[$axis])) -gt 0.0000001) {
            throw "Reviewed baseline scale is not exactly 5x on axis $axis`: $path"
        }
    }
    $newScale = @($oldScale | ForEach-Object { [double]$_ * [double]$Multiplier })

    $originalBytes = [IO.File]::ReadAllBytes($sourceUexp)
    $oldPattern = ConvertTo-DoubleBytes $oldScale
    $newPattern = ConvertTo-DoubleBytes $newScale
    $oldHits = @(Find-BytePattern $originalBytes $oldPattern)
    $newHitsBefore = @(Find-BytePattern $originalBytes $newPattern)
    if ($oldHits.Count -ne 1 -or $newHitsBefore.Count -ne 0) {
        throw "Unexpected dedicated-capsule signature count for $path (old=$($oldHits.Count), new=$($newHitsBefore.Count))"
    }

    $destinationBase = Join-Path $stagedContentRoot $relativeBase
    New-Item -ItemType Directory -Path (Split-Path -Parent $destinationBase) -Force | Out-Null
    $destinationUasset = $destinationBase + '.uasset'
    $destinationUexp = $destinationBase + '.uexp'
    Copy-Item -LiteralPath $sourceUasset -Destination $destinationUasset -Force

    $patchedBytes = [byte[]]$originalBytes.Clone()
    $patchOffset = [int]$oldHits[0]
    for ($index = 0; $index -lt $newPattern.Length; $index++) {
        $patchedBytes[$patchOffset + $index] = $newPattern[$index]
    }
    if ($patchedBytes.Length -ne $originalBytes.Length) {
        throw "Patched file length changed: $path"
    }
    $changedOffsets = [Collections.Generic.List[int]]::new()
    for ($index = 0; $index -lt $originalBytes.Length; $index++) {
        if ($originalBytes[$index] -ne $patchedBytes[$index]) { $changedOffsets.Add($index) }
    }
    $expectedChangedOffsets = [Collections.Generic.List[int]]::new()
    for ($index = 0; $index -lt $oldPattern.Length; $index++) {
        if ($oldPattern[$index] -ne $newPattern[$index]) {
            $expectedChangedOffsets.Add($patchOffset + $index)
        }
    }
    $changedDifference = Compare-Object `
        -ReferenceObject $expectedChangedOffsets.ToArray() `
        -DifferenceObject $changedOffsets.ToArray()
    if ($changedDifference) {
        throw "Unexpected byte-difference boundary for $path`: $($changedOffsets -join ',')"
    }
    [IO.File]::WriteAllBytes($destinationUexp, $patchedBytes)

    $verifiedBytes = [IO.File]::ReadAllBytes($destinationUexp)
    if (@(Find-BytePattern $verifiedBytes $oldPattern).Count -ne 0 -or
        @(Find-BytePattern $verifiedBytes $newPattern).Count -ne 1) {
        throw "Patched capsule signature verification failed: $path"
    }
    Assert-Hash $destinationUasset ([string]$target.source_uasset_sha256) "$path staged uasset"

    $buildTargets += [ordered]@{
        path = $path
        class = [string]$target.class
        component = [string]$target.component
        range_multiplier = $Multiplier
        old_scale = $oldScale
        new_scale = $newScale
        patch_offset = $patchOffset
        source_uexp_length = $originalBytes.Length
        changed_offsets = $changedOffsets.ToArray()
        source_uasset_sha256 = [string]$target.source_uasset_sha256
        source_uexp_sha256 = [string]$target.source_uexp_sha256
        patched_uexp_sha256 = (Get-FileHash -LiteralPath $destinationUexp -Algorithm SHA256).Hash
        runtime_evidence = [string]$target.runtime_evidence
    }
}

foreach ($target in $dropManifest.targets) {
    $path = [string]$target.path
    $relativeBase = $path.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $sourceUasset = Join-Path $dropSourceContentRoot ($relativeBase + '.uasset')
    $sourceUexp = Join-Path $dropSourceContentRoot ($relativeBase + '.uexp')
    foreach ($required in @($sourceUasset, $sourceUexp)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Required drop-item source asset was not found: $required"
        }
    }
    Assert-Hash $sourceUasset ([string]$target.source_uasset_sha256) "$path uasset"
    Assert-Hash $sourceUexp ([string]$target.source_uexp_sha256) "$path uexp"

    $destinationBase = Join-Path $stagedContentRoot $relativeBase
    New-Item -ItemType Directory -Path (Split-Path -Parent $destinationBase) -Force | Out-Null
    $destinationUasset = $destinationBase + '.uasset'
    $destinationUexp = $destinationBase + '.uexp'
    $patchOutput = @(& dotnet $dropPatcherDll `
        --input $sourceUasset `
        --output $destinationUasset `
        --mappings $usmap `
        --expected-uasset-sha256 ([string]$target.source_uasset_sha256) `
        --expected-uexp-sha256 ([string]$target.source_uexp_sha256) `
        --multiplier $Multiplier)
    if ($LASTEXITCODE -ne 0 -or $patchOutput.Count -ne 1) {
        throw "Structured drop-item patch failed for $path"
    }
    $patchEvidence = $patchOutput[0] | ConvertFrom-Json
    if ([string]$patchEvidence.component -ne 'SphereOverlapComp' -or
        [string]$patchEvidence.property -ne 'RelativeScale3D' -or
        -not [bool]$patchEvidence.binary_equality_after_reload -or
        -not (Test-Path -LiteralPath $destinationUasset -PathType Leaf) -or
        -not (Test-Path -LiteralPath $destinationUexp -PathType Leaf)) {
        throw "Structured drop-item patch evidence failed for $path"
    }

    $buildTargets += [ordered]@{
        path = $path
        class = 'drop_item'
        component = 'SphereOverlapComp'
        range_multiplier = $Multiplier
        patch_strategy = 'structured_relative_scale'
        new_scale = @($Multiplier, $Multiplier, $Multiplier)
        protected_components = @('CapsulePhysicsComp', 'SphereHitComp')
        source_uasset_sha256 = [string]$target.source_uasset_sha256
        source_uexp_sha256 = [string]$target.source_uexp_sha256
        patched_uasset_sha256 = (Get-FileHash -LiteralPath $destinationUasset -Algorithm SHA256).Hash
        patched_uexp_sha256 = (Get-FileHash -LiteralPath $destinationUexp -Algorithm SHA256).Hash
        binary_equality_after_reload = $true
        runtime_evidence = 'class_proven_drop_item_actor_runtime_interact_type_7'
    }
}

New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
foreach ($path in @($artifact, $secondArtifact)) {
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
}

& $repak pack --version V4 --mount-point '../../../DS/Content/' $stagedContentRoot $artifact
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $artifact -PathType Leaf)) {
    throw "repak did not produce the expected x$Multiplier PAK."
}
& $repak pack --version V4 --mount-point '../../../DS/Content/' $stagedContentRoot $secondArtifact
if ($LASTEXITCODE -ne 0) { throw 'The reproducibility PAK build failed.' }
$artifactHash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash
$secondHash = (Get-FileHash -LiteralPath $secondArtifact -Algorithm SHA256).Hash
$artifactContentHashes = @(& $repak hash-list $artifact | Sort-Object)
$secondContentHashes = @(& $repak hash-list $secondArtifact | Sort-Object)
$contentDifference = Compare-Object `
    -ReferenceObject $artifactContentHashes `
    -DifferenceObject $secondContentHashes
if ($contentDifference) {
    throw "Two clean PAK builds did not contain identical entry bytes: $($contentDifference | Out-String)"
}
$actualEntries = @(& $repak list $artifact | ForEach-Object { $_.Trim().Replace('\', '/') } | Sort-Object)
if ($LASTEXITCODE -ne 0) { throw 'Could not list the finished PAK.' }
$allTargets = @($manifest.targets) + @($dropManifest.targets)
$expectedEntries = @($allTargets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
} | Sort-Object)
$entryDifference = Compare-Object -ReferenceObject $expectedEntries -DifferenceObject $actualEntries
if ($entryDifference) {
    throw "Finished PAK entry set differs from the reviewed manifest: $($entryDifference | Out-String)"
}
if (@($actualEntries | Where-Object { $_ -match '(?i)treasure|chest|box|/Art/' }).Count -ne 0) {
    throw 'Finished PAK contains a denied treasure or visual-art path.'
}

& $repak unpack -q -f -o $unpackRoot $artifact
if ($LASTEXITCODE -ne 0) { throw 'Independent unpack of the finished PAK failed.' }
foreach ($entry in $expectedEntries) {
    $stagedPath = Join-Path $workRoot $entry.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $unpackedPath = Join-Path $unpackRoot $entry.Replace('/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath $unpackedPath -PathType Leaf)) {
        throw "Unpacked PAK entry is missing: $entry"
    }
    $stagedHash = (Get-FileHash -LiteralPath $stagedPath -Algorithm SHA256).Hash
    $unpackedHash = (Get-FileHash -LiteralPath $unpackedPath -Algorithm SHA256).Hash
    if ($stagedHash -ne $unpackedHash) {
        throw "Unpacked PAK entry hash mismatch: $entry"
    }
}

$buildManifest = [ordered]@{
    schema_version = 1
    version = "$ReleaseVersion-x$Multiplier"
    built_at_utc = [DateTime]::UtcNow.ToString('O')
    target_manifest_sha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
    drop_target_manifest_sha256 = (Get-FileHash -LiteralPath $dropManifestPath -Algorithm SHA256).Hash
    engine_mapping_sha256 = (Get-FileHash -LiteralPath $usmap -Algorithm SHA256).Hash
    range_multiplier = $Multiplier
    target_count = 69
    authored_target_count = 50
    drop_item_target_count = 19
    pak_entry_count = $actualEntries.Count
    artifact = [IO.Path]::GetFileName($artifact)
    artifact_size_bytes = (Get-Item -LiteralPath $artifact).Length
    artifact_sha256 = $artifactHash
    second_container_sha256 = $secondHash
    deterministic_entry_bytes = $true
    canonical_container_preserved = $false
    canonical_container_sha256 = $null
    mount_point = '../../../DS/Content/'
    targets = $buildTargets
    exclusions = $manifest.exclusions
    static_validation = 'PASSED'
    gameplay_acceptance = 'NOT_VALIDATED'
}
$buildManifestPath = Join-Path $distRoot ("DS_PickupRange${variantName}_P.build.json")
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText($buildManifestPath, ($buildManifest | ConvertTo-Json -Depth 10), $utf8)

Write-Output "ARTIFACT $artifact"
Write-Output "SHA256 $artifactHash"
Write-Output "TARGETS 69"
Write-Output "ENTRIES $($actualEntries.Count)"
Write-Output "BUILD_MANIFEST $buildManifestPath"
Write-Output "WORK_ROOT $workRoot"
Write-Output 'STATIC_VALIDATION PASSED; GAMEPLAY_ACCEPTANCE NOT_VALIDATED'
