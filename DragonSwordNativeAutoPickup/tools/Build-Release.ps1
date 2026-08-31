[CmdletBinding()]
param(
    [switch]$SkipNativeBuild,
    [string]$UE4SSRoot,
    [string]$RuntimeZip,
    [string]$InstallerTestGameExecutable,
    [string]$InstallerBuildDirectory,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$workspaceRoot = Split-Path -Parent $projectRoot
$rangeRoot = Join-Path $workspaceRoot 'DragonSwordPickupRangeExpansion'
$runtimeProject = Join-Path $workspaceRoot 'DragonSwordUE4SSCompatibilityRuntime'
$version = '1.3.0'
$rangeBundleSourceVersion = '1.3.0'
$runtimeLabel = 'DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0'
$runtimeHash = 'AB765EF93BD0DB109D7224C0E2487C68A1CE8748F20B597128D43E247B4AFA77'
$ue4ssDllHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$dwmapiHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$rangeBundleHash = '504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801'
$rangeHashes = [ordered]@{
    '3' = '6BB99A1E35C06EB0284370B9D7BD2F34E90CB6DCA7479CF10A477C68EA0103E8'
    '5' = 'DB9E129D8F8FCCA025864EC908C13C70F950AD779C37CF13A41164476587CECD'
    '10' = '6A1ADB7592BA0C70A17984DB3AC01348086AABE196F0FDAF914B3F52C7A395F1'
    '15' = '16CA8F2353D40BCED8ACBBC95FE4CE8A57E304DEB76D758517E716FF43740100'
    '20' = 'C10E1B252849B1D5DE5C94F468B60841E412487055AB7115B2E73E0D50AFF9BA'
}

if (-not $UE4SSRoot) { $UE4SSRoot = Join-Path $projectRoot '.sdk\RE-UE4SS' }
if (-not $RuntimeZip) {
    $RuntimeZip = Join-Path $runtimeProject 'dist\UE4SS-v3.0.1-Beta0-g1c1a1497-DragonSword-Compatibility-Runtime.zip'
}
if (-not $InstallerBuildDirectory) {
    $InstallerBuildDirectory = Join-Path $projectRoot 'out\installer\1.3.0'
}
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'dist\releases\1.3.0' }

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Resolve-RequiredFile {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Description)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
    (Resolve-Path -LiteralPath $Path).Path
}

function Get-Sha256 {
    param([Parameter(Mandatory)][string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Get-StreamSha256 {
    param([Parameter(Mandatory)][IO.Stream]$Stream)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { ([BitConverter]::ToString($sha.ComputeHash($Stream))).Replace('-', '') }
    finally { $sha.Dispose() }
}

function Reset-SafeDirectory {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$AllowedParent)
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    $parent = [IO.Path]::GetFullPath($AllowedParent).TrimEnd('\')
    $prefix = $parent + '\'
    if ($full -eq $parent -or
        -not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to reset a directory outside the approved parent: $full"
    }
    if (Test-Path -LiteralPath $full) {
        $rootItem = Get-Item -LiteralPath $full -Force
        $reparseMask = [IO.FileAttributes]::ReparsePoint
        if (-not $rootItem.PSIsContainer -or
            ($rootItem.Attributes -band $reparseMask) -ne 0 -or
            @(Get-ChildItem -LiteralPath $full -Recurse -Force |
                Where-Object { ($_.Attributes -band $reparseMask) -ne 0 }).Count -ne 0) {
            throw "Refusing to reset a non-plain generated directory tree: $full"
        }
        Remove-Item -LiteralPath $full -Recurse -Force
    }
    New-Item -ItemType Directory -Path $full -Force | Out-Null
    $full
}

function Copy-DirectoryContents {
    param([Parameter(Mandatory)][string]$Source, [Parameter(Mandatory)][string]$Destination)
    foreach ($item in Get-ChildItem -LiteralPath $Source -Force) {
        Copy-Item -LiteralPath $item.FullName -Destination $Destination -Recurse -Force
    }
}

function Get-RelativeFiles {
    param([Parameter(Mandatory)][string]$Root)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    @(
        Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File |
            ForEach-Object {
                $_.FullName.Substring($rootFull.Length).TrimStart('\').Replace('\', '/')
            } |
            Sort-Object
    )
}

function Assert-ExactSet {
    param(
        [Parameter(Mandatory)][string[]]$Expected,
        [Parameter(Mandatory)][string[]]$Actual,
        [Parameter(Mandatory)][string]$Description
    )
    $difference = Compare-Object -ReferenceObject @($Expected | Sort-Object) -DifferenceObject @($Actual | Sort-Object)
    if ($difference) {
        throw "$Description has an unexpected entry set:`n$($difference | Format-Table | Out-String)"
    }
}

function Test-IsReleaseTextPath {
    param([Parameter(Mandatory)][string]$RelativePath)
    $name = [IO.Path]::GetFileName($RelativePath)
    $extension = [IO.Path]::GetExtension($RelativePath).ToLowerInvariant()
    $extension -in @('.md', '.txt', '.ini', '.lua', '.json', '.license') -or
        $name -match '^(?i:license|notice)(\..*)?$'
}

function Test-IsPinnedUpstreamTextPath {
    param([Parameter(Mandatory)][string]$RelativePath)
    $normalized = $RelativePath.Replace('\', '/')
    $normalized.StartsWith('ue4ss/Mods/shared/', [StringComparison]::OrdinalIgnoreCase) -or
        $normalized.StartsWith('ue4ss/licenses/', [StringComparison]::OrdinalIgnoreCase)
}

function Assert-AsciiBytes {
    param([Parameter(Mandatory)][byte[]]$Bytes, [Parameter(Mandatory)][string]$Description)
    foreach ($value in $Bytes) {
        if ($value -gt 0x7F) { throw "$Description contains non-ASCII text." }
    }
}

function Assert-AsciiReleaseTree {
    param([Parameter(Mandatory)][string]$Root, [Parameter(Mandatory)][string]$Description)
    foreach ($relative in Get-RelativeFiles $Root) {
        foreach ($character in $relative.ToCharArray()) {
            if ([int]$character -gt 0x7F) { throw "$Description contains a non-ASCII path: $relative" }
        }
        if ((Test-IsReleaseTextPath $relative) -and -not (Test-IsPinnedUpstreamTextPath $relative)) {
            Assert-AsciiBytes ([IO.File]::ReadAllBytes((Join-Path $Root $relative.Replace('/', '\')))) "$Description file $relative"
        }
    }
}

function Assert-NoForbiddenPayload {
    param([Parameter(Mandatory)][string]$Root, [Parameter(Mandatory)][string]$Description)
    $files = @(Get-RelativeFiles $Root)
    foreach ($relative in $files) {
        if ($relative -match '(?i)(^|/)(ZeroKarya_PartySwitch|PartySwitch)(/|$)' -or
            $relative -match '(?i)(^|/)enabled\.txt$' -or
            $relative -match '(?i)(^|/)StableRoot(/|$)' -or
            $relative -match '(?i)(^|/)main_stable\.(dll|pdb)$' -or
            $relative -match '(?i)(^|/)(debug|logs?)(/|$)' -or
            $relative -match '(?i)\.(pdb|log|bak)$') {
            throw "$Description contains a forbidden payload path: $relative"
        }
        if (Test-IsReleaseTextPath $relative) {
            $text = [IO.File]::ReadAllText((Join-Path $Root $relative.Replace('/', '\')), [Text.Encoding]::ASCII)
            if ($text -match '(?i)ZeroKarya_PartySwitch|PartySwitch') {
                throw "$Description contains unrelated third-party Mod content: $relative"
            }
        }
    }

    if ($files -contains 'UE4SS.dll' -or
        @($files | Where-Object { $_ -match '^(?i:Mods)/' }).Count -gt 0) {
        throw "$Description contains a StableRoot UE4SS payload."
    }
}

function Write-Sha256Sums {
    param([Parameter(Mandatory)][string]$Root)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $destination = Join-Path $rootFull 'SHA256SUMS.txt'
    $rows = foreach ($file in Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File | Sort-Object FullName) {
        if ([string]::Equals($file.FullName, $destination, [StringComparison]::OrdinalIgnoreCase)) { continue }
        $relative = $file.FullName.Substring($rootFull.Length).TrimStart('\').Replace('\', '/')
        "$(Get-Sha256 $file.FullName)  $relative"
    }
    [IO.File]::WriteAllLines($destination, [string[]]$rows, [Text.UTF8Encoding]::new($false))
}

function New-DeterministicZip {
    param([Parameter(Mandatory)][string]$Source, [Parameter(Mandatory)][string]$Destination)
    $root = [IO.Path]::GetFullPath($Source).TrimEnd('\')
    if (Test-Path -LiteralPath $Destination) { Remove-Item -LiteralPath $Destination -Force }
    $stream = [IO.File]::Open($Destination, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create, $false)
        try {
            foreach ($file in Get-ChildItem -LiteralPath $root -Recurse -Force -File | Sort-Object FullName) {
                $name = $file.FullName.Substring($root.Length).TrimStart('\').Replace('\', '/')
                $entry = $archive.CreateEntry($name, [IO.Compression.CompressionLevel]::Optimal)
                $entry.LastWriteTime = [DateTimeOffset]::new(1980, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
                $input = [IO.File]::OpenRead($file.FullName)
                try {
                    $output = $entry.Open()
                    try { $input.CopyTo($output) }
                    finally { $output.Dispose() }
                }
                finally { $input.Dispose() }
            }
        }
        finally { $archive.Dispose() }
    }
    finally { $stream.Dispose() }
}

function Get-ZipFileEntries {
    param([Parameter(Mandatory)][string]$Path)
    $archive = [IO.Compression.ZipFile]::OpenRead($Path)
    try {
        @(
            $archive.Entries |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_.Name) } |
                ForEach-Object { $_.FullName.Replace('\', '/') } |
                Sort-Object
        )
    }
    finally { $archive.Dispose() }
}

function Get-ZipEntrySha256 {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$EntryName)
    $expected = $EntryName.Replace('\', '/')
    $archive = [IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $matches = @($archive.Entries | Where-Object { $_.FullName.Replace('\', '/') -eq $expected })
        if ($matches.Count -ne 1) { throw "Expected one archive entry named $expected; found $($matches.Count)." }
        $stream = $matches[0].Open()
        try { Get-StreamSha256 $stream }
        finally { $stream.Dispose() }
    }
    finally { $archive.Dispose() }
}

function Assert-ZipPathsAndText {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Description)
    $archive = [IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $normalized = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in $archive.Entries) {
            if ([string]::IsNullOrWhiteSpace($entry.Name)) { continue }
            $relative = $entry.FullName.Replace('\', '/')
            if ($relative -match '(^/|^[A-Za-z]:|(^|/)\.\.(/|$))') {
                throw "$Description contains an unsafe path: $relative"
            }
            if (-not $normalized.Add($relative)) { throw "$Description contains a duplicate path: $relative" }
            foreach ($character in $relative.ToCharArray()) {
                if ([int]$character -gt 0x7F) { throw "$Description contains a non-ASCII path: $relative" }
            }
            if ($relative -match '(?i)(^|/)(ZeroKarya_PartySwitch|PartySwitch)(/|$)' -or
                $relative -match '(?i)(^|/)enabled\.txt$' -or
                $relative -match '(?i)(^|/)StableRoot(/|$)' -or
                $relative -match '(?i)(^|/)main_stable\.(dll|pdb)$') {
                throw "$Description contains a forbidden payload path: $relative"
            }
            if (Test-IsReleaseTextPath $relative) {
                $stream = $entry.Open()
                try {
                    $memory = [IO.MemoryStream]::new()
                    try { $stream.CopyTo($memory); $bytes = $memory.ToArray() }
                    finally { $memory.Dispose() }
                }
                finally { $stream.Dispose() }
                if (-not (Test-IsPinnedUpstreamTextPath $relative)) {
                    Assert-AsciiBytes $bytes "$Description entry $relative"
                }
                $text = [Text.Encoding]::ASCII.GetString($bytes)
                if ($text -match '(?i)ZeroKarya_PartySwitch|PartySwitch') {
                    throw "$Description contains unrelated third-party Mod content: $relative"
                }
            }
        }
    }
    finally { $archive.Dispose() }
}

function Assert-PinnedUpstreamTextPreserved {
    param(
        [Parameter(Mandatory)][string]$RuntimeZip,
        [Parameter(Mandatory)][string]$ExpandedRoot
    )
    $archive = [IO.Compression.ZipFile]::OpenRead($RuntimeZip)
    try {
        $checked = 0
        foreach ($entry in $archive.Entries) {
            if ([string]::IsNullOrWhiteSpace($entry.Name)) { continue }
            $relative = $entry.FullName.Replace('\', '/')
            if (-not (Test-IsPinnedUpstreamTextPath $relative)) { continue }
            $expanded = Join-Path $ExpandedRoot $relative.Replace('/', '\')
            if (-not (Test-Path -LiteralPath $expanded -PathType Leaf)) {
                throw "Pinned upstream text is missing from the complete manual package: $relative"
            }
            $stream = $entry.Open()
            try { $expected = Get-StreamSha256 $stream }
            finally { $stream.Dispose() }
            if ((Get-Sha256 $expanded) -ne $expected) {
                throw "Pinned upstream text was not preserved byte-for-byte: $relative"
            }
            $checked++
        }
        if ($checked -eq 0) { throw 'The pinned runtime did not expose any upstream text exemption paths.' }
    }
    finally { $archive.Dispose() }
}

function New-VerifiedDeterministicZip {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string[]]$ExpectedEntries,
        [Parameter(Mandatory)][string]$ReproductionPath,
        [Parameter(Mandatory)][string]$Description
    )
    New-DeterministicZip $Source $Destination
    New-DeterministicZip $Source $ReproductionPath
    $first = Get-Sha256 $Destination
    $second = Get-Sha256 $ReproductionPath
    if ($first -ne $second) { throw "$Description is not byte-for-byte reproducible: $first vs $second" }
    Assert-ExactSet $ExpectedEntries (Get-ZipFileEntries $Destination) "$Description archive"
    Assert-ExactSet $ExpectedEntries (Get-ZipFileEntries $ReproductionPath) "$Description reproduction"
    Assert-ZipPathsAndText $Destination $Description
    Remove-Item -LiteralPath $ReproductionPath -Force
}

function Assert-Sha256Sums {
    param([Parameter(Mandatory)][string]$ZipPath, [Parameter(Mandatory)][string]$Description)
    $archive = [IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $entries = @{}
        foreach ($entry in $archive.Entries) {
            if ([string]::IsNullOrWhiteSpace($entry.Name)) { continue }
            $entries[$entry.FullName.Replace('\', '/')] = $entry
        }
        if (-not $entries.ContainsKey('SHA256SUMS.txt')) { throw "$Description is missing SHA256SUMS.txt." }
        $reader = [IO.StreamReader]::new($entries['SHA256SUMS.txt'].Open(), [Text.Encoding]::ASCII)
        try { $lines = @($reader.ReadToEnd() -split '\r?\n' | Where-Object { $_ }) }
        finally { $reader.Dispose() }
        $summed = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        foreach ($line in $lines) {
            if ($line -notmatch '^([0-9A-F]{64})  (.+)$') { throw "$Description has a malformed checksum row: $line" }
            $expectedHash = $matches[1]
            $relative = $matches[2]
            if (-not $entries.ContainsKey($relative)) { throw "$Description checksum target is missing: $relative" }
            if (-not $summed.Add($relative)) { throw "$Description has a duplicate checksum row: $relative" }
            $stream = $entries[$relative].Open()
            try { $actualHash = Get-StreamSha256 $stream }
            finally { $stream.Dispose() }
            if ($actualHash -ne $expectedHash) { throw "$Description checksum mismatch: $relative" }
        }
        $expectedSummed = @($entries.Keys | Where-Object { $_ -ne 'SHA256SUMS.txt' } | Sort-Object)
        Assert-ExactSet $expectedSummed @($summed | Sort-Object) "$Description checksum coverage"
    }
    finally { $archive.Dispose() }
}

function Assert-InstallerResources {
    param(
        [Parameter(Mandatory)][string]$InstallerPath,
        [Parameter(Mandatory)][hashtable]$ExpectedHashes
    )
    $assembly = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes($InstallerPath))
    $expectedNames = @(
        'Payload.DefaultConfig.ini',
        'Payload.ExperimentalPlugin.dll',
        'Payload.ExperimentalUE4SSRuntime.zip',
        'Payload.Main.lua',
        'Payload.Manifest.ini',
        'Payload.PickupRangeX10.pak',
        'Payload.PickupRangeX15.pak',
        'Payload.PickupRangeX20.pak',
        'Payload.PickupRangeX3.pak',
        'Payload.PickupRangeX5.pak',
        'Payload.ThirdPartyNotices.txt'
    )
    Assert-ExactSet $expectedNames @($assembly.GetManifestResourceNames()) 'Installer embedded resources'
    foreach ($name in $ExpectedHashes.Keys) {
        $stream = $assembly.GetManifestResourceStream($name)
        if ($null -eq $stream) { throw "Installer embedded resource is missing: $name" }
        try { $actual = Get-StreamSha256 $stream }
        finally { $stream.Dispose() }
        if ($actual -ne $ExpectedHashes[$name]) { throw "Installer embedded resource hash mismatch: $name" }
    }
    $manifestStream = $assembly.GetManifestResourceStream('Payload.Manifest.ini')
    try {
        $memory = [IO.MemoryStream]::new()
        try { $manifestStream.CopyTo($memory); $manifestBytes = $memory.ToArray() }
        finally { $memory.Dispose() }
    }
    finally { $manifestStream.Dispose() }
    Assert-AsciiBytes $manifestBytes 'Installer embedded manifest'
    $manifestText = [Text.Encoding]::ASCII.GetString($manifestBytes)
    if ($manifestText -notmatch '(?m)^version=1\.3\.0\r?$') {
        throw 'Installer embedded manifest does not identify version 1.3.0.'
    }
    if ($manifestText -notmatch '(?m)^game_hash_policy=diagnostic_only\r?$') {
        throw 'Installer embedded manifest does not preserve diagnostic-only game hashes.'
    }
    if ($manifestText -notmatch
        '(?m)^selector_resolution_policy=runtime_reflection_dual_caller_rel32_consensus_fail_closed\r?$') {
        throw 'Installer embedded manifest does not identify the fail-closed dynamic selector policy.'
    }
    if ($manifestText -notmatch
            '(?m)^selector_server_anchor=reflected_virtual_slot_runtime_bounded_implementation\r?$' -or
        $manifestText -notmatch
            '(?m)^selector_ui_anchor=reflected_pdata_terminal_rel32_runtime_bounded_implementation\r?$' -or
        $manifestText -notmatch
            '(?m)^selector_consensus=exact_server_ui_target_match\r?$') {
        throw 'Installer embedded manifest does not identify both structural selector anchors.'
    }
}

function Add-ModFiles {
    param([Parameter(Mandatory)][string]$Root, [Parameter(Mandatory)][string]$Dll)
    $mod = Join-Path $Root 'ue4ss\Mods\DragonSwordNativeAutoPickup'
    New-Item -ItemType Directory -Path (Join-Path $mod 'dlls') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $mod 'Scripts') -Force | Out-Null
    Copy-Item -LiteralPath $Dll -Destination (Join-Path $mod 'dlls\main.dll') -Force
    Copy-Item -LiteralPath (Join-Path $projectRoot 'config\default.ini') -Destination (Join-Path $mod 'config.ini') -Force
    Copy-Item -LiteralPath (Join-Path $projectRoot 'Scripts\main.lua') -Destination (Join-Path $mod 'Scripts\main.lua') -Force
    Copy-Item -LiteralPath (Join-Path $projectRoot 'installer\THIRD_PARTY_NOTICES.txt') -Destination (Join-Path $mod 'THIRD_PARTY_NOTICES.txt') -Force
}

function New-ArtifactRecord {
    param([Parameter(Mandatory)][string]$Type, [Parameter(Mandatory)][string]$Path)
    [ordered]@{
        type = $Type
        name = [IO.Path]::GetFileName($Path)
        size_bytes = [int64](Get-Item -LiteralPath $Path).Length
        sha256 = Get-Sha256 $Path
        entries = @(Get-ZipFileEntries $Path)
    }
}

if (Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue) {
    throw 'Close DragonSword before building release artifacts.'
}

# The source gate is intentionally full: manifest, package rules, and core tests.
& (Join-Path $PSScriptRoot 'Verify-Source.ps1')

$runtime = Resolve-RequiredFile $RuntimeZip 'Pinned ExperimentalNested UE4SS compatibility runtime'
if ((Get-Sha256 $runtime) -ne $runtimeHash) {
    throw 'The supplied UE4SS runtime archive is not the approved g1c1a1497 artifact.'
}
if ((Get-ZipEntrySha256 $runtime 'ue4ss/UE4SS.dll') -ne $ue4ssDllHash -or
    (Get-ZipEntrySha256 $runtime 'dwmapi.dll') -ne $dwmapiHash) {
    throw 'The supplied UE4SS runtime contains an unexpected loader or ExperimentalNested DLL.'
}
Assert-ZipPathsAndText $runtime 'Pinned UE4SS compatibility runtime'
$runtimeEntries = @(Get-ZipFileEntries $runtime)
if ($runtimeEntries -contains 'UE4SS.dll' -or
    $runtimeEntries -notcontains 'ue4ss/UE4SS.dll' -or
    $runtimeEntries -notcontains 'dwmapi.dll' -or
    @($runtimeEntries | Where-Object { $_ -match '^(?i:Mods)/' }).Count -gt 0) {
    throw 'The supplied UE4SS runtime is not ExperimentalNested-only.'
}

$rangePaks = [ordered]@{}
foreach ($multiplier in @(3, 5, 10, 15, 20)) {
    $pak = Resolve-RequiredFile (Join-Path $rangeRoot "dist\DS_PickupRangeX${multiplier}_P.pak") "${multiplier}x range PAK"
    if ((Get-Sha256 $pak) -ne $rangeHashes[[string]$multiplier]) {
        throw "The ${multiplier}x range PAK is not the approved release input."
    }
    $rangePaks[[string]$multiplier] = $pak
}
$rangeBundle = Resolve-RequiredFile `
    (Join-Path $rangeRoot "dist\DragonSwordPickupRangeExpansion-v$rangeBundleSourceVersion.zip") `
    'Standalone range PAK bundle'
if ((Get-Sha256 $rangeBundle) -ne $rangeBundleHash) {
    throw 'The standalone range PAK bundle is not the exact approved immutable input.'
}
$rangeBundleEntries = @(
    'DS_PickupRangeX10_P.pak',
    'DS_PickupRangeX15_P.pak',
    'DS_PickupRangeX20_P.pak',
    'DS_PickupRangeX3_P.pak',
    'DS_PickupRangeX5_P.pak',
    'README.md',
    'SHA256SUMS.txt'
)
Assert-ExactSet $rangeBundleEntries (Get-ZipFileEntries $rangeBundle) 'Standalone range PAK bundle'
Assert-Sha256Sums $rangeBundle 'Standalone range PAK bundle'
foreach ($multiplier in @(3, 5, 10, 15, 20)) {
    $entryName = "DS_PickupRangeX${multiplier}_P.pak"
    if ((Get-ZipEntrySha256 $rangeBundle $entryName) -ne $rangeHashes[[string]$multiplier]) {
        throw "The standalone range PAK bundle contains an unexpected ${multiplier}x payload."
    }
}

$nativeBuild = Join-Path $projectRoot 'out\native\ExperimentalNested'
if (-not $SkipNativeBuild) {
    & (Join-Path $PSScriptRoot 'Build-Native.ps1') `
        -UE4SSRoot $UE4SSRoot `
        -UE4SSVariant ExperimentalNested `
        -BuildDirectory $nativeBuild
}
$dll = Resolve-RequiredFile (Join-Path $nativeBuild 'main.dll') 'ExperimentalNested Auto Pickup DLL'
$artifact = & (Join-Path $PSScriptRoot 'Verify-BuiltArtifact.ps1') `
    -DllPath $dll `
    -UE4SSVariant ExperimentalNested
$dllBytes = [IO.File]::ReadAllBytes($dll)
$dllAscii = [Text.Encoding]::ASCII.GetString($dllBytes)
if ($dllAscii.Contains('stable_ProcessEvent') -or $dllAscii.Contains('DRAGONSWORD_NATIVE_AUTO_PICKUP_1_1_1')) {
    throw 'The native candidate contains a StableRoot or stale release identity.'
}

$installerBuild = [IO.Path]::GetFullPath($InstallerBuildDirectory)
& (Join-Path $PSScriptRoot 'Build-Installer.ps1') `
    -ExperimentalPluginDll $dll `
    -ExperimentalUE4SSRuntimeZip $runtime `
    -PickupRangeX3Pak $rangePaks['3'] `
    -PickupRangeX5Pak $rangePaks['5'] `
    -PickupRangeX10Pak $rangePaks['10'] `
    -PickupRangeX15Pak $rangePaks['15'] `
    -PickupRangeX20Pak $rangePaks['20'] `
    -OutputDirectory $installerBuild
$installer = Resolve-RequiredFile `
    (Join-Path $installerBuild "DragonSwordNativeAutoPickup-Setup-$version.exe") `
    'Auto Pickup installer'
$installerSidecar = Resolve-RequiredFile "$installer.sha256" 'Installer SHA-256 sidecar'
$installerHash = Get-Sha256 $installer
$sidecarText = [IO.File]::ReadAllText($installerSidecar, [Text.Encoding]::ASCII).Trim()
if ($sidecarText -ne "$installerHash  $([IO.Path]::GetFileName($installer))") {
    throw 'Installer SHA-256 sidecar does not match the exact Setup executable.'
}

$resourceHashes = @{
    'Payload.ExperimentalPlugin.dll' = [string]$artifact.sha256
    'Payload.ExperimentalUE4SSRuntime.zip' = $runtimeHash
    'Payload.DefaultConfig.ini' = Get-Sha256 (Join-Path $projectRoot 'config\default.ini')
    'Payload.Main.lua' = Get-Sha256 (Join-Path $projectRoot 'Scripts\main.lua')
    'Payload.PickupRangeX3.pak' = $rangeHashes['3']
    'Payload.PickupRangeX5.pak' = $rangeHashes['5']
    'Payload.PickupRangeX10.pak' = $rangeHashes['10']
    'Payload.PickupRangeX15.pak' = $rangeHashes['15']
    'Payload.PickupRangeX20.pak' = $rangeHashes['20']
    'Payload.ThirdPartyNotices.txt' = Get-Sha256 (Join-Path $projectRoot 'installer\THIRD_PARTY_NOTICES.txt')
}
Assert-InstallerResources $installer $resourceHashes

$installerTestParameters = @{ InstallerExe = $installer }
if (-not [string]::IsNullOrWhiteSpace($InstallerTestGameExecutable)) {
    $installerTestParameters.GameExecutable = Resolve-RequiredFile $InstallerTestGameExecutable 'Installer structural game executable fixture'
}
$installerTestOutput = @(& (Join-Path $PSScriptRoot 'Test-Installer110.ps1') @installerTestParameters)
$installerTestResults = @($installerTestOutput | Where-Object {
    $_ -is [psobject] -and $_.PSObject.Properties.Name -contains 'release_gate'
})
if ($installerTestResults.Count -ne 1) {
    throw "Installer matrix returned $($installerTestResults.Count) release-gate objects; expected one."
}
$installerTest = $installerTestResults[0]
if ($installerTest.expected -ne 10 -or $installerTest.passed -ne 10 -or
    $installerTest.failed -ne 0 -or $installerTest.skipped -ne 0 -or
    $installerTest.release_gate -ne 'PASSED' -or -not $installerTest.fixtures_cleaned) {
    throw 'Installer release gate requires exactly 10 passed, 0 failed, 0 skipped, and cleaned fixtures.'
}

$stagingParent = Join-Path $projectRoot 'out\staging'
New-Item -ItemType Directory -Path $stagingParent -Force | Out-Null
$releaseStage = Reset-SafeDirectory (Join-Path $stagingParent 'release-1.3.0') $stagingParent
$installerStage = Join-Path $releaseStage 'installer'
$manualStage = Join-Path $releaseStage 'manual-no-ue4ss'
$withStage = Join-Path $releaseStage 'manual-with-ue4ss'
$candidateOutput = Join-Path $releaseStage 'candidate-output'
foreach ($directory in @($installerStage, $manualStage, $withStage, $candidateOutput)) {
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
}

Copy-Item -LiteralPath $installer -Destination $installerStage -Force
[IO.File]::WriteAllText((Join-Path $installerStage 'README.md'), @'
# DragonSword Auto Pickup 1.3.0 - One-Click Installer

Close the game and run the Setup executable. It detects
`DSClient-Win64-Shipping.exe` from Steam when available; otherwise use Browse.

Setup supports the tested ExperimentalNested UE4SS v3.0.1 Beta #0 commit
`1c1a1497`. A different or mixed UE4SS layout requires confirmation and a
verified conversion backup. An exact-runtime Upgrade preserves `config.ini`
without a persistent conversion backup. Uninstall removes only owned Auto
Pickup content and approved owned range PAKs.

Auto Pickup starts disabled. Press F9 after a playable World loads. Returning
to the main menu or loading another save disables it again. Optional range
choices are Original, 3x, 5x, 10x, 15x, and 20x.
'@, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $projectRoot 'installer\THIRD_PARTY_NOTICES.txt') -Destination $installerStage -Force
Write-Sha256Sums $installerStage

Add-ModFiles $manualStage $dll
[IO.File]::WriteAllText(
    (Join-Path $manualStage 'ue4ss\Mods\mods.txt'),
    "DragonSwordNativeAutoPickup : 1`r`n",
    [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $manualStage 'README.md'), @'
# DragonSword Auto Pickup 1.3.0 - Manual Installation (No UE4SS)

This package requires the existing ExperimentalNested UE4SS v3.0.1 Beta #0
commit `1c1a1497`. UE4SS and range PAKs are not included. For a manual range
choice, use the separately published `DragonSwordPickupRangeExpansion-v1.3.0.zip`.

## Install

1. Close DragonSword: Awakening.
2. Open the game folder ending in `DS\Binaries\Win64`. It contains
   `DSClient-Win64-Shipping.exe`.
3. Confirm that `Win64\ue4ss\UE4SS.dll` already exists.
4. From this ZIP, copy only
   `ue4ss\Mods\DragonSwordNativeAutoPickup` into
   `Win64\ue4ss\Mods\`.
5. Enable the Mod in `Win64\ue4ss\Mods\mods.txt`:
   - If that file does not exist, copy the included
     `ue4ss\Mods\mods.txt` to that exact location.
   - If it already exists, especially when other Mods are installed, do not
     overwrite it. Keep every existing line and add or replace exactly one
     Auto Pickup line:

     ```text
     DragonSwordNativeAutoPickup : 1
     ```

6. Launch the game, load a playable World, and press F9 to enable Auto Pickup.

Never replace an existing `mods.txt` with the one-line file from this ZIP. That
would remove the enablement entries for other installed Mods.

## Remove Auto Pickup

1. Close DragonSword: Awakening.
2. Delete only
   `Win64\ue4ss\Mods\DragonSwordNativeAutoPickup`.
3. Open `Win64\ue4ss\Mods\mods.txt`.
4. Remove only the line for `DragonSwordNativeAutoPickup`. Keep every line for
   other Mods. Do not delete or replace the whole `mods.txt`.
5. Keep `Win64\dwmapi.dll`, the `Win64\ue4ss` runtime, and unrelated Mod
   folders. Other installed Mods may use them.

## Remove an optional range PAK

If you separately installed a range PAK, remove it as follows:

1. Close the game.
2. From the game installation folder, open `DS\Content\Paks\~mods`.
3. Delete only the one range file you installed:
   - `DS_PickupRangeX3_P.pak`
   - `DS_PickupRangeX5_P.pak`
   - `DS_PickupRangeX10_P.pak`
   - `DS_PickupRangeX15_P.pak`
   - `DS_PickupRangeX20_P.pak`
4. Keep the `~mods` folder and every other `.pak` file. Removing the selected
   range PAK restores the original interaction range after the next game start.

If you are not certain which range file belongs to Auto Pickup, leave it in
place and use the one-click installer to check ownership.
'@, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $projectRoot 'installer\THIRD_PARTY_NOTICES.txt') -Destination $manualStage -Force
Write-Sha256Sums $manualStage

Expand-Archive -LiteralPath $runtime -DestinationPath $withStage -Force
Add-ModFiles $withStage $dll
$modsTxt = Join-Path $withStage 'ue4ss\Mods\mods.txt'
$modsLines = if (Test-Path -LiteralPath $modsTxt) { @(Get-Content -LiteralPath $modsTxt) } else { @() }
$modsLines = @($modsLines | Where-Object { $_ -notmatch '^\s*DragonSwordNativeAutoPickup\s*:' }) +
    'DragonSwordNativeAutoPickup : 1'
[IO.File]::WriteAllLines($modsTxt, [string[]]$modsLines, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $withStage 'README.md'), @'
# DragonSword Auto Pickup 1.3.0 - Manual Installation (With UE4SS)

This package contains Auto Pickup and the tested ExperimentalNested UE4SS
v3.0.1 Beta #0 commit `1c1a1497`. Range PAKs are not included. For a manual
range choice, use the separately published
`DragonSwordPickupRangeExpansion-v1.3.0.zip`.

## Clean installation with no existing UE4SS

1. Close DragonSword: Awakening.
2. Open the game folder ending in `DS\Binaries\Win64`. It contains
   `DSClient-Win64-Shipping.exe`.
3. Extract every file and folder from this ZIP directly into that `Win64`
   folder.
4. Launch the game, load a playable World, and press F9 to enable Auto Pickup.

## Existing UE4SS or other installed Mods

1. Close the game and back up the existing `Win64\ue4ss` folder and
   `Win64\dwmapi.dll`.
2. Do not extract the whole ZIP over the existing installation. If the current
   runtime is already the exact supported ExperimentalNested version, copy only
   `ue4ss\Mods\DragonSwordNativeAutoPickup` into
   `Win64\ue4ss\Mods\`.
3. Open the existing `Win64\ue4ss\Mods\mods.txt`. Do not overwrite this file.
   Keep every line for other Mods and add or replace exactly one line:

   ```text
   DragonSwordNativeAutoPickup : 1
   ```

4. Launch the game, load a playable World, and press F9 to enable Auto Pickup.

If the installed UE4SS version or layout is different or uncertain, use the
one-click installer instead. It performs the compatibility check, backup, and
Mod migration.

## Remove Auto Pickup

1. Close DragonSword: Awakening.
2. Delete only
   `Win64\ue4ss\Mods\DragonSwordNativeAutoPickup`.
3. Open `Win64\ue4ss\Mods\mods.txt`.
4. Remove only the line for `DragonSwordNativeAutoPickup`. Keep every line for
   other Mods. Do not delete or replace the whole `mods.txt`.
5. Keep `Win64\dwmapi.dll`, the `Win64\ue4ss` runtime, and unrelated Mod
   folders. Other installed Mods may use them.

## Remove an optional range PAK

If you separately installed a range PAK, remove it as follows:

1. Close the game.
2. From the game installation folder, open `DS\Content\Paks\~mods`.
3. Delete only the one range file you installed:
   - `DS_PickupRangeX3_P.pak`
   - `DS_PickupRangeX5_P.pak`
   - `DS_PickupRangeX10_P.pak`
   - `DS_PickupRangeX15_P.pak`
   - `DS_PickupRangeX20_P.pak`
4. Keep the `~mods` folder and every other `.pak` file. Removing the selected
   range PAK restores the original interaction range after the next game start.

If you are not certain which range file belongs to Auto Pickup, leave it in
place and use the one-click installer to check ownership.
'@, [Text.UTF8Encoding]::new($false))
Write-Sha256Sums $withStage
Assert-PinnedUpstreamTextPreserved $runtime $withStage

$installerEntries = @(
    "DragonSwordNativeAutoPickup-Setup-$version.exe",
    'README.md',
    'SHA256SUMS.txt',
    'THIRD_PARTY_NOTICES.txt'
)
$manualEntries = @(
    'README.md',
    'SHA256SUMS.txt',
    'THIRD_PARTY_NOTICES.txt',
    'ue4ss/Mods/mods.txt',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/config.ini',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/dlls/main.dll',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/Scripts/main.lua',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/THIRD_PARTY_NOTICES.txt'
)
$withEntries = @($runtimeEntries + @(
    'ue4ss/Mods/DragonSwordNativeAutoPickup/config.ini',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/dlls/main.dll',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/Scripts/main.lua',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/THIRD_PARTY_NOTICES.txt'
) | Sort-Object -Unique)

Assert-ExactSet $installerEntries (Get-RelativeFiles $installerStage) 'Installer stage'
Assert-ExactSet $manualEntries (Get-RelativeFiles $manualStage) 'Manual-no-UE4SS stage'
Assert-ExactSet $withEntries (Get-RelativeFiles $withStage) 'Manual-with-UE4SS stage'
foreach ($stage in @(
    @($installerStage, 'Installer stage'),
    @($manualStage, 'Manual-no-UE4SS stage'),
    @($withStage, 'Manual-with-UE4SS stage'))) {
    Assert-AsciiReleaseTree $stage[0] $stage[1]
    Assert-NoForbiddenPayload $stage[0] $stage[1]
}

$installerZip = Join-Path $candidateOutput "DragonSwordAutoPickup-v$version-Installer.zip"
$manualZip = Join-Path $candidateOutput "DragonSwordAutoPickup-v$version-Manual-No-UE4SS.zip"
$withZip = Join-Path $candidateOutput "DragonSwordAutoPickup-v$version-Manual-With-UE4SS.zip"
$rangeZip = Join-Path $candidateOutput "DragonSwordPickupRangeExpansion-v$version.zip"
New-VerifiedDeterministicZip $installerStage $installerZip $installerEntries (Join-Path $releaseStage 'installer.repro.zip') 'Installer ZIP'
New-VerifiedDeterministicZip $manualStage $manualZip $manualEntries (Join-Path $releaseStage 'manual-no-ue4ss.repro.zip') 'Manual-no-UE4SS ZIP'
New-VerifiedDeterministicZip $withStage $withZip $withEntries (Join-Path $releaseStage 'manual-with-ue4ss.repro.zip') 'Manual-with-UE4SS ZIP'
Assert-Sha256Sums $installerZip 'Installer ZIP'
Assert-Sha256Sums $manualZip 'Manual-no-UE4SS ZIP'
Assert-Sha256Sums $withZip 'Manual-with-UE4SS ZIP'
Copy-Item -LiteralPath $rangeBundle -Destination $rangeZip -Force
if ((Get-Sha256 $rangeZip) -ne $rangeBundleHash) {
    throw 'The copied standalone range PAK bundle changed during staging.'
}
Assert-ExactSet $rangeBundleEntries (Get-ZipFileEntries $rangeZip) 'Published standalone range PAK bundle'
Assert-Sha256Sums $rangeZip 'Published standalone range PAK bundle'

$artifacts = @(
    New-ArtifactRecord 'installer' $installerZip
    New-ArtifactRecord 'manual_without_ue4ss' $manualZip
    New-ArtifactRecord 'manual_with_ue4ss' $withZip
    New-ArtifactRecord 'standalone_range_paks' $rangeZip
)
$rangeInputs = @(
    foreach ($multiplier in @(3, 5, 10, 15, 20)) {
        [ordered]@{
            multiplier = $multiplier
            name = [IO.Path]::GetFileName($rangePaks[[string]$multiplier])
            sha256 = $rangeHashes[[string]$multiplier]
        }
    }
)
$manifest = [ordered]@{
    schema_version = 5
    version = $version
    generated_at_utc = [DateTime]::UtcNow.ToString('O')
    runtime_label = $runtimeLabel
    supported_ue4ss = [ordered]@{
        version = 'v3.0.1 Beta #0'
        git_sha = '1c1a1497f942c707f47ba668db75b25e86f6c08a'
        layout = 'experimental_nested'
        ue4ss_dll_sha256 = $ue4ssDllHash
        dwmapi_sha256 = $dwmapiHash
        stable_root_payload_included = $false
    }
    native_artifact = [ordered]@{
        size_bytes = [int64]$artifact.size_bytes
        sha256 = [string]$artifact.sha256
        selector_resolution_policy = [string]$artifact.selector_resolution_policy
    }
    game_hash_policy = 'diagnostic_only'
    fixed_rva_fallback = $false
    game_hash_used_for_selector_resolution = $false
    compatibility_scope = 'compatible_reflection_and_machine_code_contracts_not_arbitrary_recompile'
    selector_anchor_resolution = [ordered]@{
        server = 'reflected_exec_leaf_to_unique_virtual_slot_to_cdo_vtable_to_runtime_bounded_native_implementation'
        ui = 'reflected_exec_pdata_chaininfo_to_unique_terminal_rel32_call_to_runtime_bounded_native_implementation'
        consensus = 'server_and_ui_selector_addresses_must_match_exactly'
        failure = 'automation_off_for_process_no_historical_address_fallback'
    }
    action_lifecycle = [ordered]@{
        pending_identity = 'exact_returned_interaction_component_weak_identity'
        confirmation = 'exact_actor_or_component_invalidation_or_exact_component_inactive'
        confirmation_window_ms = 650
        retry_delay_ms = 100
        maximum_attempts = 2
        terminal_timeout = 'quarantine_exact_component_for_current_activation'
        retry_preflight = 'before_live_action_mapping_and_subsystem_resolution'
        interaction_owner_change = 'clear_pending_and_attempt_records_then_enforce_1500ms_settle'
    }
    drop_item_range = [ordered]@{
        activation = 'one_recognized_owned_range_pak_only'
        hook = 'DropItemActor_BeginPlay_exact_reflection'
        target = 'DropItemActor.SphereOverlapComp'
        coverage = 'ordinary_meat_aged_meat_and_other_class_proven_monster_drops'
        general_uobject_scan = $false
        root_physics_or_hit_collision_mutation = $false
    }
    installer_executable = [ordered]@{
        name = [IO.Path]::GetFileName($installer)
        size_bytes = [int64](Get-Item -LiteralPath $installer).Length
        sha256 = $installerHash
        signed = $false
    }
    installer_ownership = [ordered]@{
        recognized_owned_installation = 'in_place_upgrade_or_repair_preserving_config'
        current_embedded_payload = 'owned_by_exact_embedded_resource_hashes'
        historical_payloads = 'owned_only_by_exact_evidenced_version_plugin_and_lua_hash_contracts'
        unknown_same_name_payload = 'fail_closed_zero_mutation'
    }
    public_defaults = [ordered]@{
        enabled_on_launch = $false
        automatic_pickup = $true
        toggle_hotkey = 'F9'
        interaction_key = 'AUTO'
        interaction_key_fallback = 'F'
        debug_logging = $false
        range_selection = 'original'
    }
    supported_range_choices = @('original', '3x', '5x', '10x', '15x', '20x')
    release_package_classes = @('installer', 'manual_without_ue4ss', 'manual_with_ue4ss', 'standalone_range_paks')
    artifacts = $artifacts
    supporting_inputs = [ordered]@{
        standalone_ue4ss_runtime_published = $false
        individual_range_paks_published = $false
        range_bundle_published = $true
        ue4ss_runtime = [ordered]@{
            name = [IO.Path]::GetFileName($runtime)
            sha256 = $runtimeHash
        }
        range_paks = $rangeInputs
        range_bundle = [ordered]@{
            name = [IO.Path]::GetFileName($rangeZip)
            sha256 = $rangeBundleHash
        }
    }
    validation = [ordered]@{
        source_and_core = 'PASSED'
        selector_resolver_fixtures = 'PASSED'
        pe_runtime_fixtures = 'PASSED'
        native_artifact = 'PASSED'
        installer_matrix = [ordered]@{
            expected = 10
            passed = 10
            failed = 0
            skipped = 0
        }
        auto_pickup_archives_deterministic_zip_rebuild = 'PASSED'
        standalone_range_bundle_pinned_owner_artifact = 'PASSED'
        exact_entry_sets = 'PASSED'
        checksum_coverage = 'PASSED'
        auto_pickup_first_party_ascii_english_text = 'PASSED'
        pinned_upstream_text_preserved_byte_for_byte = 'PASSED'
        stable_root_and_unrelated_third_party_exclusion = 'PASSED'
    }
    static_validation = 'PASSED'
    deployed = $false
    runtime_acceptance = 'NOT_VALIDATED_FOR_EXACT_ARTIFACT'
    gameplay_acceptance = 'NOT_VALIDATED_FOR_EXACT_1.3.0_PACKAGES; OWNER_SMOKE_TEST_REQUIRED'
}
$manifestPath = Join-Path $candidateOutput "DragonSwordAutoPickup-v$version.release.json"
[IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
Assert-AsciiBytes ([IO.File]::ReadAllBytes($manifestPath)) 'Release JSON'

$expectedOutput = @(
    [IO.Path]::GetFileName($installerZip),
    [IO.Path]::GetFileName($manualZip),
    [IO.Path]::GetFileName($withZip),
    [IO.Path]::GetFileName($rangeZip),
    [IO.Path]::GetFileName($manifestPath)
)
Assert-ExactSet $expectedOutput (Get-RelativeFiles $candidateOutput) 'Candidate release output'

$distRoot = Join-Path $projectRoot 'dist'
New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
$output = [IO.Path]::GetFullPath($OutputDirectory)
$previousOutput = Join-Path $releaseStage 'previous-output'
$hadPrevious = Test-Path -LiteralPath $output -PathType Container
if ($hadPrevious) {
    New-Item -ItemType Directory -Path $previousOutput -Force | Out-Null
    Copy-DirectoryContents $output $previousOutput
}
try {
    [void](Reset-SafeDirectory $output $distRoot)
    Copy-DirectoryContents $candidateOutput $output
    Assert-ExactSet $expectedOutput (Get-RelativeFiles $output) 'Published release output'
}
catch {
    [void](Reset-SafeDirectory $output $distRoot)
    if ($hadPrevious) { Copy-DirectoryContents $previousOutput $output }
    throw
}

foreach ($record in $artifacts) {
    $published = Join-Path $output $record.name
    if ((Get-Item -LiteralPath $published).Length -ne $record.size_bytes -or
        (Get-Sha256 $published) -ne $record.sha256) {
        throw "Published release artifact changed during finalization: $($record.name)"
    }
}

[pscustomobject]@{
    version = $version
    output = $output
    archives = @($artifacts | ForEach-Object { $_.name })
    installer_tests = '10/10'
    static_validation = 'PASSED'
    deployed = $false
    gameplay_acceptance = 'NOT_VALIDATED_FOR_EXACT_1.3.0_PACKAGES'
}
