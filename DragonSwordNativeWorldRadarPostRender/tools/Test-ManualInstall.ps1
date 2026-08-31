[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InstallerExe,
    [Parameter(Mandatory = $true)][string]$PluginDll,
    [Parameter(Mandatory = $true)][string]$NativeBuildReceipt,
    [Parameter(Mandatory = $true)][string]$ManualNoUE4SSArchive,
    [Parameter(Mandatory = $true)][string]$ManualWithUE4SSArchive,
    [Parameter(Mandatory = $true)][string]$SupportedGameExecutable,
    [Parameter(Mandatory = $true)][string]$WorkingDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'ReleaseLayout.ps1')

$version = '2.1.0'
$runtimeLabel = 'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0'
$product = 'DragonSwordNativeWorldRadarPostRender'
$expectedTestCount = 2
$expectedNoFileCount = 33
$expectedWithFileCount = 37
$expectedLoaderHashes = [ordered]@{
    'ue4ss/UE4SS.dll' =
        'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
    'dwmapi.dll' =
        '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
    'ue4ss/DS-5.3.2-0+UE5-1c1a1497.usmap' =
        '0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47'
    'ue4ss/UE4SS-settings.ini' =
        '4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74'
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if (-not [object]::Equals($Actual, $Expected)) {
        throw "$Message Expected=[$Expected] Actual=[$Actual]"
    }
}

function Resolve-RequiredLeaf {
    param([string]$Path, [string]$Description)
    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).ProviderPath
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Get-SourceState {
    param([string]$Path)
    $item = Get-Item -LiteralPath $Path -Force
    return [pscustomobject]@{
        path = $item.FullName
        length = [int64]$item.Length
        sha256 = Get-Sha256 $item.FullName
        last_write_utc_ticks = [int64]$item.LastWriteTimeUtc.Ticks
        attributes = [int]$item.Attributes
    }
}

function Test-SourceStateEqual {
    param($Before, $After)
    foreach ($property in @(
            'path', 'length', 'sha256', 'last_write_utc_ticks', 'attributes')) {
        if (-not [object]::Equals($Before.$property, $After.$property)) {
            return $false
        }
    }
    return $true
}

function Assert-SafeRelativePath {
    param([string]$Path, [string]$Description)
    $normalized = $Path.Replace('\', '/')
    $segments = @($normalized -split '/')
    $bad = @($segments | Where-Object {
            [string]::IsNullOrWhiteSpace($_) -or $_ -eq '.' -or $_ -eq '..'
        }).Count
    if ([string]::IsNullOrWhiteSpace($normalized) -or
        [IO.Path]::IsPathRooted($normalized) -or
        $normalized.Contains(':') -or $bad -ne 0) {
        throw "$Description is unsafe: $Path"
    }
    return $normalized
}

function Remove-DisposableTree {
    param([string]$Path, [string]$ExpectedParent)
    $full = [IO.Path]::GetFullPath($Path)
    $parent = [IO.Path]::GetFullPath($ExpectedParent).TrimEnd('\')
    Assert-True ([string]::Equals(
            [IO.Path]::GetDirectoryName($full),
            $parent,
            [StringComparison]::OrdinalIgnoreCase)) `
        'Disposable test root escaped its exact working directory.'
    if (-not (Test-Path -LiteralPath $full)) { return }
    $item = Get-Item -LiteralPath $full -Force
    Assert-True ($item.PSIsContainer -and
        ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) `
        'Disposable test root is not a plain directory.'
    Assert-DsnwrNoReparseTree -Root $full -Description 'Disposable test tree'
    Remove-Item -LiteralPath $full -Recurse -Force
}

function Expand-SafeZip {
    param([string]$ArchivePath, [string]$Destination)
    [void][IO.Directory]::CreateDirectory($Destination)
    $root = [IO.Path]::GetFullPath($Destination).TrimEnd('\')
    $prefix = $root + '\'
    $seen = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    $archive = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
    try {
        foreach ($entry in @($archive.Entries)) {
            Assert-True (-not [string]::IsNullOrWhiteSpace([string]$entry.Name)) `
                "ZIP contains a directory or empty entry: $($entry.FullName)"
            $relative = Assert-SafeRelativePath $entry.FullName 'ZIP entry'
            Assert-True ($seen.Add($relative)) `
                "ZIP repeats a case-insensitive path: $relative"
            $target = [IO.Path]::GetFullPath(
                (Join-Path $root $relative.Replace('/', '\')))
            Assert-True ($target.StartsWith(
                    $prefix, [StringComparison]::OrdinalIgnoreCase)) `
                "ZIP entry escaped its destination: $relative"
            [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target))
            $input = $entry.Open()
            $output = [IO.File]::Open(
                $target,
                [IO.FileMode]::CreateNew,
                [IO.FileAccess]::Write,
                [IO.FileShare]::None)
            try { $input.CopyTo($output) }
            finally {
                $output.Dispose()
                $input.Dispose()
            }
        }
    } finally {
        $archive.Dispose()
    }
    Assert-DsnwrNoReparseTree -Root $root -Description 'Extracted archive'
}

function Copy-PlainTree {
    param([string]$Source, [string]$Destination)
    Assert-DsnwrNoReparseTree -Root $Source -Description 'Manual copy source'
    [void][IO.Directory]::CreateDirectory($Destination)
    foreach ($item in @(Get-ChildItem -LiteralPath $Source -Force)) {
        Copy-Item -LiteralPath $item.FullName -Destination $Destination -Recurse -Force
    }
    Assert-DsnwrNoReparseTree -Root $Destination -Description 'Manual copy destination'
}

function Get-TreeFileMap {
    param([string]$Root, [string[]]$Excluded = @())
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    Assert-DsnwrNoReparseTree -Root $rootFull -Description 'Compared tree'
    $exclude = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($relative in $Excluded) {
        [void]$exclude.Add($relative.Replace('\', '/'))
    }
    $map = [Collections.Generic.Dictionary[string, object]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($file in @(Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File)) {
        $relative = $file.FullName.Substring($rootFull.Length).
            TrimStart('\').Replace('\', '/')
        $relative = Assert-SafeRelativePath $relative 'Compared file'
        if ($exclude.Contains($relative)) { continue }
        Assert-True (-not $map.ContainsKey($relative)) `
            "Compared tree repeats a path: $relative"
        $map.Add($relative, [pscustomobject]@{
                size = [int64]$file.Length
                sha256 = Get-Sha256 $file.FullName
            })
    }
    return $map
}

function Assert-TreeEquivalent {
    param([string]$ActualRoot, [string]$ExpectedRoot)
    $actual = Get-TreeFileMap $ActualRoot
    $expected = Get-TreeFileMap $ExpectedRoot
    Assert-Equal $actual.Count $expected.Count 'Tree file counts differ.'
    foreach ($relative in $expected.Keys) {
        Assert-True ($actual.ContainsKey($relative)) "Tree is missing: $relative"
        Assert-True (
            [int64]$actual[$relative].size -eq [int64]$expected[$relative].size -and
            [string]$actual[$relative].sha256 -eq [string]$expected[$relative].sha256) `
            "Tree bytes differ: $relative"
    }
}

function Assert-ExactRootEntries {
    param([string]$Root, [string[]]$Expected, [string]$Description)
    $actual = @(Get-ChildItem -LiteralPath $Root -Force |
        Select-Object -ExpandProperty Name | Sort-Object)
    $expectedSorted = @($Expected | Sort-Object)
    Assert-Equal ($actual -join '|') ($expectedSorted -join '|') `
        "$Description root allowlist differs."
}

function Assert-ChecksumCoverage {
    param([string]$Root)
    $path = Join-Path $Root 'SHA256SUMS.txt'
    $bytes = [IO.File]::ReadAllBytes($path)
    Assert-True (-not ($bytes.Length -ge 3 -and
            $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and
            $bytes[2] -eq 0xBF)) 'Checksum list contains a UTF-8 BOM.'
    $text = [Text.UTF8Encoding]::new($false, $true).GetString($bytes)
    Assert-True ($text.EndsWith("`r`n", [StringComparison]::Ordinal)) `
        'Checksum list is not CRLF terminated.'
    $entries = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($line in @($text -split '\r\n' | Where-Object Length -ne 0)) {
        Assert-True ($line -cmatch '^([0-9A-F]{64})  (.+)$') `
            "Checksum line is malformed: $line"
        $relative = Assert-SafeRelativePath $Matches[2] 'Checksum path'
        Assert-True ($relative -cne 'SHA256SUMS.txt' -and
            -not $entries.ContainsKey($relative)) `
            "Checksum path is recursive or repeated: $relative"
        $entries.Add($relative, $Matches[1])
    }
    $actual = Get-TreeFileMap -Root $Root -Excluded @('SHA256SUMS.txt')
    Assert-Equal $entries.Count $actual.Count 'Checksum coverage count differs.'
    foreach ($relative in $actual.Keys) {
        Assert-True ($entries.ContainsKey($relative)) `
            "Checksum list omits: $relative"
        Assert-Equal $entries[$relative] ([string]$actual[$relative].sha256) `
            "Checksum differs: $relative"
    }
}

function Assert-LoaderHashes {
    param([string]$Win64Root)
    foreach ($relative in $expectedLoaderHashes.Keys) {
        $path = Join-Path $Win64Root $relative.Replace('/', '\')
        Assert-True (Test-Path -LiteralPath $path -PathType Leaf) `
            "Experimental runtime file is missing: $relative"
        Assert-Equal (Get-Sha256 $path) $expectedLoaderHashes[$relative] `
            "Experimental runtime hash differs: $relative"
    }
}

function Assert-ModAuthority {
    param([string]$ModsTextPath, [switch]$AllowUnrelated)
    Assert-True (Test-Path -LiteralPath $ModsTextPath -PathType Leaf) `
        'Controlling mods.txt is missing.'
    $lines = @(Get-Content -LiteralPath $ModsTextPath)
    $radar = @($lines | Where-Object {
            $_ -match "^\s*$([regex]::Escape($product))\s*:\s*1\s*$"
        })
    Assert-Equal $radar.Count 1 'mods.txt does not contain one enabled Radar entry.'
    if ($AllowUnrelated) {
        Assert-True (@($lines | Where-Object { $_ -eq 'ExistingMod : 1' }).Count -eq 1) `
            'Manual activation changed or removed an unrelated Mod entry.'
    }
}

function Assert-ManualPackage {
    param([string]$Root, [switch]$IncludesUE4SS)
    $expectedRoot = @('README.md', 'SHA256SUMS.txt', 'THIRD_PARTY_NOTICES.txt', 'ue4ss')
    if ($IncludesUE4SS) { $expectedRoot += 'dwmapi.dll' }
    Assert-ExactRootEntries -Root $Root -Expected $expectedRoot `
        -Description 'Manual package'
    Assert-ChecksumCoverage $Root
    $copyRoot = $Root
    $target = Join-Path $copyRoot "ue4ss\Mods\$product"
    Test-DsnwrRuntimePayload -ProjectRoot $projectRoot `
        -DllPath $PluginDll `
        -BuildReceiptPath $NativeBuildReceipt `
        -PayloadRoot $target -InstalledConfiguration `
        -AllowUserVisibility -AllowUserDiagnostics | Out-Null
    $release = Get-Content -LiteralPath (Join-Path $target 'metadata\release.json') `
        -Raw | ConvertFrom-Json
    Assert-Equal ([string]$release.version) $version 'Manual version differs.'
    Assert-Equal ([string]$release.runtime_label) $runtimeLabel `
        'Manual runtime label differs.'
    $forbidden = @(Get-ChildItem -LiteralPath $Root -Recurse -Force -File |
        Where-Object {
            $_.Name -ieq 'enabled.txt' -or
            $_.Extension -match '(?i)^\.(cmd|bat|ps1)$' -or
            $_.Name -match '(?i)\.example\.ini$' -or
            $_.FullName -match '(?i)[\\/]runtime[\\/](logs|backups|diagnostics)[\\/]'
        })
    Assert-Equal $forbidden.Count 0 'Manual package contains forbidden state or scripts.'
    $modsText = [IO.File]::ReadAllText(
        (Join-Path $Root 'ue4ss\Mods\mods.txt'),
        [Text.UTF8Encoding]::new($false, $true))
    Assert-Equal $modsText "$product : 1`r`n" 'Packaged mods.txt differs.'
    $actualCount = @(Get-ChildItem -LiteralPath $Root -Recurse -Force -File).Count
    Assert-Equal $actualCount `
        $(if ($IncludesUE4SS) { $expectedWithFileCount } else { $expectedNoFileCount }) `
        'Manual package file count differs.'
    if ($IncludesUE4SS) {
        Assert-LoaderHashes $copyRoot
        Assert-ModAuthority (Join-Path $copyRoot 'ue4ss\Mods\mods.txt')
    } else {
        foreach ($relative in @(
                'dwmapi.dll',
                'ue4ss\UE4SS.dll',
                'ue4ss\UE4SS-settings.ini')) {
            Assert-True (-not (Test-Path -LiteralPath (Join-Path $copyRoot $relative))) `
                "No-UE4SS package unexpectedly contains: $relative"
        }
    }
    return [pscustomobject]@{ copy_root = $copyRoot; target = $target }
}

function Export-SetupRuntime {
    param([string]$Installer, [string]$Destination)
    $archivePath = Join-Path $Destination 'setup-runtime.zip'
    $runtimeRoot = Join-Path $Destination 'setup-runtime'
    [void][IO.Directory]::CreateDirectory($Destination)
    $assembly = [Reflection.Assembly]::LoadFile($Installer)
    $stream = $assembly.GetManifestResourceStream('Payload.ExperimentalRuntime.zip')
    Assert-True ($null -ne $stream) 'Setup omits Payload.ExperimentalRuntime.zip.'
    $output = [IO.File]::Open(
        $archivePath, [IO.FileMode]::CreateNew,
        [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $stream.CopyTo($output) }
    finally {
        $output.Dispose()
        $stream.Dispose()
    }
    Expand-SafeZip -ArchivePath $archivePath -Destination $runtimeRoot
    foreach ($name in @('visibility', 'diagnostics')) {
        Move-Item -LiteralPath (Join-Path $runtimeRoot "config\$name.example.ini") `
            -Destination (Join-Path $runtimeRoot "config\$name.ini")
    }
    return $runtimeRoot
}

function New-Win64Fixture {
    param([string]$Root)
    $win64 = Join-Path $Root 'DS\Binaries\Win64'
    [void][IO.Directory]::CreateDirectory($win64)
    return $win64
}

function Get-ExceptionText {
    param([Exception]$Exception)
    $values = [Collections.Generic.List[string]]::new()
    $current = $Exception
    while ($null -ne $current) {
        if (-not [string]::IsNullOrWhiteSpace($current.Message)) {
            $values.Add($current.Message)
        }
        $current = $current.InnerException
    }
    return ($values -join ' -> ')
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$installer = Resolve-RequiredLeaf $InstallerExe 'Installer executable'
$manualNo = Resolve-RequiredLeaf $ManualNoUE4SSArchive 'No-UE4SS manual archive'
$manualWith = Resolve-RequiredLeaf $ManualWithUE4SSArchive 'With-UE4SS manual archive'
$game = Resolve-RequiredLeaf $SupportedGameExecutable 'Supported game executable'
$working = (Resolve-Path -LiteralPath $WorkingDirectory).ProviderPath
$PluginDll = Resolve-RequiredLeaf $PluginDll 'Native plugin DLL'
$NativeBuildReceipt = Resolve-RequiredLeaf `
    $NativeBuildReceipt 'Native build receipt'
$sourcePaths = @(
    $installer, $manualNo, $manualWith, $game,
    $PluginDll, $NativeBuildReceipt)
$sourceBefore = @($sourcePaths | ForEach-Object { Get-SourceState $_ })
$runRoot = Join-Path $working ('DSNWR-ManualCopy-' + [Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($runRoot)
$expectedRuntime = Export-SetupRuntime -Installer $installer `
    -Destination (Join-Path $runRoot 'setup-payload')

$results = [Collections.Generic.List[object]]::new()
$caseIndex = 0
function Run-Case {
    param([string]$Name, [scriptblock]$Body)
    $script:caseIndex++
    $caseRoot = Join-Path $runRoot ('{0:D2}' -f $script:caseIndex)
    [void][IO.Directory]::CreateDirectory($caseRoot)
    try {
        & $Body $caseRoot
        $results.Add([pscustomobject]@{ name = $Name; status = 'PASSED'; error = $null })
        Write-Host "PASS $($script:caseIndex): $Name"
    } catch {
        $results.Add([pscustomobject]@{
                name = $Name
                status = 'FAILED'
                error = Get-ExceptionText $_.Exception
            })
        Write-Host "FAIL $($script:caseIndex): $Name - $(Get-ExceptionText $_.Exception)"
    }
}

Run-Case 'Manual-No-UE4SS is script-free and copies only the Radar payload' {
    param($caseRoot)
    $noRoot = Join-Path $caseRoot 'manual-no'
    $withRoot = Join-Path $caseRoot 'manual-with-support'
    Expand-SafeZip $manualNo $noRoot
    Expand-SafeZip $manualWith $withRoot
    $no = Assert-ManualPackage $noRoot
    $with = Assert-ManualPackage $withRoot -IncludesUE4SS
    Assert-TreeEquivalent $no.target $expectedRuntime
    Assert-TreeEquivalent $no.target $with.target

    $win64 = New-Win64Fixture (Join-Path $caseRoot 'game')
    foreach ($relative in $expectedLoaderHashes.Keys) {
        $source = Join-Path $with.copy_root $relative.Replace('/', '\')
        $destination = Join-Path $win64 $relative.Replace('/', '\')
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
        Copy-Item -LiteralPath $source -Destination $destination
    }
    $mods = Join-Path $win64 'ue4ss\Mods'
    [void][IO.Directory]::CreateDirectory($mods)
    $modsTxt = Join-Path $mods 'mods.txt'
    [IO.File]::WriteAllText(
        $modsTxt, "ExistingMod : 1`r`n", [Text.UTF8Encoding]::new($false))
    $modsBefore = Get-Sha256 $modsTxt
    Copy-PlainTree $no.target (Join-Path $mods $product)
    Assert-Equal (Get-Sha256 $modsTxt) $modsBefore `
        'Copying only the No-UE4SS Mod folder changed the existing mods.txt.'
    [IO.File]::AppendAllText(
        $modsTxt, "$product : 1`r`n", [Text.UTF8Encoding]::new($false))
    Assert-ModAuthority $modsTxt -AllowUnrelated
    Assert-TreeEquivalent (Join-Path $mods $product) $expectedRuntime

    $missingModsWin64 = New-Win64Fixture (Join-Path $caseRoot 'missing-mods-txt')
    $missingMods = Join-Path $missingModsWin64 'ue4ss\Mods'
    [void][IO.Directory]::CreateDirectory($missingMods)
    Copy-Item -LiteralPath (Join-Path $no.copy_root 'ue4ss\Mods\mods.txt') `
        -Destination (Join-Path $missingMods 'mods.txt')
    Assert-ModAuthority (Join-Path $missingMods 'mods.txt')
}

Run-Case 'Manual-With-UE4SS is a complete script-free clean-target copy' {
    param($caseRoot)
    $root = Join-Path $caseRoot 'manual-with'
    Expand-SafeZip $manualWith $root
    $package = Assert-ManualPackage $root -IncludesUE4SS
    Assert-TreeEquivalent $package.target $expectedRuntime
    $win64 = New-Win64Fixture (Join-Path $caseRoot 'game')
    Copy-PlainTree $package.copy_root $win64
    Assert-LoaderHashes $win64
    Assert-ModAuthority (Join-Path $win64 'ue4ss\Mods\mods.txt')
    Assert-TreeEquivalent `
        (Join-Path $win64 "ue4ss\Mods\$product") $expectedRuntime
    Assert-TreeEquivalent $win64 $package.copy_root
}

$sourceAfter = @($sourcePaths | ForEach-Object { Get-SourceState $_ })
$sourcesUnchanged = $true
for ($index = 0; $index -lt $sourceBefore.Count; $index++) {
    if (-not (Test-SourceStateEqual $sourceBefore[$index] $sourceAfter[$index])) {
        $sourcesUnchanged = $false
    }
}

$fixturesCleaned = $false
try {
    Remove-DisposableTree -Path $runRoot -ExpectedParent $working
    $fixturesCleaned = -not (Test-Path -LiteralPath $runRoot)
} catch {
    $fixturesCleaned = $false
}

$passed = @($results | Where-Object status -eq 'PASSED').Count
$failed = @($results | Where-Object status -eq 'FAILED').Count
$gatePassed = $passed -eq $expectedTestCount -and $failed -eq 0 -and
    $sourcesUnchanged -and $fixturesCleaned

$summary = [pscustomobject]@{
    expected = $expectedTestCount
    passed = $passed
    failed = $failed
    skipped = 0
    release_gate = if ($gatePassed) { 'PASSED' } else { 'FAILED' }
    setup_manual_payload_equivalence = if ($gatePassed) { 'PASSED' } else { 'FAILED' }
    manual_copy_layout_validation = if ($gatePassed) { 'PASSED' } else { 'FAILED' }
    manual_clean_target_policy_validation = if ($gatePassed) { 'PASSED' } else { 'FAILED' }
    sources_unchanged = $sourcesUnchanged
    fixtures_cleaned = $fixturesCleaned
    details = @($results)
}

$summary
if (-not $gatePassed) {
    throw "Manual package release gate failed: passed=$passed failed=$failed " +
        "sources_unchanged=$sourcesUnchanged fixtures_cleaned=$fixturesCleaned"
}
