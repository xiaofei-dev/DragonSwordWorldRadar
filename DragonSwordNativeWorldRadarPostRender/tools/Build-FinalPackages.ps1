[CmdletBinding()]
param(
    [string]$InstallerExe,
    [string]$PluginDll,
    [string]$NativeBuildReceipt,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$version = '2.2.1'
$runtimeLabel = 'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1'
$product = 'DragonSwordNativeWorldRadarPostRender'
$experimentalVersion = 'v3.0.1-Beta0-g1c1a1497'
$setupName = "$product-Setup-$version.exe"
$installerArchiveName = "$product-v$version-Installer.zip"
$manualNoArchiveName = "$product-v$version-Manual-No-UE4SS.zip"
$manualWithArchiveName =
    "$product-v$version-Manual-With-UE4SS-$experimentalVersion.zip"
$expectedExperimentalHashes = [ordered]@{
    'Payload.ExperimentalUE4SS.dll' =
        'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
    'Payload.ExperimentalDwmapi.dll' =
        '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
    'Payload.Experimental.usmap' =
        '0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47'
    'Payload.ExperimentalUE4SS-settings.ini' =
        '4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74'
}

if (-not $InstallerExe) {
    $InstallerExe = Join-Path $projectRoot `
        "dist\work\build\installer\$setupName"
}
if (-not $PluginDll) {
    $PluginDll = Join-Path $projectRoot 'dist\work\build\native\main.dll'
}
if (-not $NativeBuildReceipt) {
    $NativeBuildReceipt = Join-Path `
        (Split-Path -Parent $PluginDll) 'native-build-receipt.json'
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot "dist\final-$version"
}

. (Join-Path $PSScriptRoot 'ReleaseLayout.ps1')

function Resolve-RequiredFile {
    param([string]$Path, [string]$Description)
    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description is missing: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).ProviderPath
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Write-Utf8NoBom {
    param([string]$Path, [string]$Text)
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

function Reset-PlainDirectory {
    param([string]$Path, [string]$RequiredParent, [string]$Description)
    $full = [IO.Path]::GetFullPath($Path)
    $parent = [IO.Path]::GetFullPath($RequiredParent).TrimEnd('\')
    if (-not [string]::Equals(
            [IO.Path]::GetDirectoryName($full),
            $parent,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Description escaped its exact parent: $full"
    }
    if (Test-Path -LiteralPath $full) {
        $item = Get-Item -LiteralPath $full -Force
        if (-not $item.PSIsContainer -or
            ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Description is not a plain directory: $full"
        }
        Assert-DsnwrNoReparseTree -Root $full -Description $Description
        Remove-Item -LiteralPath $full -Recurse -Force
    }
    [void](New-Item -ItemType Directory -Path $full)
    return $full
}

function Assert-EnglishTextTree {
    param([string]$Root)
    foreach ($file in @(Get-ChildItem -LiteralPath $Root -Recurse -Force -File |
            Where-Object { $_.Extension.ToLowerInvariant() -in @('.md', '.txt') })) {
        $text = [Text.UTF8Encoding]::new($false, $true).GetString(
            [IO.File]::ReadAllBytes($file.FullName))
        if ($text -match '[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]') {
            throw "Release text contains CJK characters: $($file.FullName)"
        }
    }
}

function Assert-SafeRelativePath {
    param([string]$Path, [string]$Description)
    $normalized = $Path.Replace('\', '/')
    $segments = @($normalized -split '/')
    if ([string]::IsNullOrWhiteSpace($normalized) -or
        [IO.Path]::IsPathRooted($normalized) -or
        $normalized.Contains(':') -or
        @($segments | Where-Object {
                [string]::IsNullOrWhiteSpace($_) -or
                $_ -eq '.' -or $_ -eq '..'
            }).Count -ne 0) {
        throw "$Description is unsafe: $Path"
    }
    return $normalized
}

function Get-FileMap {
    param([string]$Root)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    Assert-DsnwrNoReparseTree -Root $rootFull -Description 'Release tree'
    $map = [Collections.Generic.Dictionary[string, object]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($file in @(Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File)) {
        $relative = $file.FullName.Substring($rootFull.Length).
            TrimStart('\').Replace('\', '/')
        $relative = Assert-SafeRelativePath $relative 'Release file path'
        if ($map.ContainsKey($relative)) {
            throw "Release tree repeats a case-insensitive path: $relative"
        }
        $map.Add($relative, [pscustomobject]@{
                size = [int64]$file.Length
                sha256 = Get-Sha256 $file.FullName
            })
    }
    return $map
}

function Copy-PlainTree {
    param([string]$Source, [string]$Destination)
    Assert-DsnwrNoReparseTree -Root $Source -Description 'Payload source tree'
    [void](New-Item -ItemType Directory -Path $Destination -Force)
    foreach ($item in @(Get-ChildItem -LiteralPath $Source -Force)) {
        Copy-Item -LiteralPath $item.FullName -Destination $Destination `
            -Recurse -Force
    }
    Assert-DsnwrNoReparseTree -Root $Destination -Description 'Copied payload tree'
}

function Export-AssemblyResource {
    param($Assembly, [string]$Name, [string]$Destination)
    $stream = $Assembly.GetManifestResourceStream($Name)
    if ($null -eq $stream) {
        throw "Setup omits required embedded resource: $Name"
    }
    $parent = [IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($Destination))
    [void][IO.Directory]::CreateDirectory($parent)
    $output = [IO.File]::Open(
        $Destination,
        [IO.FileMode]::CreateNew,
        [IO.FileAccess]::Write,
        [IO.FileShare]::None)
    try { $stream.CopyTo($output) }
    finally {
        $output.Dispose()
        $stream.Dispose()
    }
}

function Read-KeyValueManifest {
    param([string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        throw 'Embedded payload manifest contains a UTF-8 BOM.'
    }
    $text = [Text.UTF8Encoding]::new($false, $true).GetString($bytes)
    $result = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($original in @($text -split '\r?\n')) {
        $line = $original.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith('#')) { continue }
        $separator = $line.IndexOf('=')
        if ($separator -le 0) {
            throw "Embedded payload manifest contains a malformed line: $line"
        }
        $key = $line.Substring(0, $separator).Trim()
        $value = $line.Substring($separator + 1).Trim()
        if ($key.Length -eq 0 -or $value.Length -eq 0 -or
            $result.ContainsKey($key)) {
            throw "Embedded payload manifest contains an empty or duplicate key: $key"
        }
        $result.Add($key, $value)
    }
    return $result
}

function Expand-SafeZip {
    param([string]$ArchivePath, [string]$Destination)
    [void][IO.Directory]::CreateDirectory($Destination)
    $destinationFull = [IO.Path]::GetFullPath($Destination).TrimEnd('\')
    $destinationPrefix = $destinationFull + '\'
    $seen = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    $archive = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
    try {
        foreach ($entry in @($archive.Entries)) {
            if ([string]::IsNullOrWhiteSpace([string]$entry.Name)) {
                throw "ZIP contains a directory or empty entry: $($entry.FullName)"
            }
            $relative = Assert-SafeRelativePath $entry.FullName 'ZIP entry'
            if (-not $seen.Add($relative)) {
                throw "ZIP repeats a case-insensitive path: $relative"
            }
            $target = [IO.Path]::GetFullPath(
                (Join-Path $destinationFull $relative.Replace('/', '\')))
            if (-not $target.StartsWith(
                    $destinationPrefix,
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "ZIP entry escaped its destination: $relative"
            }
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
    } finally { $archive.Dispose() }
    Assert-DsnwrNoReparseTree -Root $destinationFull `
        -Description 'Freshly extracted ZIP'
}

function Write-ChecksumList {
    param([string]$Root)
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $checksumPath = Join-Path $rootFull 'SHA256SUMS.txt'
    if (Test-Path -LiteralPath $checksumPath) {
        throw "Checksum list already exists: $checksumPath"
    }
    $lines = @(Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File |
        Sort-Object { $_.FullName.Substring($rootFull.Length).ToLowerInvariant() } |
        ForEach-Object {
            $relative = $_.FullName.Substring($rootFull.Length).
                TrimStart('\').Replace('\', '/')
            "$(Get-Sha256 $_.FullName)  $relative"
        })
    Write-Utf8NoBom $checksumPath (($lines -join "`r`n") + "`r`n")
}

function Assert-RootEntries {
    param([string]$Root, [string[]]$Expected, [string]$Description)
    $actual = @(Get-ChildItem -LiteralPath $Root -Force |
        Select-Object -ExpandProperty Name | Sort-Object)
    $expectedSorted = @($Expected | Sort-Object)
    if (($actual -join '|') -ne ($expectedSorted -join '|')) {
        throw "$Description root allowlist differs. Expected=$($expectedSorted -join ',') Actual=$($actual -join ',')"
    }
}

function New-AuditedArchive {
    param(
        [string]$Stage,
        [string]$ArchivePath,
        [string]$VerificationRoot
    )
    $stageMap = Get-FileMap $Stage
    if ($stageMap.Count -eq 0) { throw "Archive stage is empty: $Stage" }
    [IO.Compression.ZipFile]::CreateFromDirectory(
        $Stage,
        $ArchivePath,
        [IO.Compression.CompressionLevel]::Optimal,
        $false)

    $archive = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
    try {
        $seen = [Collections.Generic.HashSet[string]]::new(
            [StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in @($archive.Entries)) {
            if ([string]::IsNullOrWhiteSpace([string]$entry.Name)) {
                throw "Public archive contains a directory entry: $($entry.FullName)"
            }
            $relative = Assert-SafeRelativePath $entry.FullName 'Public archive entry'
            if (-not $seen.Add($relative) -or -not $stageMap.ContainsKey($relative)) {
                throw "Public archive contains an unexpected or duplicate entry: $relative"
            }
            if ([int64]$entry.Length -ne [int64]$stageMap[$relative].size) {
                throw "Public archive entry size differs from staging: $relative"
            }
        }
        if ($seen.Count -ne $stageMap.Count) {
            throw 'Public archive entry count differs from staging.'
        }
    } finally { $archive.Dispose() }

    $verificationParent = Split-Path -Parent ([IO.Path]::GetFullPath($VerificationRoot))
    $verification = Reset-PlainDirectory $VerificationRoot $verificationParent `
        'Archive verification stage'
    try {
        Expand-SafeZip -ArchivePath $ArchivePath -Destination $verification
        $verifyMap = Get-FileMap $verification
        if ($verifyMap.Count -ne $stageMap.Count) {
            throw 'Freshly extracted archive count differs from staging.'
        }
        foreach ($relative in $stageMap.Keys) {
            if (-not $verifyMap.ContainsKey($relative) -or
                [int64]$verifyMap[$relative].size -ne [int64]$stageMap[$relative].size -or
                [string]$verifyMap[$relative].sha256 -ne [string]$stageMap[$relative].sha256) {
                throw "Freshly extracted archive differs from staging: $relative"
            }
        }
    } finally {
        Remove-Item -LiteralPath $verification -Recurse -Force
    }
    return [pscustomobject]@{
        name = [IO.Path]::GetFileName($ArchivePath)
        path = $ArchivePath
        size_bytes = [int64](Get-Item -LiteralPath $ArchivePath).Length
        sha256 = Get-Sha256 $ArchivePath
        entry_count = $stageMap.Count
    }
}

function New-ManualStage {
    param(
        [string]$Stage,
        [string]$EmbeddedRuntime,
        [string]$ManualReadme,
        [string]$Notices,
        [string]$Plugin,
        [string]$BuildReceipt,
        [hashtable]$Resources,
        [switch]$IncludeUE4SS
    )
    $copyRoot = $Stage
    $modRoot = Join-Path $Stage "ue4ss\Mods\$product"
    Copy-PlainTree -Source $EmbeddedRuntime -Destination $modRoot

    $configRoot = Join-Path $modRoot 'config'
    foreach ($name in @('visibility', 'diagnostics')) {
        $example = Join-Path $configRoot "$name.example.ini"
        $live = Join-Path $configRoot "$name.ini"
        if (-not (Test-Path -LiteralPath $example -PathType Leaf) -or
            (Test-Path -LiteralPath $live)) {
            throw "Embedded runtime has an invalid $name configuration state."
        }
        Move-Item -LiteralPath $example -Destination $live
        $source = Join-Path $projectRoot "config\$name.ini"
        if ((Get-Sha256 $live) -ne (Get-Sha256 $source)) {
            throw "Manual live $name configuration differs from its public source."
        }
    }

    Copy-Item -LiteralPath $ManualReadme -Destination (Join-Path $Stage 'README.md')
    Copy-Item -LiteralPath $Notices `
        -Destination (Join-Path $Stage 'THIRD_PARTY_NOTICES.txt')
    $modsLine = "$product : 1`r`n"
    Write-Utf8NoBom (Join-Path $Stage 'ue4ss\Mods\mods.txt') $modsLine

    if ($IncludeUE4SS) {
        Copy-Item -LiteralPath $Resources['Payload.ExperimentalDwmapi.dll'] `
            -Destination (Join-Path $copyRoot 'dwmapi.dll')
        $ue4ssRoot = Join-Path $copyRoot 'ue4ss'
        Copy-Item -LiteralPath $Resources['Payload.ExperimentalUE4SS.dll'] `
            -Destination (Join-Path $ue4ssRoot 'UE4SS.dll')
        Copy-Item -LiteralPath $Resources['Payload.ExperimentalUE4SS-settings.ini'] `
            -Destination (Join-Path $ue4ssRoot 'UE4SS-settings.ini')
        Copy-Item -LiteralPath $Resources['Payload.Experimental.usmap'] `
            -Destination (Join-Path $ue4ssRoot 'DS-5.3.2-0+UE5-1c1a1497.usmap')
    }

    Test-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $Plugin `
        -BuildReceiptPath $BuildReceipt `
        -PayloadRoot $modRoot -InstalledConfiguration `
        -AllowUserVisibility -AllowUserDiagnostics | Out-Null
    if (Get-ChildItem -LiteralPath $Stage -Recurse -Force -File |
        Where-Object {
            $_.Name -ieq 'enabled.txt' -or
            $_.Extension -match '(?i)^\.(cmd|bat|ps1)$' -or
            $_.FullName -match '(?i)\\runtime\\(logs|backups|diagnostics)\\' -or
            $_.Name -match '(?i)\.example\.ini$'
        }) {
        throw 'Forbidden local or installer-only state entered a manual archive.'
    }

    Write-ChecksumList $Stage
    $expectedRoot = @('README.md', 'SHA256SUMS.txt', 'THIRD_PARTY_NOTICES.txt', 'ue4ss')
    if ($IncludeUE4SS) { $expectedRoot += 'dwmapi.dll' }
    Assert-RootEntries $Stage $expectedRoot 'Manual stage'
    Assert-EnglishTextTree $Stage
    # The exact runtime payload now includes seven F6 localized-text TGAs and
    # their source-bound manifest in both manual channels.
    $expectedCount = if ($IncludeUE4SS) { 45 } else { 41 }
    $actualCount = @(Get-ChildItem -LiteralPath $Stage -Recurse -Force -File).Count
    if ($actualCount -ne $expectedCount) {
        throw "Manual stage file count differs. Expected=$expectedCount Actual=$actualCount"
    }
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$installer = Resolve-RequiredFile $InstallerExe 'Installer executable'
$plugin = Resolve-RequiredFile $PluginDll 'Current native Radar DLL'
$nativeReceipt = Resolve-RequiredFile `
    $NativeBuildReceipt 'Native build receipt'
$installerSidecar = Resolve-RequiredFile "$installer.sha256" 'Installer checksum sidecar'
$installGuide = Resolve-RequiredFile (Join-Path $projectRoot 'docs\INSTALL.md') `
    'Installer guide'
$manualReadme = Resolve-RequiredFile (Join-Path $projectRoot 'docs\MANUAL_INSTALL.md') `
    'Manual installation guide'
$notices = Resolve-RequiredFile (Join-Path $projectRoot 'THIRD_PARTY_NOTICES.txt') `
    'Third-party notices'

if ([IO.Path]::GetFileName($installer) -cne $setupName) {
    throw "Installer filename does not match the release identity: $installer"
}
$installerHash = Get-Sha256 $installer
$sidecarBytes = [IO.File]::ReadAllBytes($installerSidecar)
if ($sidecarBytes.Length -ge 3 -and
    $sidecarBytes[0] -eq 0xEF -and
    $sidecarBytes[1] -eq 0xBB -and
    $sidecarBytes[2] -eq 0xBF) {
    throw 'Installer checksum sidecar contains a UTF-8 BOM.'
}
$sidecarText = [Text.UTF8Encoding]::new($false, $true).GetString($sidecarBytes)
if (-not [string]::Equals(
        $sidecarText,
        "$installerHash  $setupName`r`n",
        [StringComparison]::Ordinal)) {
    throw 'Installer checksum sidecar is stale or not byte-exact.'
}
$signature = Get-AuthenticodeSignature -LiteralPath $installer
if ([string]$signature.Status -ne 'NotSigned') {
    throw 'Setup signature state differs from the declared unsigned release state.'
}

$outputParent = Split-Path -Parent ([IO.Path]::GetFullPath($OutputDirectory))
if (-not (Test-Path -LiteralPath $outputParent -PathType Container)) {
    [void](New-Item -ItemType Directory -Path $outputParent)
}
$output = Reset-PlainDirectory $OutputDirectory $outputParent 'Release output'
$stageParent = Join-Path $projectRoot 'dist\work\staging'
if (-not (Test-Path -LiteralPath $stageParent -PathType Container)) {
    [void](New-Item -ItemType Directory -Path $stageParent)
}
$installerStage = Reset-PlainDirectory `
    (Join-Path $stageParent "final-$version-installer") $stageParent 'Installer stage'
$manualNoStage = Reset-PlainDirectory `
    (Join-Path $stageParent "final-$version-manual-no-ue4ss") `
    $stageParent 'No-UE4SS manual stage'
$manualWithStage = Reset-PlainDirectory `
    (Join-Path $stageParent "final-$version-manual-with-ue4ss") `
    $stageParent 'With-UE4SS manual stage'
$resourceStage = Reset-PlainDirectory `
    (Join-Path $stageParent "final-$version-embedded-resources") `
    $stageParent 'Embedded resource stage'
$embeddedRuntime = Reset-PlainDirectory `
    (Join-Path $stageParent "final-$version-embedded-runtime") `
    $stageParent 'Embedded runtime stage'

$assembly = [Reflection.Assembly]::LoadFile($installer)
$resources = @{}
foreach ($name in @(
        'Payload.Manifest.ini',
        'Payload.ExperimentalRuntime.zip',
        'Payload.ExperimentalUE4SS.dll',
        'Payload.ExperimentalDwmapi.dll',
        'Payload.Experimental.usmap',
        'Payload.ExperimentalUE4SS-settings.ini',
        'Payload.ThirdPartyNotices.txt')) {
    $destination = Join-Path $resourceStage $name
    Export-AssemblyResource -Assembly $assembly -Name $name -Destination $destination
    $resources[$name] = $destination
}

$manifest = Read-KeyValueManifest $resources['Payload.Manifest.ini']
if ([string]$manifest['version'] -ne $version -or
    [string]$manifest['runtime_label'] -ne $runtimeLabel) {
    throw 'Setup embedded manifest identity differs from the release identity.'
}
if ((Get-Sha256 $resources['Payload.ExperimentalRuntime.zip']) -ne
        [string]$manifest['experimental_runtime_zip_sha256'] -or
    (Get-Sha256 $resources['Payload.ExperimentalUE4SS.dll']) -ne
        [string]$manifest['experimental_ue4ss_sha256'] -or
    (Get-Sha256 $resources['Payload.ExperimentalDwmapi.dll']) -ne
        [string]$manifest['experimental_dwmapi_sha256'] -or
    (Get-Sha256 $resources['Payload.ThirdPartyNotices.txt']) -ne
        [string]$manifest['third_party_notices_sha256']) {
    throw 'Setup embedded resources differ from their payload manifest.'
}
foreach ($name in $expectedExperimentalHashes.Keys) {
    if ((Get-Sha256 $resources[$name]) -ne $expectedExperimentalHashes[$name]) {
        throw "Setup embedded Experimental resource differs: $name"
    }
}
if ((Get-Sha256 $resources['Payload.ThirdPartyNotices.txt']) -ne
        (Get-Sha256 $notices)) {
    throw 'Setup embedded notices differ from the release notices.'
}

Expand-SafeZip -ArchivePath $resources['Payload.ExperimentalRuntime.zip'] `
    -Destination $embeddedRuntime
$embeddedFiles = @(Get-ChildItem -LiteralPath $embeddedRuntime -Recurse -Force -File)
if ($embeddedFiles.Count -ne [int]$manifest['experimental_runtime_file_count']) {
    throw 'Setup embedded runtime file count differs from its manifest.'
}
Test-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $plugin `
    -BuildReceiptPath $nativeReceipt `
    -PayloadRoot $embeddedRuntime | Out-Null

Copy-Item -LiteralPath $installer -Destination (Join-Path $installerStage $setupName)
Copy-Item -LiteralPath $installerSidecar `
    -Destination (Join-Path $installerStage "$setupName.sha256")
Copy-Item -LiteralPath $installGuide `
    -Destination (Join-Path $installerStage 'INSTALL.md')
Copy-Item -LiteralPath $notices `
    -Destination (Join-Path $installerStage 'THIRD_PARTY_NOTICES.txt')
Assert-RootEntries $installerStage @(
    $setupName,
    "$setupName.sha256",
    'INSTALL.md',
    'THIRD_PARTY_NOTICES.txt') 'Installer stage'
Assert-EnglishTextTree $installerStage

New-ManualStage -Stage $manualNoStage -EmbeddedRuntime $embeddedRuntime `
    -ManualReadme $manualReadme `
    -Notices $notices -Plugin $plugin -BuildReceipt $nativeReceipt `
    -Resources $resources
New-ManualStage -Stage $manualWithStage -EmbeddedRuntime $embeddedRuntime `
    -ManualReadme $manualReadme `
    -Notices $notices -Plugin $plugin -BuildReceipt $nativeReceipt `
    -Resources $resources -IncludeUE4SS

$installerArchive = Join-Path $output $installerArchiveName
$manualNoArchive = Join-Path $output $manualNoArchiveName
$manualWithArchive = Join-Path $output $manualWithArchiveName
$installerResult = New-AuditedArchive -Stage $installerStage `
    -ArchivePath $installerArchive `
    -VerificationRoot (Join-Path $stageParent "final-$version-verify-installer")
$manualNoResult = New-AuditedArchive -Stage $manualNoStage `
    -ArchivePath $manualNoArchive `
    -VerificationRoot (Join-Path $stageParent "final-$version-verify-manual-no")
$manualWithResult = New-AuditedArchive -Stage $manualWithStage `
    -ArchivePath $manualWithArchive `
    -VerificationRoot (Join-Path $stageParent "final-$version-verify-manual-with")

$archiveResults = @($installerResult, $manualNoResult, $manualWithResult)
$outerChecksums = @($archiveResults | ForEach-Object {
        "$($_.sha256)  $($_.name)"
    })
$outerChecksumPath = Join-Path $output 'SHA256SUMS.txt'
Write-Utf8NoBom $outerChecksumPath (($outerChecksums -join "`r`n") + "`r`n")

$releaseManifestPath = Join-Path $output 'release-manifest.json'
$releaseManifest = [ordered]@{
    schema_version = 1
    version = $version
    runtime_label = $runtimeLabel
    generated_at_utc = [DateTime]::UtcNow.ToString('o')
    installer_first = $true
    installer_unsigned = $true
    supported_layouts = @('experimental_nested')
    experimental_ue4ss = $experimentalVersion
    archives = @($archiveResults | ForEach-Object {
            [ordered]@{
                name = $_.name
                size_bytes = $_.size_bytes
                sha256 = $_.sha256
                entry_count = $_.entry_count
            }
        })
    native_artifact = [ordered]@{
        size_bytes = [int64](Get-Item -LiteralPath $plugin).Length
        sha256 = Get-Sha256 $plugin
    }
    setup_artifact = [ordered]@{
        name = $setupName
        size_bytes = [int64](Get-Item -LiteralPath $installer).Length
        sha256 = $installerHash
    }
    runtime_payload = [ordered]@{
        file_count = $embeddedFiles.Count
        zip_sha256 = Get-Sha256 $resources['Payload.ExperimentalRuntime.zip']
        setup_manual_equivalence = 'SOURCE_BOUND_PENDING_MANUAL_MATRIX'
    }
    source_validation = 'PENDING_BUILD_RELEASE'
    build_validation = 'PENDING_BUILD_RELEASE'
    package_validation = 'PASSED'
    installer_matrix = 'PENDING_BUILD_RELEASE_20_0_0'
    manual_install_matrix = 'PENDING_BUILD_RELEASE_2_0_0'
    archive_verification = 'REEXTRACTED_BYTE_IDENTICAL_THREE_PUBLIC_ZIPS'
    gameplay_acceptance = 'NOT_VALIDATED_FOR_EXACT_ARTIFACT'
    publication_authorization =
        'BLOCKED_PENDING_RIGHTS_AND_SOURCE_PROVENANCE_REVIEW'
}
Write-Utf8NoBom $releaseManifestPath `
    (($releaseManifest | ConvertTo-Json -Depth 8) + "`r`n")
Assert-RootEntries $output @(
    $installerArchiveName,
    $manualNoArchiveName,
    $manualWithArchiveName,
    'release-manifest.json',
    'SHA256SUMS.txt') 'Final release directory'

[pscustomobject]@{
    version = $version
    output = $output
    installer_archive = $installerArchive
    installer_archive_sha256 = $installerResult.sha256
    manual_no_ue4ss_archive = $manualNoArchive
    manual_no_ue4ss_sha256 = $manualNoResult.sha256
    manual_with_ue4ss_archive = $manualWithArchive
    manual_with_ue4ss_sha256 = $manualWithResult.sha256
    release_manifest = $releaseManifestPath
    sha256sums = $outerChecksumPath
    installer = $installer
    installer_sha256 = $installerHash
    installer_unsigned = $true
    runtime_payload_file_count = $embeddedFiles.Count
    runtime_payload_sha256 = Get-Sha256 $resources['Payload.ExperimentalRuntime.zip']
}
