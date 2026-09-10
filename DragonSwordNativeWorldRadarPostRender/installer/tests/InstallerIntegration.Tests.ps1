[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InstallerExe,
    [Parameter(Mandatory = $true)][string]$SupportedGameExecutable,
    [Parameter(Mandatory = $true)][string]$ExperimentalUE4SSDll,
    [Parameter(Mandatory = $true)][string]$ExperimentalDwmapiDll,
    [Parameter(Mandatory = $true)][string]$WorkingDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$gameCompatibilityPolicy = 'pe32plus-x64-runtime-unique-owner-pointer-pattern'
$expectedUE4SSHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$expectedDwmapiHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$expectedStableUE4SSHash = '8AC18FBFFC1EF96B0662D4A2D537B3F224C26D65CAABA7989A9404C566102B26'
$expectedStableDwmapiHash = 'CE596412BEFA68C30B7F88F65BEB77D9BDAD55E9B96A276A5A9CF690C63F24BB'
$expectedOfficialUE4SSZipHash = '4B47D4BCEDDD2F561A4E395BFA00924CCFC945AF576A2D0C613E6537846C57EC'
$productName = 'DragonSwordNativeWorldRadarPostRender'
$legacyName = 'DragonSwordWorldRadarObjectState'
$externalLegacyName = 'DragonSwordWorldRadar'
$runtimeLabel = 'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_3_0_0'
$expectedTestCount = 29

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

function Get-BytesSha256 {
    param([byte[]]$Bytes)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $hash = $sha.ComputeHash($Bytes)
        return ([BitConverter]::ToString($hash)).Replace('-', '')
    } finally {
        $sha.Dispose()
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if (-not [object]::Equals($Actual, $Expected)) {
        throw "$Message Expected: [$Expected] Actual: [$Actual]"
    }
}

function Assert-PathEqual {
    param([string]$Actual, [string]$Expected, [string]$Message)
    $actualFull = [System.IO.Path]::GetFullPath($Actual).TrimEnd('\')
    $expectedFull = [System.IO.Path]::GetFullPath($Expected).TrimEnd('\')
    if (-not [string]::Equals(
            $actualFull,
            $expectedFull,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Message Expected: [$expectedFull] Actual: [$actualFull]"
    }
}

function Assert-BytesEqual {
    param([byte[]]$Actual, [byte[]]$Expected, [string]$Message)
    if (-not [System.Collections.StructuralComparisons]::StructuralEqualityComparer.Equals(
            $Actual,
            $Expected)) {
        throw "$Message Expected SHA-256: $(Get-BytesSha256 $Expected) Actual SHA-256: $(Get-BytesSha256 $Actual)"
    }
}

function ConvertTo-Utf8Bytes {
    param([string]$Text, [bool]$Bom)
    $body = [System.Text.UTF8Encoding]::new($false).GetBytes($Text)
    if (-not $Bom) { return ,$body }
    $preamble = [System.Text.Encoding]::UTF8.GetPreamble()
    $result = [byte[]]::new($preamble.Length + $body.Length)
    [Buffer]::BlockCopy($preamble, 0, $result, 0, $preamble.Length)
    [Buffer]::BlockCopy($body, 0, $result, $preamble.Length, $body.Length)
    return ,$result
}

function Write-Utf8Text {
    param([string]$Path, [string]$Text, [bool]$Bom = $false)
    $parent = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($Path))
    [void][System.IO.Directory]::CreateDirectory($parent)
    [System.IO.File]::WriteAllBytes($Path, (ConvertTo-Utf8Bytes -Text $Text -Bom $Bom))
}

function Read-Utf8Text {
    param([string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $offset = 0
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $offset = 3
    }
    return [System.Text.UTF8Encoding]::new($false, $true).GetString(
        $bytes,
        $offset,
        $bytes.Length - $offset)
}

function Copy-FixtureFile {
    param([string]$Source, [string]$Destination)
    $parent = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($Destination))
    [void][System.IO.Directory]::CreateDirectory($parent)
    [System.IO.File]::Copy($Source, $Destination, $false)
    [System.IO.File]::SetAttributes($Destination, [System.IO.FileAttributes]::Normal)
}

function Set-FirstByteDifferent {
    param([string]$Path)
    $stream = [System.IO.File]::Open(
        $Path,
        [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None)
    try {
        $value = $stream.ReadByte()
        if ($value -lt 0) { throw "Cannot alter an empty fixture file: $Path" }
        $stream.Position = 0
        $stream.WriteByte([byte]($value -bxor 1))
        $stream.Flush($true)
    } finally {
        $stream.Dispose()
    }
}

function Set-LastByteDifferent {
    param([string]$Path)
    $stream = [System.IO.File]::Open(
        $Path,
        [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None)
    try {
        if ($stream.Length -le 0) { throw "Cannot alter an empty fixture file: $Path" }
        $stream.Position = $stream.Length - 1
        $value = $stream.ReadByte()
        $stream.Position = $stream.Length - 1
        $stream.WriteByte([byte]($value -bxor 1))
        $stream.Flush($true)
    } finally {
        $stream.Dispose()
    }
}

function Get-SourceState {
    param([string]$Path)
    $item = Get-Item -LiteralPath $Path -Force
    return [pscustomobject]@{
        path = $item.FullName
        length = [long]$item.Length
        sha256 = Get-Sha256 $item.FullName
        last_write_utc_ticks = [long]$item.LastWriteTimeUtc.Ticks
        attributes = [int]$item.Attributes
    }
}

function Assert-SourceStateEqual {
    param($Before, $After, [string]$Description)
    foreach ($property in @('path', 'length', 'sha256', 'last_write_utc_ticks', 'attributes')) {
        if (-not [object]::Equals($Before.$property, $After.$property)) {
            throw "$Description changed at property $property."
        }
    }
}

function Remove-FixtureTree {
    param([string]$Path)
    try {
        $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    } catch [System.Management.Automation.ItemNotFoundException] {
        return
    }
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        if ($item.PSIsContainer) {
            [System.IO.Directory]::Delete($item.FullName, $false)
        } else {
            [System.IO.File]::SetAttributes($item.FullName, [System.IO.FileAttributes]::Normal)
            [System.IO.File]::Delete($item.FullName)
        }
        return
    }
    if ($item.PSIsContainer) {
        $children = @([System.IO.Directory]::GetFileSystemEntries($item.FullName) |
            Sort-Object {
                $attributes = [System.IO.File]::GetAttributes($_)
                if (($attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                    return 0
                }
                return 1
            })
        foreach ($child in $children) {
            Remove-FixtureTree -Path $child
        }
        [System.IO.Directory]::Delete($item.FullName, $false)
    } else {
        [System.IO.File]::SetAttributes($item.FullName, [System.IO.FileAttributes]::Normal)
        [System.IO.File]::Delete($item.FullName)
    }
}

function Get-TreeFingerprint {
    param([string]$Root)
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    if (-not (Test-Path -LiteralPath $rootFull)) { return '<ABSENT>' }
    $records = [System.Collections.Generic.List[string]]::new()
    $pending = [System.Collections.Generic.Stack[string]]::new()
    $pending.Push($rootFull)
    while ($pending.Count -ne 0) {
        $current = $pending.Pop()
        foreach ($child in [System.IO.Directory]::GetFileSystemEntries($current)) {
            $full = [System.IO.Path]::GetFullPath($child)
            $relative = $full.Substring($rootFull.Length).TrimStart('\').Replace('\', '/')
            $attributes = [System.IO.File]::GetAttributes($full)
            $reparse = ($attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0
            if ([System.IO.Directory]::Exists($full)) {
                $records.Add("D|$relative|$([int]$attributes)")
                if (-not $reparse) { $pending.Push($full) }
            } else {
                $item = Get-Item -LiteralPath $full -Force
                if ($reparse) {
                    $hash = '<REPARSE>'
                } else {
                    $hash = Get-Sha256 $full
                }
                $records.Add("F|$relative|$([int]$attributes)|$([long]$item.Length)|$hash")
            }
        }
    }
    return (@($records | Sort-Object) -join "`n")
}

function New-ValidFixture {
    param([string]$Root)
    $win64 = Join-Path $Root 'DS\Binaries\Win64'
    $ue4ss = Join-Path $win64 'ue4ss'
    $mods = Join-Path $ue4ss 'Mods'
    [void][System.IO.Directory]::CreateDirectory($mods)
    $game = Join-Path $win64 'DSClient-Win64-Shipping.exe'
    $loader = Join-Path $ue4ss 'UE4SS.dll'
    $proxy = Join-Path $win64 'dwmapi.dll'
    Copy-FixtureFile -Source $script:ResolvedGame -Destination $game
    Copy-FixtureFile -Source $script:ResolvedUE4SS -Destination $loader
    Copy-FixtureFile -Source $script:ResolvedDwmapi -Destination $proxy
    Write-Utf8Text -Path (Join-Path $ue4ss 'UE4SS-settings.ini') `
        -Text "[Overrides]`r`n" -Bom $false
    Write-Utf8Text -Path (Join-Path $mods 'mods.txt') -Text '' -Bom $false
    return [pscustomobject]@{
        Root = $Root
        GameRoot = Join-Path $Root 'DS'
        Win64 = $win64
        UE4SS = $ue4ss
        Mods = $mods
        ModsTxt = Join-Path $mods 'mods.txt'
        GameExe = $game
        Loader = $loader
        Proxy = $proxy
        Target = Join-Path $mods $script:ProductName
    }
}

function Write-OwnedMetadata {
    param([string]$ModDirectory, [string]$Name, [string]$Version = '3.0.0')
    $releasePath = Join-Path $ModDirectory 'metadata\release.json'
    if ($Name -eq $script:ProductName) {
        Write-Utf8Text -Path $releasePath `
            -Text ('{"schema_version":5,"name":"' + $Name +
                '","version":"' + $Version + '","runtime_label":"' +
                $script:RuntimeLabel + '"}' + "`n") -Bom $false
        $pluginPath = Join-Path $ModDirectory 'dlls\main.dll'
        [void][System.IO.Directory]::CreateDirectory((Split-Path -Parent $pluginPath))
        [System.IO.File]::WriteAllBytes(
            $pluginPath,
            (Get-ResourcePayloadFile -Path 'dlls/main.dll'))
        $files = @(
            [ordered]@{
                path = 'dlls/main.dll'
                size = [int64](Get-Item -LiteralPath $pluginPath).Length
                sha256 = Get-Sha256 $pluginPath
            },
            [ordered]@{
                path = 'metadata/release.json'
                size = [int64](Get-Item -LiteralPath $releasePath).Length
                sha256 = Get-Sha256 $releasePath
            }
        )
        $manifest = [ordered]@{
            schema_version = 1
            name = $Name
            version = $Version
            file_count = $files.Count
            files = $files
        }
        Write-Utf8Text -Path (Join-Path $ModDirectory 'metadata\package-manifest.json') `
            -Text (($manifest | ConvertTo-Json -Depth 5) + "`n") -Bom $false
    } else {
        Write-Utf8Text -Path $releasePath `
            -Text ('{"schema_version":5,"name":"' + $Name +
                '","version":"' + $Version + '"}' + "`n") -Bom $false
    }
}

function Get-ExceptionText {
    param([System.Exception]$Exception)
    $messages = [System.Collections.Generic.List[string]]::new()
    $current = $Exception
    while ($null -ne $current) {
        if (-not [string]::IsNullOrWhiteSpace($current.Message)) {
            $messages.Add($current.Message)
        }
        $current = $current.InnerException
    }
    return ($messages -join ' | ')
}

function Invoke-InstallerInstall {
    param([string]$GameExecutable)
    return $script:InstallMethod.Invoke($null, [object[]]@($GameExecutable))
}

function Invoke-InstallerInspect {
    param([string]$GameExecutable)
    return $script:InspectMethod.Invoke($null, [object[]]@($GameExecutable))
}

function Invoke-InstallerInstallConfirmed {
    param([string]$GameExecutable, [string]$IdentityToken)
    return $script:InstallConfirmedMethod.Invoke(
        $null,
        [object[]]@($GameExecutable, $IdentityToken))
}

function Get-InternalProperty {
    param($Object, [string]$Name)
    $property = $Object.GetType().GetProperty(
        $Name,
        [System.Reflection.BindingFlags]'Instance, Public, NonPublic')
    if ($null -eq $property) { throw "Installer result property is missing: $Name" }
    return $property.GetValue($Object, $null)
}

function Invoke-ZeroMutationFailure {
    param($Fixture, [string]$ExpectedMessage)
    $before = Get-TreeFingerprint -Root $Fixture.Root
    $caught = $null
    try {
        [void](Invoke-InstallerInstall -GameExecutable $Fixture.GameExe)
    } catch {
        $caught = $_.Exception
    }
    Assert-True ($null -ne $caught) 'The installer unexpectedly accepted a fail-closed fixture.'
    $message = Get-ExceptionText -Exception $caught
    Assert-True ($message -match $ExpectedMessage) `
        "Installer failure did not match [$ExpectedMessage]. Actual: $message"
    $after = Get-TreeFingerprint -Root $Fixture.Root
    Assert-Equal $after $before 'A fail-closed installer case mutated its fixture.'
}

function Get-ResourceBytes {
    param([string]$Name)
    $stream = $script:InstallerAssembly.GetManifestResourceStream($Name)
    if ($null -eq $stream) { throw "Embedded resource is missing: $Name" }
    try {
        $output = [System.IO.MemoryStream]::new()
        try {
            $stream.CopyTo($output)
            return ,$output.ToArray()
        } finally {
            $output.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
}

function Expand-ApprovedStableUE4SS {
    param([string]$Win64Directory)
    $bytes = Get-ResourceBytes -Name 'Payload.UE4SS.zip'
    $memory = [System.IO.MemoryStream]::new($bytes, $false)
    try {
        $archive = [System.IO.Compression.ZipArchive]::new(
            $memory,
            [System.IO.Compression.ZipArchiveMode]::Read,
            $false)
        try {
            foreach ($relative in @('UE4SS.dll', 'dwmapi.dll', 'UE4SS-settings.ini', 'Mods/mods.txt')) {
                $entry = $archive.GetEntry($relative)
                Assert-True ($null -ne $entry) "Official UE4SS fixture entry is missing: $relative"
                $target = Join-Path $Win64Directory ($relative.Replace('/', '\'))
                $parent = [System.IO.Path]::GetDirectoryName($target)
                [void][System.IO.Directory]::CreateDirectory($parent)
                $input = $entry.Open()
                try {
                    $output = [System.IO.File]::Open(
                        $target,
                        [System.IO.FileMode]::Create,
                        [System.IO.FileAccess]::Write,
                        [System.IO.FileShare]::None)
                    try { $input.CopyTo($output) }
                    finally { $output.Dispose() }
                } finally { $input.Dispose() }
            }
        } finally { $archive.Dispose() }
    } finally { $memory.Dispose() }
}

function Get-ManifestMap {
    $bytes = Get-ResourceBytes -Name 'Payload.Manifest.ini'
    Assert-True (-not ($bytes.Length -ge 3 -and
            $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)) `
        'The embedded payload manifest must be UTF-8 without a BOM.'
    $text = [System.Text.UTF8Encoding]::new($false, $true).GetString($bytes)
    $map = [System.Collections.Generic.Dictionary[string, string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($original in $text.Split(@("`r`n", "`n"), [System.StringSplitOptions]::None)) {
        $line = $original.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith('#')) { continue }
        $separator = $line.IndexOf('=')
        if ($separator -le 0) { throw "Malformed embedded manifest line: $line" }
        $key = $line.Substring(0, $separator).Trim()
        $value = $line.Substring($separator + 1).Trim()
        if ($value.Length -eq 0 -or $map.ContainsKey($key)) {
            throw "Empty or duplicate embedded manifest key: $key"
        }
        $map.Add($key, $value)
    }
    return $map
}

function Get-ManifestPayload {
    param($Manifest, [string]$Variant)
    $fileCountKey = $Variant + '_runtime_file_count'
    [int]$count = 0
    if (-not [int]::TryParse(
            $Manifest[$fileCountKey],
            [System.Globalization.NumberStyles]::None,
            [System.Globalization.CultureInfo]::InvariantCulture,
            [ref]$count) -or $count -le 0 -or $count -gt 128) {
        throw 'The embedded runtime_file_count is invalid.'
    }
    $payload = [System.Collections.Generic.Dictionary[string, object]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    for ($index = 0; $index -lt $count; ++$index) {
        $prefix = '{0}_file_{1:D3}_' -f $Variant, $index
        $path = $Manifest[$prefix + 'path'].Replace('\', '/')
        [long]$size = 0
        if ([string]::IsNullOrWhiteSpace($path) -or
            -not [long]::TryParse(
                $Manifest[$prefix + 'size'],
                [System.Globalization.NumberStyles]::None,
                [System.Globalization.CultureInfo]::InvariantCulture,
                [ref]$size) -or
            $Manifest[$prefix + 'sha256'] -notmatch '^[0-9A-Fa-f]{64}$' -or
            $payload.ContainsKey($path)) {
            throw "Invalid embedded payload contract at index $index."
        }
        $payload.Add($path, [pscustomobject]@{
            Path = $path
            Size = $size
            Sha256 = $Manifest[$prefix + 'sha256'].ToUpperInvariant()
        })
    }
    return $payload
}

function Test-EmbeddedResourceContract {
    $actualNames = @($script:InstallerAssembly.GetManifestResourceNames() | Sort-Object)
    $expectedNames = @(
        'Payload.Manifest.ini',
        'Payload.ExperimentalRuntime.zip',
        'Payload.StableRuntime.zip',
        'Payload.UE4SS.zip',
        'Payload.ThirdPartyNotices.txt'
    ) | Sort-Object
    Assert-Equal ($actualNames -join '|') ($expectedNames -join '|') `
        'The installer embedded resource set is not exact.'

    $manifest = Get-ManifestMap
    $fixed = [ordered]@{
        version = '3.0.0'
        runtime_label = $script:RuntimeLabel
        game_compatibility_policy = $script:GameCompatibilityPolicy
        stable_ue4ss_sha256 = $script:ExpectedStableUE4SSHash
        experimental_ue4ss_sha256 = $script:ExpectedUE4SSHash
        stable_dwmapi_sha256 = $script:ExpectedStableDwmapiHash
        experimental_dwmapi_sha256 = $script:ExpectedDwmapiHash
        official_ue4ss_zip_sha256 = $script:ExpectedOfficialUE4SSZipHash
    }
    foreach ($key in $fixed.Keys) {
        Assert-Equal $manifest[$key] $fixed[$key] "Embedded manifest value is stale for $key."
    }
    foreach ($key in @(
            'experimental_runtime_zip_sha256',
            'stable_runtime_zip_sha256',
            'third_party_notices_sha256')) {
        Assert-True ($manifest[$key] -match '^[0-9A-Fa-f]{64}$') `
            "Embedded manifest SHA-256 is invalid for $key."
    }
    $experimentalPayload = Get-ManifestPayload -Manifest $manifest -Variant 'experimental'
    $stablePayload = Get-ManifestPayload -Manifest $manifest -Variant 'stable'
    Assert-Equal $manifest.Count `
        (13 + (3 * ($experimentalPayload.Count + $stablePayload.Count))) `
        'The embedded manifest contains an unexpected key set.'

    $officialUE4SSZipBytes = Get-ResourceBytes -Name 'Payload.UE4SS.zip'
    $noticeBytes = Get-ResourceBytes -Name 'Payload.ThirdPartyNotices.txt'
    Assert-Equal (Get-BytesSha256 $officialUE4SSZipBytes) $script:ExpectedOfficialUE4SSZipHash `
        'Embedded official UE4SS ZIP hash does not match the approved release.'
    Assert-Equal (Get-BytesSha256 $noticeBytes) $manifest['third_party_notices_sha256'].ToUpperInvariant() `
        'Embedded notices hash does not match the manifest.'

    foreach ($contract in @(
            [pscustomobject]@{
                Variant = 'experimental'
                Resource = 'Payload.ExperimentalRuntime.zip'
                Payload = $experimentalPayload
            },
            [pscustomobject]@{
                Variant = 'stable'
                Resource = 'Payload.StableRuntime.zip'
                Payload = $stablePayload
            })) {
        $zipBytes = Get-ResourceBytes -Name $contract.Resource
        $zipHashKey = $contract.Variant + '_runtime_zip_sha256'
        Assert-Equal (Get-BytesSha256 $zipBytes) $manifest[$zipHashKey].ToUpperInvariant() `
            "Embedded $($contract.Variant) runtime ZIP hash does not match the manifest."
        $seen = [System.Collections.Generic.HashSet[string]]::new(
            [System.StringComparer]::OrdinalIgnoreCase)
        $zipNoticeBytes = $null
        $memory = [System.IO.MemoryStream]::new($zipBytes, $false)
        try {
            $archive = [System.IO.Compression.ZipArchive]::new(
                $memory,
                [System.IO.Compression.ZipArchiveMode]::Read,
                $false)
            try {
                foreach ($entry in $archive.Entries) {
                    Assert-True (-not [string]::IsNullOrEmpty($entry.Name)) `
                        "Embedded ZIP contains a directory entry: $($entry.FullName)"
                    $path = $entry.FullName.Replace('\', '/')
                    Assert-True ($seen.Add($path)) "Embedded ZIP repeats a path: $path"
                    Assert-True ($contract.Payload.ContainsKey($path)) `
                        "Embedded ZIP has an unexpected path: $path"
                    $entryContract = $contract.Payload[$path]
                    Assert-Equal ([long]$entry.Length) ([long]$entryContract.Size) `
                        "Embedded ZIP size differs for $path."
                    $entryStream = $entry.Open()
                    try {
                        $output = [System.IO.MemoryStream]::new()
                        try {
                            $entryStream.CopyTo($output)
                            $entryBytes = $output.ToArray()
                        } finally {
                            $output.Dispose()
                        }
                    } finally {
                        $entryStream.Dispose()
                    }
                    Assert-Equal (Get-BytesSha256 $entryBytes) $entryContract.Sha256 `
                        "Embedded ZIP hash differs for $path."
                    if ([string]::Equals(
                            $path,
                            'THIRD_PARTY_NOTICES.txt',
                            [System.StringComparison]::OrdinalIgnoreCase)) {
                        $zipNoticeBytes = $entryBytes
                    }
                    $lower = $path.ToLowerInvariant()
                    Assert-True (-not (
                            $lower -eq 'enabled.txt' -or
                            $lower.EndsWith('/enabled.txt') -or
                            $lower -eq 'config/visibility.ini' -or
                            $lower -eq 'config/diagnostics.ini' -or
                            $lower.StartsWith('runtime/logs/') -or
                            $lower.StartsWith('runtime/backups/') -or
                            $lower.StartsWith('runtime/diagnostics/'))) `
                        "Forbidden local state is embedded: $path"
                }
            } finally {
                $archive.Dispose()
            }
        } finally {
            $memory.Dispose()
        }
        Assert-Equal $seen.Count $contract.Payload.Count `
            "Embedded $($contract.Variant) ZIP file set is incomplete."
        Assert-True ($null -ne $zipNoticeBytes) `
            "Embedded $($contract.Variant) runtime ZIP lacks THIRD_PARTY_NOTICES.txt."
        Assert-BytesEqual $zipNoticeBytes $noticeBytes `
            "The standalone and $($contract.Variant) runtime-copy third-party notices differ."
        foreach ($required in @(
                'dlls/main.dll',
                'config/visibility.example.ini',
                'config/diagnostics.example.ini',
                'metadata/release.json',
                'metadata/native-build-lock.json',
                'metadata/native-build-receipt.json',
                'metadata/package-manifest.json',
                'THIRD_PARTY_NOTICES.txt',
                'README.txt')) {
            Assert-True ($seen.Contains($required)) `
                "Required $($contract.Variant) payload is missing: $required"
        }
    }

    Assert-Equal $experimentalPayload.Count $stablePayload.Count `
        'The ABI runtime payload file counts differ.'
    $variantPaths = @(
        'dlls/main.dll',
        'metadata/native-build-lock.json',
        'metadata/native-build-receipt.json',
        'metadata/package-manifest.json')
    foreach ($path in $experimentalPayload.Keys) {
        Assert-True ($stablePayload.ContainsKey($path)) `
            "Stable runtime payload is missing: $path"
        if ($path -in $variantPaths) {
            Assert-True ($experimentalPayload[$path].Sha256 -ne $stablePayload[$path].Sha256) `
                "ABI-specific payload unexpectedly matches: $path"
        } else {
            Assert-Equal $experimentalPayload[$path].Sha256 $stablePayload[$path].Sha256 `
                "Runtime payloads differ outside the ABI-specific unit: $path"
        }
    }
}

function Assert-CleanInstall {
    param($Fixture, $Result)
    Assert-Equal (Get-InternalProperty -Object $Result -Name 'LayoutDescription') `
        'experimental-nested UE4SS ABI' 'Unexpected installed layout description.'
    Assert-PathEqual (Get-InternalProperty -Object $Result -Name 'ModDirectory') `
        $Fixture.Target 'Unexpected installed Mod directory.'
    Assert-PathEqual (Get-InternalProperty -Object $Result -Name 'ModsTxtPath') `
        $Fixture.ModsTxt 'Unexpected controlling mods.txt.'
    $backup = [string](Get-InternalProperty -Object $Result -Name 'BackupDirectory')
    Assert-True (Test-Path -LiteralPath (Join-Path $backup 'INSTALL-LOG.txt') -PathType Leaf) `
        'A successful install did not produce INSTALL-LOG.txt.'

    $manifest = Get-ManifestMap
    $payload = Get-ManifestPayload -Manifest $manifest -Variant 'experimental'
    foreach ($entry in $payload.Values | Where-Object {
            $_.Path -notin @(
                'config/visibility.example.ini',
                'config/diagnostics.example.ini')
        }) {
        $installed = Join-Path $Fixture.Target ($entry.Path.Replace('/', '\'))
        Assert-True (Test-Path -LiteralPath $installed -PathType Leaf) `
            "Installed payload is missing: $($entry.Path)"
        Assert-Equal (Get-Sha256 $installed) $entry.Sha256 `
            "Installed payload hash differs: $($entry.Path)"
    }
    Assert-InstalledAbiPayload -Target $Fixture.Target -Variant 'experimental' `
        -Context 'ExperimentalNested install'
    Assert-BytesEqual `
        ([System.IO.File]::ReadAllBytes((Join-Path $Fixture.Target 'config\visibility.ini'))) `
        (Get-ResourcePayloadFile -Path 'config/visibility.example.ini') `
        'Clean visibility.ini is not the immutable public default.'
    Assert-BytesEqual `
        ([System.IO.File]::ReadAllBytes((Join-Path $Fixture.Target 'config\diagnostics.ini'))) `
        (Get-ResourcePayloadFile -Path 'config/diagnostics.example.ini') `
        'Clean diagnostics.ini is not the immutable public default.'
    foreach ($example in @(
            'config\visibility.example.ini',
            'config\diagnostics.example.ini')) {
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $Fixture.Target $example))) `
            "Installer-only configuration default leaked into the installed target: $example"
    }
    Assert-True (Test-Path -LiteralPath (Join-Path $Fixture.Target 'INSTALL-RECORD.txt') -PathType Leaf) `
        'Clean install record is missing.'
    Assert-Equal `
        ([System.IO.Directory]::GetFiles($Fixture.Target, 'enabled.txt', [System.IO.SearchOption]::AllDirectories).Count) `
        0 'The installed payload contains enabled.txt.'
}

function Get-ResourcePayloadFile {
    param([string]$Path, [string]$Variant = 'experimental')
    $resourceName = if ($Variant -eq 'stable') {
        'Payload.StableRuntime.zip'
    } else {
        'Payload.ExperimentalRuntime.zip'
    }
    $zipBytes = Get-ResourceBytes -Name $resourceName
    $memory = [System.IO.MemoryStream]::new($zipBytes, $false)
    try {
        $archive = [System.IO.Compression.ZipArchive]::new(
            $memory,
            [System.IO.Compression.ZipArchiveMode]::Read,
            $false)
        try {
            $entry = $archive.GetEntry($Path)
            if ($null -eq $entry) { throw "Embedded payload entry is missing: $Path" }
            $stream = $entry.Open()
            try {
                $output = [System.IO.MemoryStream]::new()
                try {
                    $stream.CopyTo($output)
                    return ,$output.ToArray()
                } finally {
                    $output.Dispose()
                }
            } finally {
                $stream.Dispose()
            }
        } finally {
            $archive.Dispose()
        }
    } finally {
        $memory.Dispose()
    }
}

function Assert-InstalledAbiPayload {
    param(
        [string]$Target,
        [ValidateSet('experimental', 'stable')][string]$Variant,
        [string]$Context
    )
    foreach ($path in @(
            'dlls/main.dll',
            'metadata/native-build-lock.json',
            'metadata/native-build-receipt.json',
            'metadata/package-manifest.json')) {
        $installed = Join-Path $Target ($path.Replace('/', '\'))
        Assert-True (Test-Path -LiteralPath $installed -PathType Leaf) `
            "$Context is missing $path."
        $expected = Get-BytesSha256 `
            (Get-ResourcePayloadFile -Path $path -Variant $Variant)
        Assert-Equal (Get-Sha256 $installed) $expected `
            "$Context installed the wrong $Variant ABI file: $path"
    }
}

function Count-ModEntry {
    param([string]$Text, [string]$Name, [object]$Enabled)
    if ($null -eq $Enabled) {
        $pattern = '(?m)^[\t ]*' + [regex]::Escape($Name) + '[\t ]*:[^\r\n]*(?=\r?$)'
    } else {
        if ([bool]$Enabled) {
            $state = '1'
        } else {
            $state = '0'
        }
        $pattern = '(?m)^[\t ]*' + [regex]::Escape($Name) +
            '[\t ]*:[\t ]*' + $state + '[\t ]*(?=\r?$)'
    }
    return [regex]::Matches($Text, $pattern, [System.Text.RegularExpressions.RegexOptions]::CultureInvariant).Count
}

$script:ResolvedInstaller = Resolve-RequiredLeaf $InstallerExe 'Installer executable'
$script:ResolvedGame = Resolve-RequiredLeaf $SupportedGameExecutable 'Supported game executable'
$script:ResolvedUE4SS = Resolve-RequiredLeaf $ExperimentalUE4SSDll 'Experimental UE4SS DLL'
$script:ResolvedDwmapi = Resolve-RequiredLeaf $ExperimentalDwmapiDll 'Experimental dwmapi proxy'
$script:ProductName = $productName
$script:LegacyName = $legacyName
$script:ExternalLegacyName = $externalLegacyName
$script:RuntimeLabel = $runtimeLabel
$script:GameCompatibilityPolicy = $gameCompatibilityPolicy
$script:ExpectedUE4SSHash = $expectedUE4SSHash
$script:ExpectedDwmapiHash = $expectedDwmapiHash
$script:ExpectedStableUE4SSHash = $expectedStableUE4SSHash
$script:ExpectedStableDwmapiHash = $expectedStableDwmapiHash
$script:ExpectedOfficialUE4SSZipHash = $expectedOfficialUE4SSZipHash

Assert-Equal ([System.IO.Path]::GetFileName($script:ResolvedGame)) `
    'DSClient-Win64-Shipping.exe' 'The supplied game fixture source has the wrong name.'
Assert-Equal ([System.IO.Path]::GetFileName($script:ResolvedUE4SS)) `
    'UE4SS.dll' 'The supplied UE4SS fixture source has the wrong name.'
Assert-Equal ([System.IO.Path]::GetFileName($script:ResolvedDwmapi)) `
    'dwmapi.dll' 'The supplied proxy fixture source has the wrong name.'
Assert-Equal (Get-Sha256 $script:ResolvedUE4SS) $expectedUE4SSHash `
    'The supplied UE4SS fixture source is not the exact supported loader.'
Assert-Equal (Get-Sha256 $script:ResolvedDwmapi) $expectedDwmapiHash `
    'The supplied proxy fixture source is not the exact supported dwmapi.dll.'

$sourceStates = @(
    Get-SourceState $script:ResolvedGame
    Get-SourceState $script:ResolvedUE4SS
    Get-SourceState $script:ResolvedDwmapi
)

if ([string]::IsNullOrWhiteSpace($WorkingDirectory) -or
    -not (Test-Path -LiteralPath $WorkingDirectory -PathType Container)) {
    throw "WorkingDirectory must be a caller-created directory: $WorkingDirectory"
}
$workingRoot = [System.IO.Path]::GetFullPath(
    (Resolve-Path -LiteralPath $WorkingDirectory).ProviderPath)
$workingItem = Get-Item -LiteralPath $workingRoot -Force
if (($workingItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'WorkingDirectory must not be a reparse point.'
}
$runRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $workingRoot ('DSNWR-InstallerTests-' + [Guid]::NewGuid().ToString('N'))))
$workingPrefix = $workingRoot.TrimEnd('\') + '\'
if (-not $runRoot.StartsWith($workingPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Resolved installer test root escaped WorkingDirectory.'
}
foreach ($source in @($script:ResolvedGame, $script:ResolvedUE4SS, $script:ResolvedDwmapi)) {
    $sourceFull = [System.IO.Path]::GetFullPath($source)
    if ($sourceFull.StartsWith($runRoot + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'A fixture source must not be inside the disposable test run root.'
    }
}

$sourceWin64 = [System.IO.DirectoryInfo]::new(
    [System.IO.Path]::GetDirectoryName($script:ResolvedGame))
if ([string]::Equals($sourceWin64.Name, 'Win64', [System.StringComparison]::OrdinalIgnoreCase) -and
    $null -ne $sourceWin64.Parent -and
    [string]::Equals($sourceWin64.Parent.Name, 'Binaries', [System.StringComparison]::OrdinalIgnoreCase) -and
    $null -ne $sourceWin64.Parent.Parent -and
    [string]::Equals($sourceWin64.Parent.Parent.Name, 'DS', [System.StringComparison]::OrdinalIgnoreCase)) {
    $suppliedGameRoot = $sourceWin64.Parent.Parent.FullName.TrimEnd('\')
    if ([string]::Equals(
            $runRoot,
            $suppliedGameRoot,
            [System.StringComparison]::OrdinalIgnoreCase) -or
        $runRoot.StartsWith(
            $suppliedGameRoot + '\',
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'The disposable installer test root must never be inside the supplied game directory.'
    }
}

Add-Type -AssemblyName System.IO.Compression
$script:InstallerAssembly = [System.Reflection.Assembly]::LoadFile($script:ResolvedInstaller)
$engineType = $script:InstallerAssembly.GetType(
    'DragonSwordNativeWorldRadarPostRender.Installer.InstallerEngine',
    $true,
    $false)
$script:InstallMethod = $engineType.GetMethod(
    'Install',
    [System.Reflection.BindingFlags]'Static, NonPublic')
if ($null -eq $script:InstallMethod) {
    throw 'Internal InstallerEngine.Install(string) was not found.'
}
$installParameters = $script:InstallMethod.GetParameters()
if ($installParameters.Count -ne 1 -or $installParameters[0].ParameterType -ne [string]) {
    throw 'Internal InstallerEngine.Install does not have the expected string signature.'
}
$script:InspectMethod = $engineType.GetMethod(
    'Inspect',
    [System.Reflection.BindingFlags]'Static, NonPublic')
if ($null -eq $script:InspectMethod) {
    throw 'Internal InstallerEngine.Inspect(string) was not found.'
}
$inspectParameters = $script:InspectMethod.GetParameters()
if ($inspectParameters.Count -ne 1 -or $inspectParameters[0].ParameterType -ne [string]) {
    throw 'Internal InstallerEngine.Inspect does not have the expected string signature.'
}
$script:InstallConfirmedMethod = $engineType.GetMethod(
    'InstallConfirmed',
    [System.Reflection.BindingFlags]'Static, NonPublic')
if ($null -eq $script:InstallConfirmedMethod) {
    throw 'Internal InstallerEngine.InstallConfirmed(string,string) was not found.'
}
$confirmedParameters = $script:InstallConfirmedMethod.GetParameters()
if ($confirmedParameters.Count -ne 2 -or
    $confirmedParameters[0].ParameterType -ne [string] -or
    $confirmedParameters[1].ParameterType -ne [string]) {
    throw 'Internal InstallerEngine.InstallConfirmed does not have the expected signature.'
}
[void][System.IO.Directory]::CreateDirectory($runRoot)

$script:Results = [System.Collections.Generic.List[object]]::new()
$script:CaseNumber = 0

function Invoke-InstallerCase {
    param([string]$Name, [scriptblock]$Body)
    $script:CaseNumber += 1
    $slug = ($Name -replace '[^A-Za-z0-9]+', '-').Trim('-')
    $fixtureRoot = Join-Path $runRoot ('{0:D2}-{1}' -f $script:CaseNumber, $slug)
    [void][System.IO.Directory]::CreateDirectory($fixtureRoot)
    $failure = $null
    try {
        & $Body $fixtureRoot
    } catch {
        $failure = $_.Exception
    }
    try {
        Remove-FixtureTree -Path $fixtureRoot
        if (Test-Path -LiteralPath $fixtureRoot) {
            throw "Fixture cleanup left a path behind: $fixtureRoot"
        }
    } catch {
        if ($null -eq $failure) {
            $failure = $_.Exception
        } else {
            $failure = [System.InvalidOperationException]::new(
                ((Get-ExceptionText $failure) + ' | Cleanup: ' + (Get-ExceptionText $_.Exception)),
                $failure)
        }
    }
    if ($null -eq $failure) {
        $script:Results.Add([pscustomobject]@{
            number = $script:CaseNumber
            name = $Name
            status = 'PASSED'
            error = $null
        })
        Write-Host ("PASS {0:D2}: {1}" -f $script:CaseNumber, $Name)
    } else {
        $script:Results.Add([pscustomobject]@{
            number = $script:CaseNumber
            name = $Name
            status = 'FAILED'
            error = Get-ExceptionText $failure
        })
        Write-Host ("FAIL {0:D2}: {1}: {2}" -f `
            $script:CaseNumber, $Name, (Get-ExceptionText $failure))
    }
}

Invoke-InstallerCase 'Embedded resources and manifest are exact' {
    param($fixtureRoot)
    Test-EmbeddedResourceContract
}

Invoke-InstallerCase 'Preflight identifies ExperimentalNested without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $before = Get-TreeFingerprint -Root $fixture.Root
    $plan = Invoke-InstallerInspect $fixture.GameExe
    Assert-Equal (Get-InternalProperty $plan 'LayoutDescription') `
        'pinned ExperimentalNested UE4SS (commit 1c1a149)' `
        'Preflight identified the wrong ExperimentalNested layout.'
    Assert-Equal (Get-InternalProperty $plan 'PluginDescription') `
        'ExperimentalNested-compatible Radar DLL' `
        'Preflight selected the wrong ExperimentalNested payload.'
    Assert-Equal (Get-InternalProperty $plan 'BootstrapsUE4SS') $false `
        'Preflight unexpectedly planned to replace ExperimentalNested UE4SS.'
    Assert-Equal (Get-TreeFingerprint -Root $fixture.Root) $before `
        'ExperimentalNested preflight mutated its fixture.'
}

Invoke-InstallerCase 'Preflight identifies StableRoot without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    [System.IO.File]::Delete($fixture.Proxy)
    Expand-ApprovedStableUE4SS -Win64Directory $fixture.Win64
    $before = Get-TreeFingerprint -Root $fixture.Root
    $plan = Invoke-InstallerInspect $fixture.GameExe
    Assert-Equal (Get-InternalProperty $plan 'LayoutDescription') `
        'official UE4SS v3.0.1 StableRoot' `
        'Preflight identified the wrong StableRoot layout.'
    Assert-Equal (Get-InternalProperty $plan 'PluginDescription') `
        'StableRoot-compatible Radar DLL' `
        'Preflight selected the wrong StableRoot payload.'
    Assert-Equal (Get-InternalProperty $plan 'BootstrapsUE4SS') $false `
        'Preflight unexpectedly planned to replace an approved StableRoot install.'
    Assert-Equal (Get-TreeFingerprint -Root $fixture.Root) $before `
        'StableRoot preflight mutated its fixture.'
}

Invoke-InstallerCase 'Preflight plans StableRoot bootstrap without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    [System.IO.File]::Delete($fixture.Proxy)
    $before = Get-TreeFingerprint -Root $fixture.Root
    $plan = Invoke-InstallerInspect $fixture.GameExe
    Assert-Equal (Get-InternalProperty $plan 'BootstrapsUE4SS') $true `
        'Preflight did not plan the approved StableRoot bootstrap.'
    Assert-True ((Get-InternalProperty $plan 'ActionDescription') -match `
        'official UE4SS v3\.0\.1 StableRoot') `
        'Preflight did not clearly describe the approved UE4SS bootstrap.'
    Assert-Equal (Get-TreeFingerprint -Root $fixture.Root) $before `
        'Missing-UE4SS preflight mutated its fixture.'
}

Invoke-InstallerCase 'Unconfirmed or stale preflight token is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $before = Get-TreeFingerprint -Root $fixture.Root
    $caught = $null
    try {
        [void](Invoke-InstallerInstallConfirmed $fixture.GameExe ('0' * 64))
    } catch {
        $caught = $_.Exception
    }
    Assert-True ($null -ne $caught) `
        'InstallConfirmed unexpectedly accepted an invalid plan token.'
    Assert-True ((Get-ExceptionText $caught) -match 'changed after the compatibility prompt') `
        'InstallConfirmed rejected the invalid plan token for an unexpected reason.'
    Assert-Equal (Get-TreeFingerprint -Root $fixture.Root) $before `
        'Rejected confirmed-plan install mutated its fixture.'
}

Invoke-InstallerCase 'Confirmed ExperimentalNested plan installs the confirmed ABI' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $plan = Invoke-InstallerInspect $fixture.GameExe
    $token = Get-InternalProperty $plan 'IdentityToken'
    $result = Invoke-InstallerInstallConfirmed $fixture.GameExe $token
    Assert-CleanInstall -Fixture $fixture -Result $result
    Assert-Equal (Get-InternalProperty $result 'LayoutDescription') `
        'experimental-nested UE4SS ABI' `
        'Confirmed install selected a different ABI.'
}

Invoke-InstallerCase 'Layout change invalidates the confirmed plan without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $plan = Invoke-InstallerInspect $fixture.GameExe
    $token = Get-InternalProperty $plan 'IdentityToken'
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    [System.IO.File]::Delete($fixture.Proxy)
    Expand-ApprovedStableUE4SS -Win64Directory $fixture.Win64
    $before = Get-TreeFingerprint -Root $fixture.Root
    $caught = $null
    try {
        [void](Invoke-InstallerInstallConfirmed $fixture.GameExe $token)
    } catch {
        $caught = $_.Exception
    }
    Assert-True ($null -ne $caught) `
        'InstallConfirmed accepted a layout that changed after confirmation.'
    Assert-True ((Get-ExceptionText $caught) -match 'changed after the compatibility prompt') `
        'Changed-layout confirmation failed for an unexpected reason.'
    Assert-Equal (Get-TreeFingerprint -Root $fixture.Root) $before `
        'Changed-layout confirmation failure mutated its fixture.'
}

Invoke-InstallerCase 'Clean ExperimentalNested install succeeds' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $result = Invoke-InstallerInstall $fixture.GameExe
    Assert-CleanInstall -Fixture $fixture -Result $result
    Assert-Equal (Read-Utf8Text $fixture.ModsTxt) `
        ($script:ProductName + " : 1`n") 'Clean mods.txt authority is not exact.'
}

Invoke-InstallerCase 'UTF-8 BOM and CRLF are preserved' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $input = "# UTF-8 BOM fixture`r`nUnrelatedMod : 0`r`n"
    Write-Utf8Text -Path $fixture.ModsTxt -Text $input -Bom $true
    $expected = ConvertTo-Utf8Bytes `
        -Text ($input + $script:ProductName + " : 1`r`n") -Bom $true
    [void](Invoke-InstallerInstall $fixture.GameExe)
    Assert-BytesEqual ([System.IO.File]::ReadAllBytes($fixture.ModsTxt)) $expected `
        'BOM/CRLF mods.txt bytes were not preserved exactly.'
}

Invoke-InstallerCase 'UTF-8 without BOM and LF normalization are preserved' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $input = "# LF fixture`n$($script:ProductName) : 0`nUnrelatedMod : 1`n$($script:ProductName):1`n"
    Write-Utf8Text -Path $fixture.ModsTxt -Text $input -Bom $false
    $expectedText = "# LF fixture`n$($script:ProductName) : 1`nUnrelatedMod : 1`n`n"
    $expected = ConvertTo-Utf8Bytes -Text $expectedText -Bom $false
    [void](Invoke-InstallerInstall $fixture.GameExe)
    Assert-BytesEqual ([System.IO.File]::ReadAllBytes($fixture.ModsTxt)) $expected `
        'BOM-less LF mods.txt normalization was not byte-exact.'
}

Invoke-InstallerCase 'Recognized upgrade preserves both live configurations' {
    param($fixtureRoot)
    $recognizedVisibilityConfigs = @(
        [pscustomobject]@{
            Name = 'schema-1'
            Text = "; schema 1 visibility`ncompact_mask=3`nworld_mask=4`n"
        },
        [pscustomobject]@{
            Name = 'schema-2'
            Text = "; schema 2 visibility`nschema_version=2`ncompact_mask=5`nworld_mask=6`n"
        },
        [pscustomobject]@{
            Name = 'schema-3'
            Text = "; schema 3 visibility`nschema_version=3`ncompact_mask=7`n" +
                "world_mask=9`narea_quest_mode=all`n"
        },
        [pscustomobject]@{
            Name = 'schema-4'
            Text = "; schema 4 visibility`nschema_version=4`ncompact_mask=11`n" +
                "world_mask=18`narea_quest_mode=available`nassault_mode=all`n"
        },
        [pscustomobject]@{
            Name = 'sectioned-2.1.1'
            Text = "[radar]`nclock=false`ntreasure=true`nboss=true`n" +
                "assault=false`nmini_games=true`narea_quests=true`nbird_eggs=false`n`n" +
                "[map]`ntreasure=true`nboss=false`nassault=true`nmini_games=true`n" +
                "area_quests=false`n`n[modes]`narea_quests=all`nassault=available`n"
        }
    )
    $languageIds = @(
        'auto', 'en', 'ja', 'ko', 'zh-hans', 'zh-hant',
        'fr', 'de', 'es-es', 'ru', 'th', 'pt-br')
    foreach ($languageId in $languageIds) {
        $recognizedVisibilityConfigs += [pscustomobject]@{
            Name = 'sectioned-3.0.0-' + $languageId
            Text = "[radar]`nclock=false`ntreasure=true`nboss=true`n" +
                "assault=false`nmini_games=true`narea_quests=true`nbird_eggs=false`n`n" +
                "[map]`ntreasure=true`nboss=false`nassault=true`nmini_games=true`n" +
                "area_quests=false`n`n[modes]`narea_quests=all`nassault=available`n`n" +
                "[height_arrows]`ntreasure=false`narea_quests=true`nmole=true`n`n" +
                "[interface]`nlanguage=$languageId`n"
        }
    }
    foreach ($recognizedConfig in $recognizedVisibilityConfigs) {
        $fixture = New-ValidFixture (Join-Path $fixtureRoot $recognizedConfig.Name)
        Write-OwnedMetadata -ModDirectory $fixture.Target -Name $script:ProductName
        [void][System.IO.Directory]::CreateDirectory((Join-Path $fixture.Target 'config'))
        $visibility = ConvertTo-Utf8Bytes -Text $recognizedConfig.Text -Bom $true
        $diagnostics = ConvertTo-Utf8Bytes `
            -Text "[diagnostics]`r`n# local diagnostics`r`ndebug_logging=true`r`n" -Bom $false
        [System.IO.File]::WriteAllBytes(
            (Join-Path $fixture.Target 'config\visibility.ini'),
            $visibility)
        [System.IO.File]::WriteAllBytes(
            (Join-Path $fixture.Target 'config\diagnostics.ini'),
            $diagnostics)
        Write-Utf8Text -Path (Join-Path $fixture.Target 'enabled.txt') `
            -Text "owned legacy load marker`n" -Bom $false
        [void](Invoke-InstallerInstall $fixture.GameExe)
        Assert-BytesEqual `
            ([System.IO.File]::ReadAllBytes((Join-Path $fixture.Target 'config\visibility.ini'))) `
            $visibility "Recognized $($recognizedConfig.Name) upgrade changed visibility.ini bytes."
        Assert-BytesEqual `
            ([System.IO.File]::ReadAllBytes((Join-Path $fixture.Target 'config\diagnostics.ini'))) `
            $diagnostics "Recognized $($recognizedConfig.Name) upgrade changed diagnostics.ini bytes."
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $fixture.Target 'enabled.txt'))) `
            "Recognized $($recognizedConfig.Name) upgrade retained an obsolete owned enabled.txt marker."
    }

    $validateVisibility = $engineType.GetMethod(
        'ValidateVisibilityConfig',
        [System.Reflection.BindingFlags]'Static, NonPublic')
    Assert-True ($null -ne $validateVisibility) `
        'The strict visibility validator reflection seam is missing.'
    $baseSections = "[radar]`nclock=true`ntreasure=true`nboss=true`n" +
        "assault=true`nmini_games=true`narea_quests=true`nbird_eggs=true`n" +
        "[map]`ntreasure=true`nboss=true`nassault=true`nmini_games=true`n" +
        "area_quests=true`n[modes]`narea_quests=available`nassault=available`n"
    $invalidVisibilityConfigs = @(
        $baseSections +
            "[height_arrows]`ntreasure=true`narea_quests=false`nmole=false`n",
        $baseSections +
            "[height_arrows]`ntreasure=true`narea_quests=false`nmole=false`n" +
            "[interface]`nlanguage=unknown`n",
        $baseSections +
            "[height_arrows]`ntreasure=true`narea_quests=false`n" +
            "[interface]`nlanguage=auto`n",
        $baseSections +
            "[height_arrows]`ntreasure=TRUE`narea_quests=false`nmole=false`n" +
            "[interface]`nlanguage=auto`n"
    )
    foreach ($invalidVisibility in $invalidVisibilityConfigs) {
        $caught = $null
        try {
            [void]$validateVisibility.Invoke($null, [object[]]@(
                (ConvertTo-Utf8Bytes -Text $invalidVisibility -Bom $false),
                $false))
        } catch {
            $caught = $_.Exception
        }
        Assert-True ($null -ne $caught) `
            'A partial, unknown-language, incomplete, or non-canonical 2.2 visibility config was accepted.'
    }
}

Invoke-InstallerCase 'mods.txt authority is normalized and predecessor state is removed' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $legacy = Join-Path $fixture.Mods $script:LegacyName
    Write-OwnedMetadata -ModDirectory $legacy -Name $script:LegacyName
    $markerBytes = ConvertTo-Utf8Bytes -Text "owned predecessor marker`r`n" -Bom $false
    [System.IO.File]::WriteAllBytes((Join-Path $legacy 'enabled.txt'), $markerBytes)
    $modsText = "$($script:ProductName) : 0`r`n$($script:LegacyName) : 1`r`n" +
        "$($script:ProductName):1`r`n$($script:LegacyName):0`r`nOtherMod : 1`r`n"
    Write-Utf8Text -Path $fixture.ModsTxt -Text $modsText -Bom $false
    $result = Invoke-InstallerInstall $fixture.GameExe
    $installedText = Read-Utf8Text $fixture.ModsTxt
    Assert-Equal (Count-ModEntry $installedText $script:ProductName $null) 1 `
        'Native Radar authority was not normalized to one entry.'
    Assert-Equal (Count-ModEntry $installedText $script:ProductName $true) 1 `
        'Native Radar authority was not enabled.'
    Assert-Equal (Count-ModEntry $installedText $script:LegacyName $null) 0 `
        'The redundant predecessor mods.txt entry was not removed.'
    Assert-Equal (Count-ModEntry $installedText 'OtherMod' $true) 1 `
        'An unrelated mods.txt entry changed during predecessor removal.'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $legacy 'enabled.txt'))) `
        'Owned predecessor enabled.txt was not removed.'
    $backup = [string](Get-InternalProperty $result 'BackupDirectory')
    $backedUpMarker = Join-Path $backup `
        ('original\ue4ss\Mods\' + $script:LegacyName + '\enabled.txt')
    Assert-BytesEqual ([System.IO.File]::ReadAllBytes($backedUpMarker)) $markerBytes `
        'Owned predecessor marker backup is not byte-exact.'
}

Invoke-InstallerCase 'Supported UE4SS path overrides route installation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $customMods = Join-Path $fixture.Win64 'CustomMods'
    $control = Join-Path $fixture.UE4SS 'control\active-mods.txt'
    $additional = Join-Path $fixture.UE4SS 'AdditionalMods'
    [void][System.IO.Directory]::CreateDirectory($customMods)
    [void][System.IO.Directory]::CreateDirectory($additional)
    Write-Utf8Text -Path $control -Text "OtherMod : 1`r`n" -Bom $true
    Write-Utf8Text -Path (Join-Path $fixture.UE4SS 'UE4SS-settings.ini') -Text (
        "[Overrides]`r`n" +
        "ModsFolderPath=CustomMods`r`n" +
        "ControllingModsTxt=control\active-mods.txt`r`n" +
        "+ModsFolderPaths=AdditionalMods`r`n") -Bom $false
    $result = Invoke-InstallerInstall $fixture.GameExe
    $customTarget = Join-Path $customMods $script:ProductName
    Assert-PathEqual (Get-InternalProperty $result 'ModDirectory') $customTarget `
        'ModsFolderPath override was not honored.'
    Assert-PathEqual (Get-InternalProperty $result 'ModsTxtPath') $control `
        'ControllingModsTxt override was not honored.'
    Assert-True (Test-Path -LiteralPath (Join-Path $customTarget 'dlls\main.dll')) `
        'Overridden Mod root did not receive the payload.'
    Assert-True (-not (Test-Path -LiteralPath $fixture.Target)) `
        'Default Mod root was mutated while an override controlled installation.'
    Assert-Equal (Count-ModEntry (Read-Utf8Text $control) $script:ProductName $true) 1 `
        'Overridden controlling mods.txt lacks one enabled authority.'
    Assert-Equal (Get-TreeFingerprint $additional) '' `
        'An unrelated configured additional Mods root was mutated.'
}

Invoke-InstallerCase 'Approved StableRoot layout installs the stable ABI payload' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    [System.IO.File]::Delete($fixture.Proxy)
    Expand-ApprovedStableUE4SS -Win64Directory $fixture.Win64
    $result = Invoke-InstallerInstall $fixture.GameExe
    Assert-Equal (Get-InternalProperty $result 'LayoutDescription') `
        'stable-root UE4SS ABI' 'StableRoot install selected the wrong ABI.'
    $target = Join-Path $fixture.Win64 ('Mods\' + $script:ProductName)
    Assert-InstalledAbiPayload -Target $target -Variant 'stable' `
        -Context 'StableRoot install'
    Assert-Equal (Get-Sha256 (Join-Path $fixture.Win64 'UE4SS.dll')) `
        $script:ExpectedStableUE4SSHash 'StableRoot UE4SS changed unexpectedly.'
    [void](Invoke-InstallerInstall $fixture.GameExe)
    Assert-InstalledAbiPayload -Target $target -Variant 'stable' `
        -Context 'StableRoot reinstall'
}

Invoke-InstallerCase 'Missing UE4SS bootstraps approved StableRoot transactionally' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    [System.IO.File]::Delete($fixture.Proxy)
    $result = Invoke-InstallerInstall $fixture.GameExe
    Assert-Equal (Get-InternalProperty $result 'LayoutDescription') `
        'stable-root UE4SS ABI' 'Bootstrap selected the wrong ABI.'
    Assert-Equal (Get-Sha256 (Join-Path $fixture.Win64 'UE4SS.dll')) `
        $script:ExpectedStableUE4SSHash 'Bootstrap installed the wrong UE4SS.dll.'
    Assert-Equal (Get-Sha256 (Join-Path $fixture.Win64 'dwmapi.dll')) `
        $script:ExpectedStableDwmapiHash 'Bootstrap installed the wrong proxy loader.'
    $target = Join-Path $fixture.Win64 ('Mods\' + $script:ProductName)
    Assert-InstalledAbiPayload -Target $target -Variant 'stable' `
        -Context 'StableRoot bootstrap'
    [void](Invoke-InstallerInstall $fixture.GameExe)
    Assert-InstalledAbiPayload -Target $target -Variant 'stable' `
        -Context 'StableRoot post-bootstrap reinstall'
}

Invoke-InstallerCase 'Missing proxy is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.File]::Delete($fixture.Proxy)
    Invoke-ZeroMutationFailure $fixture 'dwmapi\.dll is missing'
}

Invoke-InstallerCase 'Updated game hash is accepted by structural preflight' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    Set-LastByteDifferent $fixture.GameExe
    $before = Get-TreeFingerprint -Root $fixture.Root
    $plan = Invoke-InstallerInspect $fixture.GameExe
    Assert-True ($null -ne $plan) `
        'Structural preflight rejected a valid updated x64 game image.'
    Assert-Equal (Get-TreeFingerprint -Root $fixture.Root) $before `
        'Structural preflight mutated the updated-game fixture.'
}

Invoke-InstallerCase 'Malformed game image is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    Set-FirstByteDifferent $fixture.GameExe
    Invoke-ZeroMutationFailure $fixture 'not a valid x64 PE32\+ image'
}

Invoke-InstallerCase 'Wrong nested UE4SS hash is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    Set-FirstByteDifferent $fixture.Loader
    Invoke-ZeroMutationFailure $fixture 'UE4SS\.dll is not a supported release'
}

Invoke-InstallerCase 'Wrong StableRoot UE4SS hash is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    [System.IO.File]::Delete($fixture.Proxy)
    Expand-ApprovedStableUE4SS -Win64Directory $fixture.Win64
    Set-FirstByteDifferent (Join-Path $fixture.Win64 'UE4SS.dll')
    Invoke-ZeroMutationFailure $fixture 'UE4SS\.dll is not a supported release'
}

Invoke-InstallerCase 'Wrong proxy hash is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    Set-FirstByteDifferent $fixture.Proxy
    Invoke-ZeroMutationFailure $fixture 'dwmapi\.dll is not the approved proxy'
}

Invoke-InstallerCase 'Nonempty nested layout without UE4SS DLL is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.File]::Delete($fixture.Loader)
    Invoke-ZeroMutationFailure $fixture 'nested ue4ss directory contains files but UE4SS\.dll is missing'
}

Invoke-InstallerCase 'Missing UE4SS with unknown proxy is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    [System.IO.Directory]::Delete($fixture.UE4SS, $true)
    Invoke-ZeroMutationFailure $fixture 'UE4SS is absent.*unknown loader or proxy'
}

Invoke-InstallerCase 'Dual loader layout is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    Copy-FixtureFile -Source $script:ResolvedUE4SS `
        -Destination (Join-Path $fixture.Win64 'UE4SS.dll')
    Invoke-ZeroMutationFailure $fixture 'Both stable-root and experimental-nested'
}

Invoke-InstallerCase 'Competing Mods root is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $competing = Join-Path $fixture.Win64 'Mods'
    [void][System.IO.Directory]::CreateDirectory($competing)
    Write-Utf8Text -Path (Join-Path $competing 'mods.txt') `
        -Text ($script:ProductName + " : 1`r`n") -Bom $false
    Invoke-ZeroMutationFailure $fixture 'Another active UE4SS mods\.txt contains'

    $externalEntryRoot = Join-Path $fixtureRoot 'enabled-external-renderer-entry'
    $externalEntryFixture = New-ValidFixture $externalEntryRoot
    Write-Utf8Text -Path $externalEntryFixture.ModsTxt `
        -Text ($script:ExternalLegacyName + " : 1`r`n") -Bom $false
    Invoke-ZeroMutationFailure $externalEntryFixture `
        'legacy external DragonSwordWorldRadar renderer is enabled'

    $externalMarkerRoot = Join-Path $fixtureRoot 'enabled-external-renderer-marker'
    $externalMarkerFixture = New-ValidFixture $externalMarkerRoot
    Write-Utf8Text -Path (Join-Path $externalMarkerFixture.Mods `
            "$($script:ExternalLegacyName)\enabled.txt") `
        -Text "legacy external authority`n" -Bom $false
    Invoke-ZeroMutationFailure $externalMarkerFixture `
        'legacy external DragonSwordWorldRadar enabled\.txt authority is active'

    $disabledExternalRoot = Join-Path $fixtureRoot 'disabled-external-renderer'
    $disabledExternalFixture = New-ValidFixture $disabledExternalRoot
    Write-Utf8Text -Path $disabledExternalFixture.ModsTxt `
        -Text ($script:ExternalLegacyName + " : 0`r`n") -Bom $false
    [void](Invoke-InstallerInstall $disabledExternalFixture.GameExe)
    $disabledExternalText = Read-Utf8Text $disabledExternalFixture.ModsTxt
    Assert-Equal (Count-ModEntry $disabledExternalText `
            $script:ExternalLegacyName $false) 1 `
        'A disabled legacy external renderer entry was not preserved.'
    Assert-Equal (Count-ModEntry $disabledExternalText `
            $script:ProductName $true) 1 `
        'Native Radar was not enabled alongside the preserved disabled external entry.'
}

Invoke-InstallerCase 'Unknown ownership and malformed same-name authority are rejected' {
    param($fixtureRoot)
    $unknownRoot = Join-Path $fixtureRoot 'unknown-owner'
    $unknownFixture = New-ValidFixture $unknownRoot
    Write-Utf8Text -Path (Join-Path $unknownFixture.Target 'do-not-touch.txt') `
        -Text "unknown owner`n" -Bom $false
    Invoke-ZeroMutationFailure $unknownFixture 'same-name target cannot be proven'

    $unexpectedFileRoot = Join-Path $fixtureRoot 'owned-metadata-unexpected-file'
    $unexpectedFileFixture = New-ValidFixture $unexpectedFileRoot
    Write-OwnedMetadata -ModDirectory $unexpectedFileFixture.Target `
        -Name $script:ProductName
    Write-Utf8Text -Path (Join-Path $unexpectedFileFixture.Target 'user-note.txt') `
        -Text "must not be deleted by a recognized upgrade`n" -Bom $false
    Invoke-ZeroMutationFailure $unexpectedFileFixture `
        'same-name target cannot be proven'

    $spoofRoot = Join-Path $fixtureRoot 'nested-name-spoof'
    $spoofFixture = New-ValidFixture $spoofRoot
    Write-Utf8Text -Path (Join-Path $spoofFixture.Target 'metadata\release.json') `
        -Text ('{"schema_version":5,"name":"OtherMod","version":"0.9.0","nested":{"name":"' +
            $script:ProductName + '"}}' + "`n") -Bom $false
    Write-Utf8Text -Path (Join-Path $spoofFixture.Target 'metadata\package-manifest.json') `
        -Text ('{"schema_version":1,"name":"OtherMod","version":"0.9.0","nested":{"name":"' +
            $script:ProductName + '"}}' + "`n") -Bom $false
    Invoke-ZeroMutationFailure $spoofFixture 'same-name target cannot be proven'

    $tamperedOwnershipRoot = Join-Path $fixtureRoot 'tampered-ownership-manifest'
    $tamperedOwnershipFixture = New-ValidFixture $tamperedOwnershipRoot
    Write-OwnedMetadata -ModDirectory $tamperedOwnershipFixture.Target `
        -Name $script:ProductName
    $tamperedManifestPath = Join-Path $tamperedOwnershipFixture.Target `
        'metadata\package-manifest.json'
    $tamperedManifest = Get-Content -LiteralPath $tamperedManifestPath -Raw |
        ConvertFrom-Json
    $tamperedManifest.files[0].sha256 = ('0' * 64)
    Write-Utf8Text -Path $tamperedManifestPath `
        -Text (($tamperedManifest | ConvertTo-Json -Depth 5) + "`n") -Bom $false
    Invoke-ZeroMutationFailure $tamperedOwnershipFixture `
        'same-name target cannot be proven'

    $malformedRoot = Join-Path $fixtureRoot 'malformed-authority'
    $malformedFixture = New-ValidFixture $malformedRoot
    Write-Utf8Text -Path $malformedFixture.ModsTxt `
        -Text ($script:ProductName + " : enabled`r`nOtherMod : 1`r`n") -Bom $false
    Invoke-ZeroMutationFailure $malformedFixture 'contains a malformed.*entry'
}

Invoke-InstallerCase 'Reparse target is rejected without mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    $junctionTarget = Join-Path $fixtureRoot 'junction-owned-target'
    [void][System.IO.Directory]::CreateDirectory($junctionTarget)
    Write-Utf8Text -Path (Join-Path $junctionTarget 'sentinel.txt') `
        -Text "junction sentinel`n" -Bom $false
    [void](New-Item -ItemType Junction -Path $fixture.Target -Target $junctionTarget)
    Invoke-ZeroMutationFailure $fixture 'reparse point|symbolic link|junction'
}

Invoke-InstallerCase 'Tampered embedded manifest is rejected' {
    param($fixtureRoot)
    $before = Get-TreeFingerprint $fixtureRoot
    $readManifest = $engineType.GetMethod(
        'ReadManifest',
        [System.Reflection.BindingFlags]'Static, NonPublic')
    $validateManifest = $engineType.GetMethod(
        'ValidateManifest',
        [System.Reflection.BindingFlags]'Static, NonPublic')
    Assert-True ($null -ne $readManifest -and $null -ne $validateManifest) `
        'Manifest validation reflection seam is missing.'
    $tampered = $readManifest.Invoke($null, $null)
    $tampered['version'] = '9.9.9-tampered'
    $caught = $null
    try {
        [void]$validateManifest.Invoke($null, [object[]]@($tampered))
    } catch {
        $caught = $_.Exception
    }
    Assert-True ($null -ne $caught) 'A tampered installer manifest was accepted.'
    Assert-True ((Get-ExceptionText $caught) -match 'manifest is stale for version') `
        'Tampered manifest failed for an unexpected reason.'
    Assert-Equal (Get-TreeFingerprint $fixtureRoot) $before `
        'Manifest validation mutated its isolated fixture.'
}

Invoke-InstallerCase 'Injected late failure rolls back every recorded mutation' {
    param($fixtureRoot)
    $fixture = New-ValidFixture $fixtureRoot
    Write-OwnedMetadata -ModDirectory $fixture.Target -Name $script:ProductName
    [void][System.IO.Directory]::CreateDirectory((Join-Path $fixture.Target 'config'))
    $visibility = ConvertTo-Utf8Bytes `
        -Text "compact_mask=3`r`nworld_mask=4`r`n" -Bom $true
    $diagnostics = ConvertTo-Utf8Bytes `
        -Text "event_log_enabled=true`n" -Bom $false
    [System.IO.File]::WriteAllBytes((Join-Path $fixture.Target 'config\visibility.ini'), $visibility)
    [System.IO.File]::WriteAllBytes((Join-Path $fixture.Target 'config\diagnostics.ini'), $diagnostics)
    Write-Utf8Text -Path (Join-Path $fixture.Target 'enabled.txt') `
        -Text "restore owned marker`r`n" -Bom $false
    $legacy = Join-Path $fixture.Mods $script:LegacyName
    Write-OwnedMetadata -ModDirectory $legacy -Name $script:LegacyName
    Write-Utf8Text -Path (Join-Path $legacy 'enabled.txt') `
        -Text "restore marker`n" -Bom $false
    Write-Utf8Text -Path $fixture.ModsTxt -Text (
        "$($script:ProductName) : 0`r`n" +
        "$($script:LegacyName) : 1`r`nOtherMod : 1`r`n") -Bom $true
    $beforeMods = Get-TreeFingerprint $fixture.Mods
    $failurePoint = $engineType.GetField(
        'IntegrationTestFailurePoint',
        [System.Reflection.BindingFlags]'Static, NonPublic')
    Assert-True ($null -ne $failurePoint) `
        'The deterministic late-failure integration seam is missing.'
    $caught = $null
    try {
        $failurePoint.SetValue($null, 'after-recorded-mutations')
        try {
            [void](Invoke-InstallerInstall $fixture.GameExe)
        } catch {
            $caught = $_.Exception
        }
    } finally {
        $failurePoint.SetValue($null, $null)
    }
    Assert-True ($null -ne $caught) 'The injected late installer failure did not occur.'
    $caughtText = Get-ExceptionText $caught
    Assert-True ($caughtText -match `
            'Injected installer integration-test failure at after-recorded-mutations') `
        ('The late-failure seam did not cause the expected failure. Actual: ' + $caughtText)
    Assert-True ($caughtText -match 'all recorded file mutations were restored') `
        ('The installer did not report a completed automatic rollback. Actual: ' + $caughtText)
    Assert-Equal (Get-TreeFingerprint $fixture.Mods) $beforeMods `
        'Late-failure rollback did not restore the entire Mods tree.'
    $backupRoots = @(Get-ChildItem -LiteralPath $fixture.Win64 -Directory -Filter 'UE4SS-*-Backup' |
        Sort-Object LastWriteTimeUtc -Descending)
    Assert-Equal $backupRoots.Count 1 'Late-failure backup directory count is not exact.'
    $backupRoot = $backupRoots[0].FullName
    $rollbackLogs = @([System.IO.Directory]::GetFiles(
        $backupRoot,
        'ROLLBACK-LOG.txt',
        [System.IO.SearchOption]::AllDirectories))
    Assert-Equal $rollbackLogs.Count 1 'Late-failure rollback record count is not exact.'
    Assert-Equal (Read-Utf8Text $rollbackLogs[0]).Trim() `
        'Automatic rollback completed.' 'Rollback record did not report success.'
    $installLogs = @([System.IO.Directory]::GetFiles(
        $backupRoot,
        'INSTALL-LOG.txt',
        [System.IO.SearchOption]::AllDirectories))
    Assert-Equal $installLogs.Count 0 'A failed transaction incorrectly wrote an install log.'
}

if ($script:CaseNumber -ne $expectedTestCount -or $script:Results.Count -ne $expectedTestCount) {
    throw "Installer test harness defect: expected $expectedTestCount cases, defined $($script:CaseNumber)."
}

$sourceStateError = $null
try {
    Assert-SourceStateEqual $sourceStates[0] (Get-SourceState $script:ResolvedGame) `
        'Supplied game executable'
    Assert-SourceStateEqual $sourceStates[1] (Get-SourceState $script:ResolvedUE4SS) `
        'Supplied UE4SS DLL'
    Assert-SourceStateEqual $sourceStates[2] (Get-SourceState $script:ResolvedDwmapi) `
        'Supplied dwmapi proxy'
} catch {
    $sourceStateError = Get-ExceptionText $_.Exception
}

$runRootClean = $false
try {
    Remove-FixtureTree -Path $runRoot
    $runRootClean = -not (Test-Path -LiteralPath $runRoot)
} catch {
    $runRootClean = $false
}

$passed = @($script:Results | Where-Object { $_.status -eq 'PASSED' }).Count
$failed = @($script:Results | Where-Object { $_.status -eq 'FAILED' }).Count
$skipped = 0
$releaseGate = if ($passed -eq $expectedTestCount -and
    $failed -eq 0 -and $skipped -eq 0 -and
    $null -eq $sourceStateError -and $runRootClean) { 'PASSED' } else { 'FAILED' }

Write-Host ("Installer integration result: {0} passed, {1} failed, {2} skipped." -f `
    $passed, $failed, $skipped)

[pscustomobject]@{
    expected = $expectedTestCount
    passed = $passed
    failed = $failed
    skipped = $skipped
    release_gate = $releaseGate
    sources_unchanged = ($null -eq $sourceStateError)
    source_state_error = $sourceStateError
    fixtures_cleaned = $runRootClean
    working_directory = $workingRoot
    installer = $script:ResolvedInstaller
    details = @($script:Results)
}
