[CmdletBinding()]
param(
    [switch]$PassThru
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $projectRoot 'metadata\offline-fetchcontent-sources.json'
$stopwatch = [Diagnostics.Stopwatch]::StartNew()

function Get-RequiredPropertyValue {
    param(
        [Parameter(Mandatory)][object]$Object,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Description
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        throw "$Description is missing required property '$Name'."
    }
    $property.Value
}

function Get-Sha256Hex {
    param([Parameter(Mandatory)][byte[]]$Bytes)

    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '')
    }
    finally {
        $sha.Dispose()
    }
}

function Get-StableTreeDigest {
    param([Parameter(Mandatory)][string]$RootPath)

    $rootItem = Get-Item -LiteralPath $RootPath -Force -ErrorAction Stop
    if (-not $rootItem.PSIsContainer) {
        throw "Offline dependency source is not a directory: $RootPath"
    }
    if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Offline dependency root must not be a reparse point: $($rootItem.FullName)"
    }

    $rootFull = [IO.Path]::GetFullPath($rootItem.FullName).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $rootPrefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    $files = [Collections.Generic.SortedDictionary[string, string]]::new(
        [StringComparer]::Ordinal)
    $caseFoldedPaths = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    $pending = [Collections.Generic.Stack[IO.DirectoryInfo]]::new()
    $pending.Push([IO.DirectoryInfo]$rootItem)

    while ($pending.Count -gt 0) {
        $directory = $pending.Pop()
        foreach ($item in Get-ChildItem -LiteralPath $directory.FullName -Force -ErrorAction Stop) {
            # Git administrative data is the sole exclusion, at any depth and
            # whether represented as a directory or a worktree .git file.
            if ([string]::Equals($item.Name, '.git', [StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Offline dependency trees must not contain reparse points: $($item.FullName)"
            }
            if ($item.PSIsContainer) {
                $pending.Push([IO.DirectoryInfo]$item)
                continue
            }

            $fullPath = [IO.Path]::GetFullPath($item.FullName)
            if (-not $fullPath.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Offline dependency file escaped its declared root: $fullPath"
            }
            $relativePath = $fullPath.Substring($rootPrefix.Length).Replace('\', '/')
            $relativePath = $relativePath.Normalize([Text.NormalizationForm]::FormC)
            if ([string]::IsNullOrWhiteSpace($relativePath) -or
                -not $caseFoldedPaths.Add($relativePath)) {
                throw "Offline dependency contains an empty or case-colliding normalized path: $relativePath"
            }
            $files.Add($relativePath, $fullPath)
        }
    }

    $canonical = [Text.StringBuilder]::new()
    [void]$canonical.Append("DSNAP-TREE-SHA256-V1`n")
    [uint64]$totalBytes = 0
    foreach ($pair in $files.GetEnumerator()) {
        $stream = [IO.FileStream]::new(
            $pair.Value,
            [IO.FileMode]::Open,
            [IO.FileAccess]::Read,
            [IO.FileShare]::Read)
        $fileSha = [Security.Cryptography.SHA256]::Create()
        try {
            [uint64]$length = $stream.Length
            $contentHash = ([BitConverter]::ToString($fileSha.ComputeHash($stream))).Replace('-', '')
        }
        finally {
            $fileSha.Dispose()
            $stream.Dispose()
        }
        $pathBytes = [Text.Encoding]::UTF8.GetBytes($pair.Key)
        $pathBase64 = [Convert]::ToBase64String($pathBytes)
        [void]$canonical.Append('F:')
        [void]$canonical.Append($pathBytes.Length.ToString([Globalization.CultureInfo]::InvariantCulture))
        [void]$canonical.Append(':')
        [void]$canonical.Append($pathBase64)
        [void]$canonical.Append(':')
        [void]$canonical.Append($length.ToString([Globalization.CultureInfo]::InvariantCulture))
        [void]$canonical.Append(':')
        [void]$canonical.Append($contentHash)
        [void]$canonical.Append("`n")
        $totalBytes += $length
    }
    [void]$canonical.Append('COUNT:')
    [void]$canonical.Append($files.Count.ToString([Globalization.CultureInfo]::InvariantCulture))
    [void]$canonical.Append("`nBYTES:")
    [void]$canonical.Append($totalBytes.ToString([Globalization.CultureInfo]::InvariantCulture))
    [void]$canonical.Append("`n")

    $canonicalBytes = [Text.Encoding]::UTF8.GetBytes($canonical.ToString())
    [pscustomobject]@{
        Sha256 = Get-Sha256Hex $canonicalBytes
        FileCount = [int64]$files.Count
        TotalBytes = [uint64]$totalBytes
    }
}

if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Offline FetchContent manifest was not found: $manifestPath"
}
try {
    $manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
}
catch {
    throw "Offline FetchContent manifest is not valid JSON: $($_.Exception.Message)"
}

if ((Get-RequiredPropertyValue $manifest 'schema_version' 'Offline FetchContent manifest') -ne 1 -or
    (Get-RequiredPropertyValue $manifest 'algorithm' 'Offline FetchContent manifest') -cne
        'dsnap-tree-sha256-v1' -or
    (Get-RequiredPropertyValue $manifest 'git_exclusion' 'Offline FetchContent manifest') -cne
        'any_path_segment_named_.git_only' -or
    (Get-RequiredPropertyValue $manifest 'reparse_policy' 'Offline FetchContent manifest') -cne
        'reject') {
    throw 'Offline FetchContent manifest schema or hashing policy is unsupported.'
}

$cacheRootRelative = [string](Get-RequiredPropertyValue `
    $manifest 'cache_root' 'Offline FetchContent manifest')
if ($cacheRootRelative -cne '.sdk/fetchcontent-experimentalnested') {
    throw "Offline FetchContent cache_root is not the approved project-local path: $cacheRootRelative"
}
$cacheRoot = [IO.Path]::GetFullPath(
    (Join-Path $projectRoot $cacheRootRelative.Replace('/', '\')))
$cacheRootItem = Get-Item -LiteralPath $cacheRoot -Force -ErrorAction Stop
if (-not $cacheRootItem.PSIsContainer -or
    ($cacheRootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw "Offline FetchContent cache root must be a plain directory: $cacheRoot"
}

$expectedSources = [ordered]@{
    CONCURRENTQUEUE = 'concurrentqueue-src'
    CORROSION = 'corrosion-src'
    FMT = 'fmt-src'
    GLAZE = 'glaze-src'
    GLFW = 'glfw-src'
    ICONFONTCPPHEADERS = 'iconfontcppheaders-src'
    IMGUI = 'imgui-src'
    POLYHOOK2 = 'polyhook2-src'
    RAW_PDB = 'raw_pdb-src'
    ZYDIS = 'zydis-src'
}
$sources = @(Get-RequiredPropertyValue $manifest 'sources' 'Offline FetchContent manifest')
if ($sources.Count -ne $expectedSources.Count) {
    throw "Offline FetchContent manifest must contain exactly $($expectedSources.Count) sources; found $($sources.Count)."
}

$validated = [Collections.Generic.List[object]]::new()
$seenKeys = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$seenDirectories = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$expectedKeys = @($expectedSources.Keys)
$cachePrefix = $cacheRoot.TrimEnd(
    [IO.Path]::DirectorySeparatorChar,
    [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar

for ($index = 0; $index -lt $sources.Count; ++$index) {
    $entry = $sources[$index]
    $description = "Offline FetchContent source entry $index"
    $key = [string](Get-RequiredPropertyValue $entry 'fetchcontent_key' $description)
    $directoryName = [string](Get-RequiredPropertyValue $entry 'directory' $description)
    $expectedSha = [string](Get-RequiredPropertyValue $entry 'sha256' $description)
    [int64]$expectedFileCount = Get-RequiredPropertyValue $entry 'file_count' $description
    [uint64]$expectedTotalBytes = Get-RequiredPropertyValue $entry 'total_bytes' $description

    if ($key -cne $expectedKeys[$index] -or
        -not $seenKeys.Add($key) -or
        -not $expectedSources.Contains($key) -or
        $directoryName -cne [string]$expectedSources[$key] -or
        -not $seenDirectories.Add($directoryName)) {
        throw "$description does not match the exact ordered key-to-directory contract: $key -> $directoryName"
    }
    if ($expectedSha -cnotmatch '^[0-9A-F]{64}$' -or $expectedFileCount -le 0) {
        throw "$description has an invalid SHA-256 or file count."
    }

    $absolutePath = [IO.Path]::GetFullPath((Join-Path $cacheRoot $directoryName))
    if (-not $absolutePath.StartsWith($cachePrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$description escaped the approved cache root: $absolutePath"
    }
    $actual = Get-StableTreeDigest $absolutePath
    if ($actual.Sha256 -cne $expectedSha -or
        $actual.FileCount -ne $expectedFileCount -or
        $actual.TotalBytes -ne $expectedTotalBytes) {
        throw "$description content mismatch for ${key}: expected sha256=$expectedSha files=$expectedFileCount bytes=$expectedTotalBytes; actual sha256=$($actual.Sha256) files=$($actual.FileCount) bytes=$($actual.TotalBytes)."
    }

    $validated.Add([pscustomobject]@{
        FetchContentKey = $key
        Directory = $directoryName
        AbsolutePath = $absolutePath
        Sha256 = $actual.Sha256
        FileCount = $actual.FileCount
        TotalBytes = $actual.TotalBytes
    })
}

$stopwatch.Stop()
if ($PassThru) {
    [pscustomobject]@{
        Status = 'PASSED'
        Algorithm = 'dsnap-tree-sha256-v1'
        ManifestPath = [IO.Path]::GetFullPath($manifestPath)
        CacheRoot = $cacheRoot
        Sources = $validated.ToArray()
        ElapsedMilliseconds = [int64]$stopwatch.ElapsedMilliseconds
    }
} else {
    Write-Output "OFFLINE_FETCHCONTENT_VALIDATION PASSED sources=10 elapsed_ms=$($stopwatch.ElapsedMilliseconds)"
}
