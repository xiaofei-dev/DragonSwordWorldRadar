[CmdletBinding()]
param(
    [string]$GameWin64 = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Binaries\Win64',
    [string]$UE4SSSource = 'G:\my_projects\game_mods\DragonSword\DragonSwordNativeWorldRadarPostRender\.sdk\RE-UE4SS',
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$packageName = 'UE4SS-v3.0.1-Beta0-g1c1a1497-DragonSword-Compatibility-Runtime'
$commit = '1c1a1497f942c707f47ba668db75b25e86f6c08a'
$expected = @{
    loader = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
    proxy = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
    usmap = '0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47'
    settings = '4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74'
}

if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot 'dist'
}

function Resolve-RequiredFile([string]$Path, [string]$Description) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description is missing: $Path"
    }
    (Resolve-Path -LiteralPath $Path).Path
}

function Get-Hash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Assert-Hash([string]$Path, [string]$Expected, [string]$Description) {
    $actual = Get-Hash $Path
    if ($actual -ne $Expected) {
        throw "$Description hash mismatch. Expected $Expected, got $actual."
    }
}

function Reset-GeneratedDirectory([string]$Path, [string]$RequiredParent) {
    $full = [IO.Path]::GetFullPath($Path)
    $parent = [IO.Path]::GetFullPath($RequiredParent).TrimEnd('\')
    if (-not [string]::Equals([IO.Path]::GetDirectoryName($full), $parent,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Generated directory escaped its exact parent: $full"
    }
    if (Test-Path -LiteralPath $full) {
        $item = Get-Item -LiteralPath $full -Force
        if (-not $item.PSIsContainer -or
            ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
            @(Get-ChildItem -LiteralPath $full -Recurse -Force |
                Where-Object { ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 }).Count -ne 0) {
            throw "Generated directory is not a plain tree: $full"
        }
        Remove-Item -LiteralPath $full -Recurse -Force
    }
    [void](New-Item -ItemType Directory -Path $full)
    $full
}

function Assert-EnglishReleaseText([string]$Root) {
    $extensions = @('.md', '.txt', '.ini', '.json', '.lua')
    foreach ($file in @(Get-ChildItem -LiteralPath $Root -Recurse -Force -File |
            Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() })) {
        $relative = $file.FullName.Substring($Root.TrimEnd('\').Length + 1)
        if ($relative.StartsWith('ue4ss\Mods\', [StringComparison]::OrdinalIgnoreCase) -or
            $relative.StartsWith('ue4ss\licenses\', [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        $text = [Text.UTF8Encoding]::new($false, $true).GetString(
            [IO.File]::ReadAllBytes($file.FullName))
        if ($text -match '[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]') {
            throw "Release text contains CJK characters: $($file.FullName)"
        }
    }
}

function Write-Checksums([string]$Root) {
    $checksumPath = Join-Path $Root 'SHA256SUMS.txt'
    $builder = [Text.StringBuilder]::new()
    foreach ($file in @(Get-ChildItem -LiteralPath $Root -Recurse -Force -File |
            Where-Object FullName -ne $checksumPath | Sort-Object FullName)) {
        $relative = $file.FullName.Substring($Root.TrimEnd('\').Length + 1).Replace('\', '/')
        [void]$builder.Append((Get-Hash $file.FullName)).Append('  ').Append($relative).Append("`r`n")
    }
    [IO.File]::WriteAllText($checksumPath, $builder.ToString(), [Text.UTF8Encoding]::new($false))
}

function Verify-Checksums([string]$Root) {
    $checksumPath = Join-Path $Root 'SHA256SUMS.txt'
    $lines = [IO.File]::ReadAllLines($checksumPath, [Text.UTF8Encoding]::new($false, $true))
    foreach ($line in $lines) {
        if ($line -notmatch '^([0-9A-F]{64})  (.+)$') {
            throw "Invalid checksum line: $line"
        }
        $path = Join-Path $Root ($Matches[2].Replace('/', '\'))
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-Hash $path) -ne $Matches[1]) {
            throw "Checksum verification failed: $($Matches[2])"
        }
    }
    $covered = @($lines | ForEach-Object { ($_ -split '  ', 2)[1] } | Sort-Object)
    $actual = @(Get-ChildItem -LiteralPath $Root -Recurse -Force -File |
        Where-Object Name -ne 'SHA256SUMS.txt' | ForEach-Object {
            $_.FullName.Substring($Root.TrimEnd('\').Length + 1).Replace('\', '/')
        } | Sort-Object)
    if (Compare-Object $covered $actual) {
        throw 'Checksum list does not exactly cover the package.'
    }
}

function Copy-LicenseFile([string]$SourceRoot, [string]$RelativePath, [string]$LicenseRoot) {
    $source = Resolve-RequiredFile (Join-Path $SourceRoot $RelativePath) "License $RelativePath"
    $destination = Join-Path $LicenseRoot $RelativePath
    $parent = Split-Path -Parent $destination
    if (-not (Test-Path -LiteralPath $parent)) {
        [void](New-Item -ItemType Directory -Path $parent -Force)
    }
    Copy-Item -LiteralPath $source -Destination $destination
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$sourceRoot = (Resolve-Path -LiteralPath $UE4SSSource).Path
$sourceCommit = (& git -C $sourceRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -ne $commit) {
    throw "UE4SS source checkout must be exactly $commit; got $sourceCommit."
}

$loader = Resolve-RequiredFile (Join-Path $GameWin64 'ue4ss\UE4SS.dll') 'Tested UE4SS.dll'
$proxy = Resolve-RequiredFile (Join-Path $GameWin64 'dwmapi.dll') 'Tested dwmapi.dll'
$usmap = Resolve-RequiredFile (Join-Path $GameWin64 'ue4ss\DS-5.3.2-0+UE5-1c1a1497.usmap') 'DragonSword mapping'
$settings = Resolve-RequiredFile (Join-Path $GameWin64 'ue4ss\UE4SS-settings.ini') 'Tested UE4SS settings'
$defaultMods = Join-Path $sourceRoot 'assets\Mods'
$license = Resolve-RequiredFile (Join-Path $sourceRoot 'LICENSE') 'UE4SS MIT license'
$readme = Resolve-RequiredFile (Join-Path $projectRoot 'README.md') 'Package README'

Assert-Hash $loader $expected.loader 'UE4SS.dll'
Assert-Hash $proxy $expected.proxy 'dwmapi.dll'
Assert-Hash $usmap $expected.usmap 'DragonSword mapping'
Assert-Hash $settings $expected.settings 'UE4SS settings'

$outputParent = Split-Path -Parent ([IO.Path]::GetFullPath($OutputDirectory))
if (-not (Test-Path -LiteralPath $outputParent)) {
    [void](New-Item -ItemType Directory -Path $outputParent -Force)
}
$output = Reset-GeneratedDirectory $OutputDirectory $outputParent
$stageParent = Join-Path $projectRoot 'staging'
if (-not (Test-Path -LiteralPath $stageParent)) {
    [void](New-Item -ItemType Directory -Path $stageParent)
}
$stage = Reset-GeneratedDirectory (Join-Path $stageParent $packageName) $stageParent
$runtimeRoot = Join-Path $stage 'ue4ss'
$modsRoot = Join-Path $runtimeRoot 'Mods'
[void](New-Item -ItemType Directory -Path $runtimeRoot)

Copy-Item -LiteralPath $proxy -Destination (Join-Path $stage 'dwmapi.dll')
Copy-Item -LiteralPath $loader -Destination (Join-Path $runtimeRoot 'UE4SS.dll')
Copy-Item -LiteralPath $usmap -Destination (Join-Path $runtimeRoot 'DS-5.3.2-0+UE5-1c1a1497.usmap')
Copy-Item -LiteralPath $settings -Destination (Join-Path $runtimeRoot 'UE4SS-settings.ini')
Copy-Item -LiteralPath $defaultMods -Destination $modsRoot -Recurse
Copy-Item -LiteralPath $license -Destination (Join-Path $runtimeRoot 'LICENSE')
Copy-Item -LiteralPath $readme -Destination (Join-Path $stage 'README.md')

$licenseRoot = Join-Path $runtimeRoot 'licenses'
$licenseFiles = @(
    'LICENSE',
    'UE4SS\src\USMapGenerator\LICENSE',
    'UVTD\LICENSE',
    'deps\fonts\droid\NOTICE.txt',
    'UE4SS\include\fonts\NOTICE.txt',
    'deps\first\ASMHelper\LICENSE',
    'deps\first\Constructs\LICENSE',
    'deps\first\DynamicOutput\LICENSE',
    'deps\first\File\LICENSE',
    'deps\first\Function\LICENSE',
    'deps\first\Helpers\LICENSE',
    'deps\first\IniParser\LICENSE',
    'deps\first\Input\LICENSE',
    'deps\first\JSON\LICENSE',
    'deps\first\LuaMadeSimple\LICENSE',
    'deps\first\LuaRaw\LICENSE',
    'deps\first\MProgram\LICENSE',
    'deps\first\ParserBase\LICENSE',
    'deps\first\ScopedTimer\LICENSE',
    'deps\first\SinglePassSigScanner\LICENSE'
)
foreach ($relative in $licenseFiles) {
    Copy-LicenseFile $sourceRoot $relative $licenseRoot
}

$identity = [ordered]@{
    package = $packageName
    layout = 'ExperimentalNested'
    ue4ss_version = 'v3.0.1 Beta #0'
    ue4ss_source_commit = $commit
    ue4ss_source_url = "https://github.com/UE4SS-RE/RE-UE4SS/tree/$commit"
    supported_mods = @(
        'DragonSword Native Map and Radar Enhancer',
        'DragonSword Auto Pickup'
    )
    included_default_mods = @(Get-ChildItem -LiteralPath $modsRoot -Directory |
        Where-Object Name -ne 'shared' | Sort-Object Name | Select-Object -ExpandProperty Name)
    sdk_backend_included = $false
    sdk_backend_reason = 'Not required at runtime and not present in the tested pinned installation.'
    pinned_files = [ordered]@{
        'dwmapi.dll' = $expected.proxy
        'ue4ss/UE4SS.dll' = $expected.loader
        'ue4ss/DS-5.3.2-0+UE5-1c1a1497.usmap' = $expected.usmap
        'ue4ss/UE4SS-settings.ini' = $expected.settings
    }
}
[IO.File]::WriteAllText((Join-Path $stage 'RUNTIME_IDENTITY.json'),
    ($identity | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))

$sourceText = @"
UE4SS Compatibility Runtime for DragonSword

This is an unofficial compatibility package.

UE4SS version: v3.0.1 Beta #0
Source commit: $commit
Source URL: https://github.com/UE4SS-RE/RE-UE4SS/tree/$commit
Layout: ExperimentalNested

Pinned runtime files:
$($expected.proxy)  dwmapi.dll
$($expected.loader)  ue4ss/UE4SS.dll
$($expected.usmap)  ue4ss/DS-5.3.2-0+UE5-1c1a1497.usmap
$($expected.settings)  ue4ss/UE4SS-settings.ini

UE4SS is Copyright (c) 2022 Narknon and is distributed under the MIT License.
This package is not an official UE4SS release and is not affiliated with or
endorsed by the UE4SS project.
"@
[IO.File]::WriteAllText((Join-Path $stage 'SOURCE_AND_HASHES.txt'),
    $sourceText.Replace("`n", "`r`n"), [Text.UTF8Encoding]::new($false))

if (Get-ChildItem -LiteralPath $stage -Recurse -File |
        Where-Object { $_.Name -match '(?i)\.log$|enabled\.txt$' }) {
    throw 'Release contains a forbidden log or enabled.txt file.'
}
if (Test-Path -LiteralPath (Join-Path $runtimeRoot 'UE4SS_SDK_Backends')) {
    throw 'Release unexpectedly contains an SDK backend.'
}
$unexpectedProductMods = @('DragonSwordNativeWorldRadarPostRender', 'DragonSwordNativeAutoPickup') |
    Where-Object { Test-Path -LiteralPath (Join-Path $modsRoot $_) }
if ($unexpectedProductMods) {
    throw "Release contains a product Mod: $($unexpectedProductMods -join ', ')"
}

Assert-EnglishReleaseText $stage
Write-Checksums $stage
Verify-Checksums $stage

$archive = Join-Path $output "$packageName.zip"
[IO.Compression.ZipFile]::CreateFromDirectory(
    $stage, $archive, [IO.Compression.CompressionLevel]::Optimal, $false)

$verify = Reset-GeneratedDirectory (Join-Path $stageParent ($packageName + '-verify')) $stageParent
try {
    [IO.Compression.ZipFile]::ExtractToDirectory($archive, $verify)
    Assert-EnglishReleaseText $verify
    Verify-Checksums $verify
    Assert-Hash (Join-Path $verify 'dwmapi.dll') $expected.proxy 'Archived dwmapi.dll'
    Assert-Hash (Join-Path $verify 'ue4ss\UE4SS.dll') $expected.loader 'Archived UE4SS.dll'
    Assert-Hash (Join-Path $verify 'ue4ss\DS-5.3.2-0+UE5-1c1a1497.usmap') $expected.usmap 'Archived mapping'
    Assert-Hash (Join-Path $verify 'ue4ss\UE4SS-settings.ini') $expected.settings 'Archived settings'
} finally {
    Remove-Item -LiteralPath $verify -Recurse -Force
}

$archiveHash = Get-Hash $archive
[IO.File]::WriteAllText((Join-Path $output 'SHA256SUMS.txt'),
    "$archiveHash  $([IO.Path]::GetFileName($archive))`r`n",
    [Text.UTF8Encoding]::new($false))

[pscustomobject]@{
    archive = $archive
    size_bytes = (Get-Item -LiteralPath $archive).Length
    sha256 = $archiveHash
    ue4ss_commit = $commit
    default_mod_count = @(Get-ChildItem -LiteralPath $modsRoot -Directory |
        Where-Object Name -ne 'shared').Count
    sdk_backend_included = $false
}
