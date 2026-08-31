[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InstallerExe,
    [Parameter(Mandatory = $true)][string]$ManualArchive,
    [Parameter(Mandatory = $true)][string]$SupportedGameExecutable,
    [Parameter(Mandatory = $true)][string]$ExperimentalUE4SSDll,
    [Parameter(Mandatory = $true)][string]$ExperimentalDwmapiDll,
    [Parameter(Mandatory = $true)][string]$WorkingDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$expectedUE4SSHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$expectedDwmapiHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$productName = 'DragonSwordNativeWorldRadarPostRender'
$externalLegacyName = 'DragonSwordWorldRadar'
$expectedTestCount = 1

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
    foreach ($property in @(
            'path', 'length', 'sha256', 'last_write_utc_ticks', 'attributes')) {
        if (-not [object]::Equals($Before.$property, $After.$property)) {
            throw "$Description changed at property $property."
        }
    }
}

function Write-Utf8Text {
    param([string]$Path, [string]$Text)
    $parent = [System.IO.Path]::GetDirectoryName(
        [System.IO.Path]::GetFullPath($Path))
    [void][System.IO.Directory]::CreateDirectory($parent)
    [System.IO.File]::WriteAllText(
        $Path,
        $Text,
        [System.Text.UTF8Encoding]::new($false))
}

function Copy-FixtureFile {
    param([string]$Source, [string]$Destination)
    $parent = [System.IO.Path]::GetDirectoryName(
        [System.IO.Path]::GetFullPath($Destination))
    [void][System.IO.Directory]::CreateDirectory($parent)
    [System.IO.File]::Copy($Source, $Destination, $false)
    [System.IO.File]::SetAttributes(
        $Destination,
        [System.IO.FileAttributes]::Normal)
}

function Assert-SafeRelativePath {
    param([string]$Path, [string]$Description)
    $normalized = $Path.Replace('\', '/')
    $segments = @($normalized -split '/')
    Assert-True (-not [string]::IsNullOrWhiteSpace($normalized) -and
        -not [System.IO.Path]::IsPathRooted($normalized) -and
        -not $normalized.Contains(':') -and
        @($segments | Where-Object {
                [string]::IsNullOrWhiteSpace($_) -or
                $_ -eq '.' -or $_ -eq '..'
            }).Count -eq 0) "$Description is unsafe: $Path"
    return $normalized
}

function Remove-DisposableTree {
    param([string]$Path, [string]$ExpectedParent)
    $full = [System.IO.Path]::GetFullPath($Path)
    $parent = [System.IO.Path]::GetFullPath($ExpectedParent).TrimEnd('\')
    Assert-True ([string]::Equals(
            [System.IO.Path]::GetDirectoryName($full),
            $parent,
            [System.StringComparison]::OrdinalIgnoreCase)) `
        'Disposable test root escaped its exact working directory.'
    if (-not (Test-Path -LiteralPath $full)) { return }
    $root = Get-Item -LiteralPath $full -Force
    Assert-True ($root.PSIsContainer -and
        ($root.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0) `
        'Disposable test root is not a plain directory.'
    $reparse = @(Get-ChildItem -LiteralPath $full -Recurse -Force |
        Where-Object {
            ($_.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0
        })
    Assert-Equal $reparse.Count 0 `
        'Disposable test root contains a reparse point.'
    Remove-Item -LiteralPath $full -Recurse -Force
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
        -Text "[Overrides]`r`n"
    $modsTxt = Join-Path $mods 'mods.txt'
    Write-Utf8Text -Path $modsTxt `
        -Text "OtherMod : 1`r`n$externalLegacyName : 0`r`n"
    return [pscustomobject]@{
        Root = $Root
        Win64 = $win64
        Mods = $mods
        ModsTxt = $modsTxt
        GameExe = $game
        Target = Join-Path $mods $productName
    }
}

function Count-ModEntry {
    param([string]$Text, [string]$Name, [bool]$Enabled)
    $state = if ($Enabled) { '1' } else { '0' }
    $pattern = '(?m)^[\t ]*' + [regex]::Escape($Name) +
        '[\t ]*:[\t ]*' + $state + '[\t ]*(?=\r?$)'
    return [regex]::Matches(
        $Text,
        $pattern,
        [System.Text.RegularExpressions.RegexOptions]::CultureInvariant).Count
}

function Assert-ModAuthority {
    param([string]$ModsTxt)
    $text = [System.IO.File]::ReadAllText(
        $ModsTxt,
        [System.Text.UTF8Encoding]::new($false, $true))
    Assert-Equal (Count-ModEntry $text $productName $true) 1 `
        'Native Radar does not have exactly one enabled mods.txt authority.'
    Assert-Equal (Count-ModEntry $text $externalLegacyName $false) 1 `
        'The unrelated disabled external Radar authority was not preserved.'
    Assert-Equal (Count-ModEntry $text 'OtherMod' $true) 1 `
        'The unrelated enabled mod authority was not preserved.'
}

function Get-RelativeFileMap {
    param([string]$Root, [string[]]$Exclude = @())
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    $map = [System.Collections.Generic.Dictionary[string, object]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($file in @(Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File)) {
        Assert-True (($file.Attributes -band
                [System.IO.FileAttributes]::ReparsePoint) -eq 0) `
            "Installed payload contains a reparse point: $($file.FullName)"
        $relative = $file.FullName.Substring($rootFull.Length).TrimStart('\').Replace('\', '/')
        if ($Exclude -contains $relative) { continue }
        Assert-True (-not $map.ContainsKey($relative)) `
            "Installed payload repeats a path: $relative"
        $map.Add($relative, [pscustomobject]@{
                size = [long]$file.Length
                sha256 = Get-Sha256 $file.FullName
            })
    }
    return $map
}

function Assert-ExactPayloadEquivalence {
    param([string]$SetupRoot, [string]$ManualRoot)
    $setup = Get-RelativeFileMap -Root $SetupRoot `
        -Exclude @('INSTALL-RECORD.txt')
    $manual = Get-RelativeFileMap -Root $ManualRoot
    $setupPaths = @($setup.Keys | Sort-Object)
    $manualPaths = @($manual.Keys | Sort-Object)
    Assert-Equal ($setupPaths -join '|') ($manualPaths -join '|') `
        'Setup and manual installed payloads have different shared file sets.'
    foreach ($path in $setupPaths) {
        Assert-Equal ([long]$setup[$path].size) ([long]$manual[$path].size) `
            "Setup and manual installed sizes differ for $path."
        Assert-Equal ([string]$setup[$path].sha256) ([string]$manual[$path].sha256) `
            "Setup and manual installed hashes differ for $path."
    }
    Assert-True (Test-Path -LiteralPath `
            (Join-Path $SetupRoot 'INSTALL-RECORD.txt') -PathType Leaf) `
        'Setup-specific install record is missing.'
    Assert-True (-not (Test-Path -LiteralPath `
            (Join-Path $ManualRoot 'INSTALL-RECORD.txt'))) `
        'The manual product unexpectedly contains a Setup install record.'
    return $setup.Count
}

function Test-ManualChecksumList {
    param([string]$Root)
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    $checksumPath = Join-Path $rootFull 'SHA256SUMS.txt'
    $bytes = [System.IO.File]::ReadAllBytes($checksumPath)
    Assert-True (-not ($bytes.Length -ge 3 -and
            $bytes[0] -eq 0xEF -and
            $bytes[1] -eq 0xBB -and
            $bytes[2] -eq 0xBF)) `
        'Manual checksum list contains a UTF-8 BOM.'
    $text = [System.Text.UTF8Encoding]::new($false, $true).GetString($bytes)
    $declared = [System.Collections.Generic.Dictionary[string, string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($line in @($text.Split(@("`r`n", "`n"),
                [System.StringSplitOptions]::RemoveEmptyEntries))) {
        $match = [regex]::Match($line, '^([0-9A-F]{64})  (.+)$')
        Assert-True $match.Success "Malformed SHA256SUMS line: $line"
        $relative = Assert-SafeRelativePath $match.Groups[2].Value `
            'Manual checksum path'
        Assert-True (-not $declared.ContainsKey($relative)) `
            "Manual checksum list repeats a path: $relative"
        $declared.Add($relative, $match.Groups[1].Value.ToUpperInvariant())
    }
    $actual = @(Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File |
        Where-Object { $_.FullName -ne $checksumPath })
    Assert-Equal $declared.Count $actual.Count `
        'Manual checksum list does not cover the exact extracted file count.'
    foreach ($file in $actual) {
        $relative = $file.FullName.Substring($rootFull.Length).TrimStart('\').Replace('\', '/')
        Assert-True ($declared.ContainsKey($relative)) `
            "Manual checksum list omits $relative."
        Assert-Equal (Get-Sha256 $file.FullName) $declared[$relative] `
            "Manual checksum differs for $relative."
    }
}

$script:ResolvedInstaller = Resolve-RequiredLeaf $InstallerExe 'Installer executable'
$script:ResolvedManualArchive = Resolve-RequiredLeaf $ManualArchive 'Manual archive'
$script:ResolvedGame = Resolve-RequiredLeaf $SupportedGameExecutable `
    'Supported game executable'
$script:ResolvedUE4SS = Resolve-RequiredLeaf $ExperimentalUE4SSDll `
    'Experimental UE4SS DLL'
$script:ResolvedDwmapi = Resolve-RequiredLeaf $ExperimentalDwmapiDll `
    'Experimental dwmapi proxy'

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
    Get-SourceState $script:ResolvedInstaller
    Get-SourceState $script:ResolvedManualArchive
    Get-SourceState $script:ResolvedGame
    Get-SourceState $script:ResolvedUE4SS
    Get-SourceState $script:ResolvedDwmapi
)

if ([string]::IsNullOrWhiteSpace($WorkingDirectory) -or
    -not (Test-Path -LiteralPath $WorkingDirectory -PathType Container)) {
    throw "WorkingDirectory must be a caller-created directory: $WorkingDirectory"
}
$workingRoot = [System.IO.Path]::GetFullPath(
    (Resolve-Path -LiteralPath $WorkingDirectory).ProviderPath).TrimEnd('\')
$workingItem = Get-Item -LiteralPath $workingRoot -Force
Assert-True (($workingItem.Attributes -band
        [System.IO.FileAttributes]::ReparsePoint) -eq 0) `
    'WorkingDirectory must not be a reparse point.'
$runRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $workingRoot ('DSNWR-ManualTests-' + [Guid]::NewGuid().ToString('N'))))
Assert-True ($runRoot.StartsWith(
        $workingRoot + '\',
        [System.StringComparison]::OrdinalIgnoreCase)) `
    'Resolved manual test root escaped WorkingDirectory.'
foreach ($source in @(
        $script:ResolvedInstaller,
        $script:ResolvedManualArchive,
        $script:ResolvedGame,
        $script:ResolvedUE4SS,
        $script:ResolvedDwmapi)) {
    Assert-True (-not [System.IO.Path]::GetFullPath($source).StartsWith(
            $runRoot + '\',
            [System.StringComparison]::OrdinalIgnoreCase)) `
        'A test source must not be inside the disposable manual test root.'
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$installerAssembly = [System.Reflection.Assembly]::LoadFile($script:ResolvedInstaller)
$engineType = $installerAssembly.GetType(
    'DragonSwordNativeWorldRadarPostRender.Installer.InstallerEngine',
    $true,
    $false)
$installMethod = $engineType.GetMethod(
    'Install',
    [System.Reflection.BindingFlags]'Static, NonPublic')
Assert-True ($null -ne $installMethod -and
    $installMethod.GetParameters().Count -eq 1 -and
    $installMethod.GetParameters()[0].ParameterType -eq [string]) `
    'Internal InstallerEngine.Install(string) was not found.'

$results = [System.Collections.Generic.List[object]]::new()
$caseFailure = $null
$equivalentFileCount = 0
[void][System.IO.Directory]::CreateDirectory($runRoot)
try {
    $extractRoot = Join-Path $runRoot 'manual-package'
    [void][System.IO.Directory]::CreateDirectory($extractRoot)
    $archive = [System.IO.Compression.ZipFile]::OpenRead($script:ResolvedManualArchive)
    try {
        $seen = [System.Collections.Generic.HashSet[string]]::new(
            [System.StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in $archive.Entries) {
            $identity = (Assert-SafeRelativePath `
                    $entry.FullName.TrimEnd([char[]]@('/', '\')) `
                    'Manual archive entry')
            Assert-True ($seen.Add($identity)) `
                "Manual archive repeats a path: $identity"
        }
    } finally {
        $archive.Dispose()
    }
    [System.IO.Compression.ZipFile]::ExtractToDirectory(
        $script:ResolvedManualArchive,
        $extractRoot)
    $rootEntries = @(Get-ChildItem -LiteralPath $extractRoot -Force |
        Select-Object -ExpandProperty Name | Sort-Object)
    $expectedRootEntries = @(
        $productName,
        'README.md',
        'SHA256SUMS.txt',
        'THIRD_PARTY_NOTICES.txt') | Sort-Object
    Assert-Equal ($rootEntries -join '|') ($expectedRootEntries -join '|') `
        'Manual archive root allowlist differs.'
    Test-ManualChecksumList -Root $extractRoot

    $manualSource = Join-Path $extractRoot $productName
    $releaseMetadata = Get-Content -LiteralPath `
        (Join-Path $manualSource 'metadata\release.json') -Raw | ConvertFrom-Json
    $packageMetadata = Get-Content -LiteralPath `
        (Join-Path $manualSource 'metadata\package-manifest.json') -Raw |
        ConvertFrom-Json
    Assert-Equal ([string]$releaseMetadata.name) $productName `
        'Manual release identity differs.'
    Assert-Equal ([string]$packageMetadata.name) $productName `
        'Manual package identity differs.'
    Assert-True (-not (Test-Path -LiteralPath `
            (Join-Path $manualSource 'enabled.txt'))) `
        'Manual archive contains legacy enabled.txt authority.'

    $setupFixture = New-ValidFixture (Join-Path $runRoot 'setup-route')
    $manualFixture = New-ValidFixture (Join-Path $runRoot 'manual-route')
    [void]$installMethod.Invoke(
        $null,
        [object[]]@([string]$setupFixture.GameExe))

    Copy-Item -LiteralPath $manualSource -Destination $manualFixture.Target `
        -Recurse -Force
    $manualModsText = [System.IO.File]::ReadAllText(
        $manualFixture.ModsTxt,
        [System.Text.UTF8Encoding]::new($false, $true))
    Write-Utf8Text -Path $manualFixture.ModsTxt `
        -Text ($manualModsText + "$productName : 1`r`n")

    Assert-ModAuthority -ModsTxt $setupFixture.ModsTxt
    Assert-ModAuthority -ModsTxt $manualFixture.ModsTxt
    $equivalentFileCount = Assert-ExactPayloadEquivalence `
        -SetupRoot $setupFixture.Target `
        -ManualRoot $manualFixture.Target
    Assert-True ($equivalentFileCount -gt 0) `
        'The installed payload equivalence file set is empty.'
    foreach ($fixture in @($setupFixture, $manualFixture)) {
        Assert-True (Test-Path -LiteralPath `
                (Join-Path $fixture.Target 'config\visibility.ini') -PathType Leaf) `
            'Installed visibility.ini is missing.'
        Assert-True (Test-Path -LiteralPath `
                (Join-Path $fixture.Target 'config\diagnostics.ini') -PathType Leaf) `
            'Installed diagnostics.ini is missing.'
        Assert-True (-not (Test-Path -LiteralPath `
                (Join-Path $fixture.Target 'config\visibility.example.ini'))) `
            'Installer-only visibility default leaked into an installed route.'
        Assert-True (-not (Test-Path -LiteralPath `
                (Join-Path $fixture.Target 'config\diagnostics.example.ini'))) `
            'Installer-only diagnostics default leaked into an installed route.'
    }
} catch {
    $caseFailure = $_.Exception.ToString()
}

if ($null -eq $caseFailure) {
    $results.Add([pscustomobject]@{
            name = 'Manual route matches Setup installed runtime exactly'
            status = 'PASSED'
            error = $null
        })
} else {
    $results.Add([pscustomobject]@{
            name = 'Manual route matches Setup installed runtime exactly'
            status = 'FAILED'
            error = $caseFailure
        })
}

$sourceStateError = $null
try {
    $descriptions = @(
        'Installer executable',
        'Manual archive',
        'Supplied game executable',
        'Supplied UE4SS DLL',
        'Supplied dwmapi proxy')
    $paths = @(
        $script:ResolvedInstaller,
        $script:ResolvedManualArchive,
        $script:ResolvedGame,
        $script:ResolvedUE4SS,
        $script:ResolvedDwmapi)
    for ($index = 0; $index -lt $paths.Count; ++$index) {
        Assert-SourceStateEqual $sourceStates[$index] `
            (Get-SourceState $paths[$index]) $descriptions[$index]
    }
} catch {
    $sourceStateError = $_.Exception.ToString()
}

$runRootClean = $false
try {
    Remove-DisposableTree -Path $runRoot -ExpectedParent $workingRoot
    $runRootClean = -not (Test-Path -LiteralPath $runRoot)
} catch {
    $runRootClean = $false
}

$passed = @($results | Where-Object status -eq 'PASSED').Count
$failed = @($results | Where-Object status -eq 'FAILED').Count
$skipped = 0
$releaseGate = if ($passed -eq $expectedTestCount -and
    $failed -eq 0 -and $skipped -eq 0 -and
    $null -eq $sourceStateError -and $runRootClean) { 'PASSED' } else { 'FAILED' }
$equivalence = if ($releaseGate -eq 'PASSED') {
    'PASSED_EXACT_SHARED_FILE_SET_HASHES'
} else {
    'FAILED'
}

Write-Host ("Manual install integration result: {0} passed, {1} failed, {2} skipped." -f `
    $passed, $failed, $skipped)

[pscustomobject]@{
    expected = $expectedTestCount
    passed = $passed
    failed = $failed
    skipped = $skipped
    release_gate = $releaseGate
    runtime_payload_equivalence = $equivalence
    equivalent_file_count = [int]$equivalentFileCount
    sources_unchanged = ($null -eq $sourceStateError)
    source_state_error = $sourceStateError
    fixtures_cleaned = $runRootClean
    working_directory = $workingRoot
    installer = $script:ResolvedInstaller
    manual_archive = $script:ResolvedManualArchive
    details = @($results)
}
