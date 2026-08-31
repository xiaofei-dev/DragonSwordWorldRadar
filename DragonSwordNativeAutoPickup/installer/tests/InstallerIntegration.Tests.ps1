[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerExe,
    [Parameter(Mandatory)][string]$SupportedGameExecutable,
    [Parameter(Mandatory)][string]$ExperimentalUE4SSDll,
    [string]$ExperimentalDwmapiDll,
    [string]$WorkingDirectory,
    [switch]$KeepArtifacts
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$expectedStableUE4SSHash = '8AC18FBFFC1EF96B0662D4A2D537B3F224C26D65CAABA7989A9404C566102B26'
$expectedExperimentalUE4SSHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$expectedStableDwmapiHash = 'CE596412BEFA68C30B7F88F65BEB77D9BDAD55E9B96A276A5A9CF690C63F24BB'
$expectedExperimentalDwmapiHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$expectedRangeX3PakHash = '6BB99A1E35C06EB0284370B9D7BD2F34E90CB6DCA7479CF10A477C68EA0103E8'
$expectedRangeX5PakHash = 'DB9E129D8F8FCCA025864EC908C13C70F950AD779C37CF13A41164476587CECD'
$expectedRangeX10PakHash = '6A1ADB7592BA0C70A17984DB3AC01348086AABE196F0FDAF914B3F52C7A395F1'
$gameFileName = 'DSClient-Win64-Shipping.exe'
$modName = 'DragonSwordNativeAutoPickup'
$rangeX3PakName = 'DS_PickupRangeX3_P.pak'
$rangeX5PakName = 'DS_PickupRangeX5_P.pak'
$rangeX10PakName = 'DS_PickupRangeX10_P.pak'

function Resolve-RequiredFile {
    param([string]$Path, [string]$Description)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-Sha256 {
    param([Parameter(Mandatory)][string]$Path)

    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToUpperInvariant()
}

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param($Expected, $Actual, [string]$Message)

    if (-not [object]::Equals($Expected, $Actual)) {
        throw "$Message Expected=[$Expected] Actual=[$Actual]"
    }
}

function Assert-FileHash {
    param([string]$Path, [string]$ExpectedHash, [string]$Message)

    Assert-True (Test-Path -LiteralPath $Path -PathType Leaf) "$Message Missing file: $Path"
    Assert-Equal $ExpectedHash (Get-Sha256 $Path) $Message
}

function Get-ResourceBytes {
    param([string]$Name)

    $stream = $script:installerAssembly.GetManifestResourceStream($Name)
    if ($null -eq $stream) {
        throw "The built installer does not contain resource: $Name"
    }
    try {
        $memory = New-Object System.IO.MemoryStream
        try {
            $stream.CopyTo($memory)
            return $memory.ToArray()
        }
        finally {
            $memory.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
}

function Expand-EmbeddedStableUE4SS {
    param([string]$Destination)

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    $zipBytes = Get-ResourceBytes 'Payload.UE4SS.zip'
    $memory = New-Object System.IO.MemoryStream -ArgumentList @(,$zipBytes)
    try {
        $archive = New-Object System.IO.Compression.ZipArchive(
            $memory,
            [System.IO.Compression.ZipArchiveMode]::Read,
            $false)
        try {
            $root = [IO.Path]::GetFullPath($Destination)
            $prefix = $root.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
            foreach ($entry in $archive.Entries) {
                if ([string]::IsNullOrEmpty($entry.Name)) {
                    continue
                }
                $relative = $entry.FullName.Replace('/', [IO.Path]::DirectorySeparatorChar)
                $target = [IO.Path]::GetFullPath((Join-Path $root $relative))
                if (-not $target.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
                    throw "Unsafe path in embedded UE4SS archive: $($entry.FullName)"
                }
                New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
                $input = $entry.Open()
                try {
                    $output = [IO.File]::Create($target)
                    try { $input.CopyTo($output) }
                    finally { $output.Dispose() }
                }
                finally { $input.Dispose() }
            }
        }
        finally { $archive.Dispose() }
    }
    finally { $memory.Dispose() }
}

function Get-ZipEntryBytes {
    param([string]$EntryName)

    $zipBytes = Get-ResourceBytes 'Payload.UE4SS.zip'
    $memory = New-Object System.IO.MemoryStream -ArgumentList @(,$zipBytes)
    try {
        $archive = New-Object System.IO.Compression.ZipArchive(
            $memory,
            [System.IO.Compression.ZipArchiveMode]::Read,
            $false)
        try {
            $entry = $archive.GetEntry($EntryName)
            if ($null -eq $entry) { throw "Embedded UE4SS entry is missing: $EntryName" }
            $input = $entry.Open()
            try {
                $output = New-Object System.IO.MemoryStream
                try {
                    $input.CopyTo($output)
                    return $output.ToArray()
                }
                finally { $output.Dispose() }
            }
            finally { $input.Dispose() }
        }
        finally { $archive.Dispose() }
    }
    finally { $memory.Dispose() }
}

function New-GameTree {
    param([string]$Name)

    $root = Join-Path $script:runRoot $Name
    $win64 = Join-Path $root 'DS\Binaries\Win64'
    New-Item -ItemType Directory -Path $win64 -Force | Out-Null
    $gameExecutable = Join-Path $win64 $gameFileName
    Copy-Item -LiteralPath $script:supportedGameExecutable -Destination $gameExecutable
    return [pscustomobject]@{
        Root = $root
        Win64 = $win64
        GameExecutable = $gameExecutable
        PakDirectory = Join-Path $root 'DS\Content\Paks\~mods'
    }
}

function Initialize-StableLayout {
    param($Tree)

    Expand-EmbeddedStableUE4SS $Tree.Win64
    Assert-FileHash (Join-Path $Tree.Win64 'UE4SS.dll') $expectedStableUE4SSHash `
        'Stable fixture does not contain the approved UE4SS DLL.'
    Assert-FileHash (Join-Path $Tree.Win64 'dwmapi.dll') $expectedStableDwmapiHash `
        'Stable fixture does not contain the approved proxy DLL.'
}

function Initialize-ExperimentalLayout {
    param(
        $Tree,
        [bool]$CreateNestedMods = $true,
        [bool]$InstallProxy = $true
    )

    $ue4ss = Join-Path $Tree.Win64 'ue4ss'
    New-Item -ItemType Directory -Path $ue4ss -Force | Out-Null
    if ($CreateNestedMods) {
        New-Item -ItemType Directory -Path (Join-Path $ue4ss 'Mods') -Force | Out-Null
    }
    Copy-Item -LiteralPath $script:experimentalUE4SSDll -Destination (Join-Path $ue4ss 'UE4SS.dll')
    if ($InstallProxy) {
        Copy-Item -LiteralPath $script:experimentalDwmapiDll -Destination (Join-Path $Tree.Win64 'dwmapi.dll')
    }
    [IO.File]::WriteAllBytes(
        (Join-Path $ue4ss 'UE4SS-settings.ini'),
        (Get-ZipEntryBytes 'UE4SS-settings.ini'))
}

function Get-ReflectedProperty {
    param($Instance, [string]$Name)

    $flags = [Reflection.BindingFlags]'Instance,Public,NonPublic'
    $property = $Instance.GetType().GetProperty($Name, $flags)
    if ($null -eq $property) { throw "Installer result property was not found: $Name" }
    return $property.GetValue($Instance, $null)
}

function Invoke-Installer {
    param(
        [string]$GameExecutable,
        [string]$Hotkey,
        [object]$RangeChoice,
        [string]$InteractionKeyFallback = 'F'
    )

    $multiplier = if ($RangeChoice -is [bool]) {
        if ([bool]$RangeChoice) { 5 } else { 0 }
    }
    else {
        [int]$RangeChoice
    }
    $rangeSelection = [Enum]::ToObject($script:rangeSelectionType, $multiplier)

    try {
        $result = $script:installMethod.Invoke(
            $null,
            [object[]]@($GameExecutable, $Hotkey, $InteractionKeyFallback, $rangeSelection))
    }
    catch [Reflection.TargetInvocationException] {
        if ($null -ne $_.Exception.InnerException) {
            throw $_.Exception.InnerException
        }
        throw
    }
    $installedMultiplier = [int](Get-ReflectedProperty $result 'RangeMultiplierInstalled')
    return [pscustomobject]@{
        LayoutDescription = [string](Get-ReflectedProperty $result 'LayoutDescription')
        ToggleHotkey = [string](Get-ReflectedProperty $result 'ToggleHotkey')
        InteractionKeyFallback = [string](Get-ReflectedProperty $result 'InteractionKeyFallback')
        RangeMultiplierInstalled = $installedMultiplier
        RangePakInstalled = $installedMultiplier -ne 0
        BackupDirectory = [string](Get-ReflectedProperty $result 'BackupDirectory')
    }
}

function Invoke-InstallerFailure {
    param(
        [string]$GameExecutable,
        [string]$Hotkey,
        [object]$RangeChoice,
        [string]$ExpectedMessage,
        [string]$InteractionKeyFallback = 'F'
    )

    try {
        Invoke-Installer $GameExecutable $Hotkey $RangeChoice $InteractionKeyFallback | Out-Null
    }
    catch {
        $message = $_.Exception.Message
        if (-not $message.Contains($ExpectedMessage)) {
            throw "Installer failed for the wrong reason. Expected=[$ExpectedMessage] Actual=[$message]"
        }
        return $message
    }
    throw "Installer unexpectedly succeeded. Expected failure containing: $ExpectedMessage"
}

function Get-FileSnapshot {
    param([string]$Root, [string[]]$ExcludedDirectoryNames = @())

    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $rows = foreach ($file in Get-ChildItem -LiteralPath $rootPath -Recurse -Force -File | Sort-Object FullName) {
        $relative = $file.FullName.Substring($rootPath.Length).TrimStart([IO.Path]::DirectorySeparatorChar)
        $segments = $relative.Split([IO.Path]::DirectorySeparatorChar)
        if (@($segments | Where-Object { $ExcludedDirectoryNames -contains $_ }).Count -ne 0) { continue }
        "{0}|{1}|{2}" -f $relative, $file.Length, (Get-Sha256 $file.FullName)
    }
    return [string]::Join("`n", [string[]]$rows)
}

function Assert-PublicConfig {
    param(
        [string]$ModDirectory,
        [string]$Hotkey,
        [string]$InteractionKeyFallback = 'F'
    )

    $configPath = Join-Path $ModDirectory 'config.ini'
    Assert-True (Test-Path -LiteralPath $configPath -PathType Leaf) `
        'Public config was not installed at the Mod root.'
    Assert-True (-not (Test-Path -LiteralPath (Join-Path $ModDirectory 'config\config.ini'))) `
        'A stale nested config/config.ini was installed.'
    $text = [IO.File]::ReadAllText($configPath, [Text.UTF8Encoding]::new($false, $true))
    foreach ($pattern in @(
        '(?m)^enabled_on_launch=false\r?$',
        '(?m)^automatic_pickup=true\r?$',
        ('(?m)^toggle_hotkey=' + [regex]::Escape($Hotkey) + '\r?$'),
        '(?m)^interaction_key=AUTO\r?$',
        ('(?m)^interaction_key_fallback=' + [regex]::Escape($InteractionKeyFallback) + '\r?$'),
        '(?m)^debug_logging=false\r?$')) {
        Assert-Equal 1 ([regex]::Matches($text, $pattern).Count) `
            "Public config setting did not verify: $pattern"
    }
}

function Assert-ModsTxtControlled {
    param(
        [string]$Path,
        [bool]$ExpectedBom = $false,
        [bool]$ExpectedLfOnly = $false,
        [string[]]$RequiredText = @()
    )

    Assert-True (Test-Path -LiteralPath $Path -PathType Leaf) 'Controlling mods.txt is missing.'
    $bytes = [IO.File]::ReadAllBytes($Path)
    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    Assert-Equal $ExpectedBom $hasBom 'mods.txt UTF-8 BOM preservation failed.'
    $offset = if ($hasBom) { 3 } else { 0 }
    $text = [Text.UTF8Encoding]::new($false, $true).GetString($bytes, $offset, $bytes.Length - $offset)
    if ($ExpectedLfOnly) {
        Assert-True (-not $text.Contains("`r")) 'mods.txt LF newline preservation failed.'
    }
    foreach ($required in $RequiredText) {
        Assert-True $text.Contains($required) "mods.txt lost unrelated text: $required"
    }
    $entries = [regex]::Matches(
        $text,
        '(?m)^[\t ]*DragonSwordNativeAutoPickup[\t ]*:[^\r\n]*(?=\r?$)')
    Assert-Equal 1 $entries.Count 'mods.txt does not contain exactly one Auto Pickup entry.'
    Assert-Equal 'DragonSwordNativeAutoPickup : 1' $entries[0].Value.Trim() `
        'mods.txt Auto Pickup entry is not enabled and normalized.'
}

function Invoke-TestCase {
    param([string]$Name, [scriptblock]$Body)

    try {
        & $Body
        $script:passed += 1
        Write-Host "PASS  $Name"
    }
    catch {
        if ($_.Exception.Message.StartsWith('__DSNAP_SKIP__:', [StringComparison]::Ordinal)) {
            $script:skipped += 1
            Write-Host "SKIP  $Name"
            Write-Host ("      " + $_.Exception.Message.Substring('__DSNAP_SKIP__:'.Length))
            return
        }
        $script:failed += 1
        $script:failures.Add("$Name`: $($_.Exception.Message)")
        Write-Host "FAIL  $Name" -ForegroundColor Red
        Write-Host "      $($_.Exception.Message)" -ForegroundColor Red
    }
}

function Skip-Test {
    param([string]$Reason)

    throw "__DSNAP_SKIP__:$Reason"
}

$script:installerExe = Resolve-RequiredFile $InstallerExe 'Built installer EXE'
$script:supportedGameExecutable = Resolve-RequiredFile $SupportedGameExecutable 'Supported game executable'
$script:experimentalUE4SSDll = Resolve-RequiredFile $ExperimentalUE4SSDll 'Approved experimental UE4SS DLL'
$experimentalProxyCandidate = $ExperimentalDwmapiDll
if ([string]::IsNullOrWhiteSpace($experimentalProxyCandidate)) {
    $experimentalUE4SSDirectory = Split-Path -Parent $script:experimentalUE4SSDll
    if ([string]::Equals(
            [IO.Path]::GetFileName($experimentalUE4SSDirectory),
            'ue4ss',
            [StringComparison]::OrdinalIgnoreCase)) {
        $experimentalProxyCandidate = Join-Path (Split-Path -Parent $experimentalUE4SSDirectory) 'dwmapi.dll'
    }
}
$script:experimentalDwmapiDll = Resolve-RequiredFile $experimentalProxyCandidate 'Approved experimental proxy DLL'

Assert-Equal $gameFileName ([IO.Path]::GetFileName($script:supportedGameExecutable)) `
    "The supported game input must be named $gameFileName."
Assert-Equal $expectedExperimentalUE4SSHash (Get-Sha256 $script:experimentalUE4SSDll) `
    'The experimental UE4SS fixture is not the exact approved binary.'
Assert-Equal $expectedExperimentalDwmapiHash (Get-Sha256 $script:experimentalDwmapiDll) `
    'The experimental proxy fixture is not the exact approved binary.'

Add-Type -AssemblyName System.IO.Compression
$script:installerAssembly = [Reflection.Assembly]::LoadFile($script:installerExe)
$engineType = $script:installerAssembly.GetType(
    'DragonSwordNativeAutoPickup.Installer.InstallerEngine',
    $true,
    $false)
$script:installMethod = $engineType.GetMethod(
    'Install',
    [Reflection.BindingFlags]'Static,NonPublic')
if ($null -eq $script:installMethod) {
    throw 'The built installer does not expose the expected internal InstallerEngine.Install method.'
}
$script:rangeSelectionType = $script:installerAssembly.GetType(
    'DragonSwordNativeAutoPickup.Installer.InstallerEngine+RangeSelection',
    $true,
    $false)

$resourceNames = $script:installerAssembly.GetManifestResourceNames()
foreach ($resourceName in @(
    'Payload.Manifest.ini',
    'Payload.StablePlugin.dll',
    'Payload.ExperimentalPlugin.dll',
    'Payload.DefaultConfig.ini',
    'Payload.Main.lua',
    'Payload.UE4SS.zip',
    'Payload.PickupRangeX3.pak',
    'Payload.PickupRangeX5.pak',
    'Payload.PickupRangeX10.pak',
    'Payload.ThirdPartyNotices.txt')) {
    Assert-True ($resourceNames -contains $resourceName) "Missing installer resource: $resourceName"
}

$workBase = if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
    [IO.Path]::GetTempPath()
}
else {
    [IO.Path]::GetFullPath($WorkingDirectory)
}
New-Item -ItemType Directory -Path $workBase -Force | Out-Null
$script:runRoot = Join-Path $workBase ('dsnap-installer-tests-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $script:runRoot | Out-Null
$script:passed = 0
$script:failed = 0
$script:skipped = 0
$script:failures = New-Object 'Collections.Generic.List[string]'

try {
    Invoke-TestCase 'Bootstrap official stable UE4SS into a bare game tree' {
        $tree = New-GameTree 'bootstrap-stable'
        $result = Invoke-Installer $tree.GameExecutable 'F10' $false 'k'
        Assert-Equal 'stable-root UE4SS ABI' $result.LayoutDescription 'Wrong bootstrap layout.'
        Assert-Equal 'F10' $result.ToggleHotkey 'Custom bootstrap hotkey was not returned.'
        Assert-Equal 'K' $result.InteractionKeyFallback 'Custom interaction fallback was not normalized and returned.'
        Assert-True (-not $result.RangePakInstalled) 'Bootstrap unexpectedly installed the optional PAK.'
        Assert-FileHash (Join-Path $tree.Win64 'UE4SS.dll') $expectedStableUE4SSHash `
            'Bootstrap installed the wrong UE4SS DLL.'
        Assert-FileHash (Join-Path $tree.Win64 'dwmapi.dll') $expectedStableDwmapiHash `
            'Bootstrap did not install the exact approved stable proxy loader.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'ue4ss\UE4SS.dll'))) `
            'Bootstrap incorrectly created the experimental nested layout.'
        $modDirectory = Join-Path $tree.Win64 "Mods\$modName"
        Assert-PublicConfig $modDirectory 'F10' 'K'
        # The pinned official UE4SS archive ships its controlling mods.txt with
        # a UTF-8 BOM. Bootstrap must preserve that existing encoding choice.
        Assert-ModsTxtControlled (Join-Path $tree.Win64 'Mods\mods.txt') $true
        Assert-True (Test-Path -LiteralPath $result.BackupDirectory -PathType Container) `
            'Bootstrap did not create its audit/backup directory.'
        Assert-True (Test-Path -LiteralPath (Join-Path $result.BackupDirectory 'INSTALL-LOG.txt') -PathType Leaf) `
            'Bootstrap install log is missing.'
    }

    Invoke-TestCase 'Stable bootstrap preserves existing settings and built-in Mod files' {
        $tree = New-GameTree 'bootstrap-preserves-existing'
        $settingsPath = Join-Path $tree.Win64 'UE4SS-settings.ini'
        $modsDirectory = Join-Path $tree.Win64 'Mods'
        $builtInPath = Join-Path $modsDirectory 'ConsoleEnablerMod\Scripts\main.lua'
        New-Item -ItemType Directory -Path (Split-Path -Parent $builtInPath) -Force | Out-Null
        $settingsText = "[General]`nEnableHotReloadSystem = 0`n; preserve-user-settings`n"
        $builtInBytes = [Text.Encoding]::UTF8.GetBytes('-- preserve-user-built-in')
        [IO.File]::WriteAllText($settingsPath, $settingsText, [Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllBytes($builtInPath, $builtInBytes)
        [IO.File]::WriteAllText(
            (Join-Path $modsDirectory 'mods.txt'),
            "; preserve-user-mod-list`nConsoleEnablerMod : 0`n",
            [Text.UTF8Encoding]::new($false))
        $settingsHash = Get-Sha256 $settingsPath
        $builtInHash = Get-Sha256 $builtInPath

        $result = Invoke-Installer $tree.GameExecutable 'F11' $false
        Assert-Equal 'stable-root UE4SS ABI' $result.LayoutDescription 'Wrong preserved bootstrap layout.'
        Assert-Equal $settingsHash (Get-Sha256 $settingsPath) `
            'Bootstrap overwrote an existing UE4SS-settings.ini.'
        Assert-Equal $builtInHash (Get-Sha256 $builtInPath) `
            'Bootstrap overwrote an existing built-in Mod file.'
        Assert-ModsTxtControlled `
            (Join-Path $modsDirectory 'mods.txt') `
            $false `
            $true `
            @('; preserve-user-mod-list', 'ConsoleEnablerMod : 0')
        Assert-FileHash (Join-Path $tree.Win64 'UE4SS.dll') $expectedStableUE4SSHash `
            'Preserving bootstrap did not install approved stable UE4SS.'
        Assert-FileHash (Join-Path $tree.Win64 'dwmapi.dll') $expectedStableDwmapiHash `
            'Preserving bootstrap did not install approved stable proxy.'
    }

    Invoke-TestCase 'Existing stable layout preserves mods.txt format and removes legacy enablement' {
        $tree = New-GameTree 'existing-stable'
        Initialize-StableLayout $tree
        $modsDirectory = Join-Path $tree.Win64 'Mods'
        $modDirectory = Join-Path $modsDirectory $modName
        New-Item -ItemType Directory -Path (Join-Path $modDirectory 'dlls') -Force | Out-Null
        [IO.File]::WriteAllBytes((Join-Path $modDirectory 'dlls\main.dll'), [Text.Encoding]::ASCII.GetBytes('old-plugin'))
        $nestedLegacy = Join-Path $tree.Win64 "ue4ss\Mods\$modName\enabled.txt"
        New-Item -ItemType Directory -Path (Split-Path -Parent $nestedLegacy) -Force | Out-Null
        [IO.File]::WriteAllText((Join-Path $modDirectory 'enabled.txt'), 'legacy-root')
        [IO.File]::WriteAllText($nestedLegacy, 'legacy-nested')

        $modsText = "; preserved comment`nOtherMod : 1`n$modName : 0`n# preserved middle`n$modName : 1`n"
        $body = [Text.UTF8Encoding]::new($false).GetBytes($modsText)
        $preamble = [Text.Encoding]::UTF8.GetPreamble()
        $modsBytes = New-Object byte[] ($preamble.Length + $body.Length)
        [Array]::Copy($preamble, 0, $modsBytes, 0, $preamble.Length)
        [Array]::Copy($body, 0, $modsBytes, $preamble.Length, $body.Length)
        [IO.File]::WriteAllBytes((Join-Path $modsDirectory 'mods.txt'), $modsBytes)

        $result = Invoke-Installer $tree.GameExecutable 'F12' $false
        Assert-Equal 'stable-root UE4SS ABI' $result.LayoutDescription 'Wrong existing stable layout.'
        Assert-PublicConfig $modDirectory 'F12'
        Assert-ModsTxtControlled `
            (Join-Path $modsDirectory 'mods.txt') `
            $true `
            $true `
            @('; preserved comment', 'OtherMod : 1', '# preserved middle')
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $modDirectory 'enabled.txt'))) `
            'Stable legacy enabled.txt still bypasses mods.txt.'
        Assert-True (-not (Test-Path -LiteralPath $nestedLegacy)) `
            'Nested legacy enabled.txt still bypasses mods.txt.'

        $stablePluginPath = Join-Path $modDirectory 'dlls\main.dll'
        $expectedStablePluginHash = [BitConverter]::ToString(
            [Security.Cryptography.SHA256]::Create().ComputeHash(
                (Get-ResourceBytes 'Payload.StablePlugin.dll'))).Replace('-', '')
        Assert-FileHash $stablePluginPath $expectedStablePluginHash 'Stable ABI plugin selection failed.'
        Assert-True (Test-Path -LiteralPath (Join-Path $result.BackupDirectory 'original\Mods\mods.txt')) `
            'The original controlling mods.txt was not backed up.'
        Assert-True (Test-Path -LiteralPath (Join-Path $result.BackupDirectory "original\Mods\$modName\dlls\main.dll")) `
            'The previous native plugin was not backed up.'
        Assert-True (Test-Path -LiteralPath (Join-Path $result.BackupDirectory "original\Mods\$modName\enabled.txt")) `
            'The removed stable enabled.txt was not backed up.'
        Assert-True (Test-Path -LiteralPath (Join-Path $result.BackupDirectory "original\ue4ss\Mods\$modName\enabled.txt")) `
            'The removed nested enabled.txt was not backed up.'
    }

    Invoke-TestCase 'Stable layout rejects unsupported ControllingModsTxt fail closed' {
        $tree = New-GameTree 'stable-unsupported-controlling'
        Initialize-StableLayout $tree
        $settingsPath = Join-Path $tree.Win64 'UE4SS-settings.ini'
        [IO.File]::WriteAllText(
            $settingsPath,
            "[Overrides]`r`nControllingModsTxt = Control\mods.txt`r`n",
            [Text.UTF8Encoding]::new($false))
        $before = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false `
            'does not support ControllingModsTxt or +/-ModsFolderPaths' | Out-Null
        $after = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Assert-Equal $before $after 'Stable ControllingModsTxt rejection changed product or user files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 "Mods\$modName"))) `
            'Stable ControllingModsTxt rejection installed the Mod.'
    }

    Invoke-TestCase 'Invalid interaction fallback key is rejected without mutation' {
        $tree = New-GameTree 'invalid-interaction-fallback'
        Initialize-StableLayout $tree
        $before = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false `
            'Unsupported interaction fallback key' 'AUTO' | Out-Null
        $after = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Assert-Equal $before $after 'Invalid interaction fallback rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Invalid interaction fallback rejection created a transaction directory.'
    }

    Invoke-TestCase 'Stable layout rejects unsupported additional ModsFolderPaths fail closed' {
        $tree = New-GameTree 'stable-unsupported-additional'
        Initialize-StableLayout $tree
        $settingsPath = Join-Path $tree.Win64 'UE4SS-settings.ini'
        [IO.File]::WriteAllText(
            $settingsPath,
            "[Overrides]`r`n+ModsFolderPaths = ExtraMods`r`n",
            [Text.UTF8Encoding]::new($false))
        $before = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false `
            'does not support ControllingModsTxt or +/-ModsFolderPaths' | Out-Null
        $after = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Assert-Equal $before $after 'Stable additional ModsFolderPaths rejection changed product or user files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 "Mods\$modName"))) `
            'Stable additional ModsFolderPaths rejection installed the Mod.'
    }

    Invoke-TestCase 'Existing experimental nested layout receives the experimental ABI plugin' {
        $tree = New-GameTree 'existing-experimental'
        Initialize-ExperimentalLayout $tree
        $modsDirectory = Join-Path $tree.Win64 'ue4ss\Mods'
        [IO.File]::WriteAllText((Join-Path $modsDirectory 'mods.txt'), "OtherMod : 0`r`n")

        $result = Invoke-Installer $tree.GameExecutable 'NUM3' $false
        Assert-Equal 'experimental-nested UE4SS ABI' $result.LayoutDescription 'Wrong experimental layout.'
        Assert-FileHash (Join-Path $tree.Win64 'dwmapi.dll') $expectedExperimentalDwmapiHash `
            'Experimental layout did not retain the exact approved proxy DLL.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'UE4SS.dll'))) `
            'Experimental install incorrectly bootstrapped stable UE4SS.'
        $modDirectory = Join-Path $modsDirectory $modName
        Assert-PublicConfig $modDirectory 'NUM3'
        Assert-ModsTxtControlled (Join-Path $modsDirectory 'mods.txt') $false $false @('OtherMod : 0')
        $expectedExperimentalPluginHash = [BitConverter]::ToString(
            [Security.Cryptography.SHA256]::Create().ComputeHash(
                (Get-ResourceBytes 'Payload.ExperimentalPlugin.dll'))).Replace('-', '')
        Assert-FileHash (Join-Path $modDirectory 'dlls\main.dll') $expectedExperimentalPluginHash `
            'Experimental ABI plugin selection failed.'
    }

    Invoke-TestCase 'Experimental layout falls back to legacy Win64 Mods when nested Mods is absent' {
        $tree = New-GameTree 'experimental-legacy-mods-fallback'
        Initialize-ExperimentalLayout $tree $false $true
        $legacyModsDirectory = Join-Path $tree.Win64 'Mods'
        New-Item -ItemType Directory -Path $legacyModsDirectory -Force | Out-Null
        [IO.File]::WriteAllText(
            (Join-Path $legacyModsDirectory 'mods.txt'),
            "LegacyOtherMod : 1`r`n",
            [Text.UTF8Encoding]::new($false))

        $result = Invoke-Installer $tree.GameExecutable 'PAGEUP' $false
        Assert-Equal 'experimental-nested UE4SS ABI' $result.LayoutDescription 'Wrong legacy fallback ABI.'
        $legacyModDirectory = Join-Path $legacyModsDirectory $modName
        Assert-PublicConfig $legacyModDirectory 'PAGEUP'
        Assert-ModsTxtControlled `
            (Join-Path $legacyModsDirectory 'mods.txt') `
            $false `
            $false `
            @('LegacyOtherMod : 1')
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 "ue4ss\Mods\$modName"))) `
            'Legacy fallback also installed a duplicate nested Mod.'
        Assert-True ([IO.File]::ReadAllText((Join-Path $result.BackupDirectory 'INSTALL-LOG.txt')).Contains(
                "Mods directory: $legacyModsDirectory")) `
            'Install audit log does not identify the selected legacy Win64 Mods directory.'
    }

    Invoke-TestCase 'Experimental relative controlling and additional paths resolve from nested UE4SS' {
        $tree = New-GameTree 'experimental-relative-paths'
        Initialize-ExperimentalLayout $tree
        $nestedUE4SS = Join-Path $tree.Win64 'ue4ss'
        $settingsPath = Join-Path $nestedUE4SS 'UE4SS-settings.ini'
        $controlDirectory = Join-Path $nestedUE4SS 'Control'
        $additionalDirectory = Join-Path $nestedUE4SS 'AdditionalMods'
        New-Item -ItemType Directory -Path $controlDirectory,$additionalDirectory -Force | Out-Null
        [IO.File]::WriteAllText(
            (Join-Path $controlDirectory 'mods.txt'),
            "ControlOtherMod : 0`n",
            [Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllText(
            (Join-Path $additionalDirectory 'mods.txt'),
            "AdditionalOtherMod : 1`r`n",
            [Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllText(
            $settingsPath,
            "[Overrides]`r`nControllingModsTxt = .\Control\mods.txt`r`n+ModsFolderPaths = .\AdditionalMods`r`n",
            [Text.UTF8Encoding]::new($false))

        $result = Invoke-Installer $tree.GameExecutable 'PAGEDOWN' $false
        $mainModsDirectory = Join-Path $nestedUE4SS 'Mods'
        Assert-PublicConfig (Join-Path $mainModsDirectory $modName) 'PAGEDOWN'
        Assert-ModsTxtControlled `
            (Join-Path $controlDirectory 'mods.txt') `
            $false `
            $true `
            @('ControlOtherMod : 0')
        $additionalText = [IO.File]::ReadAllText((Join-Path $additionalDirectory 'mods.txt'))
        Assert-Equal "AdditionalOtherMod : 1`r`n" $additionalText `
            'Additional relative mods.txt was unexpectedly selected as controlling or modified.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $mainModsDirectory 'mods.txt'))) `
            'Default nested mods.txt was written despite an explicit controlling path.'
        $record = [IO.File]::ReadAllText((Join-Path $mainModsDirectory "$modName\INSTALL-RECORD.txt"))
        Assert-True $record.Contains("Controlling mods.txt: $(Join-Path $controlDirectory 'mods.txt')") `
            'Relative ControllingModsTxt did not resolve from nested UE4SS.'
        Assert-True (Test-Path -LiteralPath $result.BackupDirectory -PathType Container) `
            'Relative-path install did not produce an audit directory.'
    }

    Invoke-TestCase 'Ambiguous dual UE4SS layout is rejected without mutation' {
        $tree = New-GameTree 'ambiguous-layout'
        Initialize-StableLayout $tree
        $nestedDll = Join-Path $tree.Win64 'ue4ss\UE4SS.dll'
        New-Item -ItemType Directory -Path (Split-Path -Parent $nestedDll) -Force | Out-Null
        Copy-Item -LiteralPath $script:experimentalUE4SSDll -Destination $nestedDll
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false 'Both stable-root and experimental-nested' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Ambiguous-layout rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Ambiguous-layout rejection created a transaction directory.'
    }

    Invoke-TestCase 'Unknown UE4SS is rejected without mutation' {
        $tree = New-GameTree 'unknown-ue4ss'
        [IO.File]::WriteAllBytes((Join-Path $tree.Win64 'UE4SS.dll'), [Text.Encoding]::ASCII.GetBytes('unknown-ue4ss'))
        [IO.File]::WriteAllText((Join-Path $tree.Win64 'UE4SS-settings.ini'), "[General]`r`n")
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false 'not a supported release' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Unknown-UE4SS rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Unknown-UE4SS rejection created a transaction directory.'
    }

    Invoke-TestCase 'Stable layout with missing proxy is rejected without mutation' {
        $tree = New-GameTree 'stable-missing-proxy'
        Initialize-StableLayout $tree
        Remove-Item -LiteralPath (Join-Path $tree.Win64 'dwmapi.dll')
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false 'Win64\dwmapi.dll is missing' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Stable missing-proxy rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Stable missing-proxy rejection created a transaction directory.'
    }

    Invoke-TestCase 'Stable layout with wrong proxy is rejected without mutation' {
        $tree = New-GameTree 'stable-wrong-proxy'
        Initialize-StableLayout $tree
        Copy-Item -LiteralPath $script:experimentalDwmapiDll -Destination (Join-Path $tree.Win64 'dwmapi.dll') -Force
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false 'not the approved proxy' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Stable wrong-proxy rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Stable wrong-proxy rejection created a transaction directory.'
    }

    Invoke-TestCase 'Experimental layout with missing proxy is rejected without mutation' {
        $tree = New-GameTree 'experimental-missing-proxy'
        Initialize-ExperimentalLayout $tree $true $false
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false 'Win64\dwmapi.dll is missing' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Experimental missing-proxy rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Experimental missing-proxy rejection created a transaction directory.'
    }

    Invoke-TestCase 'Experimental layout with wrong proxy is rejected without mutation' {
        $tree = New-GameTree 'experimental-wrong-proxy'
        Initialize-ExperimentalLayout $tree
        [IO.File]::WriteAllBytes(
            (Join-Path $tree.Win64 'dwmapi.dll'),
            (Get-ZipEntryBytes 'dwmapi.dll'))
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false 'not the approved proxy' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Experimental wrong-proxy rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Experimental wrong-proxy rejection created a transaction directory.'
    }

    Invoke-TestCase 'Optional range selection installs, switches, reconciles, and removes approved PAKs' {
        $tree = New-GameTree 'optional-range-variants'
        Initialize-StableLayout $tree
        $x3Path = Join-Path $tree.PakDirectory $rangeX3PakName
        $x5Path = Join-Path $tree.PakDirectory $rangeX5PakName
        $x10Path = Join-Path $tree.PakDirectory $rangeX10PakName

        $x3Result = Invoke-Installer $tree.GameExecutable 'HOME' 3
        Assert-Equal 3 $x3Result.RangeMultiplierInstalled 'Installer did not report the selected 3x PAK.'
        Assert-FileHash $x3Path $expectedRangeX3PakHash 'Installed optional 3x PAK identity failed.'
        Assert-True (-not (Test-Path -LiteralPath $x5Path)) 'Installing 3x unexpectedly installed 5x.'
        Assert-True (-not (Test-Path -LiteralPath $x10Path)) 'Installing 3x unexpectedly installed 10x.'

        $x5Result = Invoke-Installer $tree.GameExecutable 'HOME' 5
        Assert-Equal 5 $x5Result.RangeMultiplierInstalled 'Installer did not report the selected 5x PAK.'
        Assert-True (-not (Test-Path -LiteralPath $x3Path)) 'Switching to 5x did not remove 3x.'
        Assert-FileHash $x5Path $expectedRangeX5PakHash 'Installed optional 5x PAK identity failed.'
        $x3Backups = @(Get-ChildItem -LiteralPath $x5Result.BackupDirectory -Recurse -Force -File |
            Where-Object { $_.Name -like "*-$rangeX3PakName" })
        Assert-Equal 1 $x3Backups.Count 'Switching to 5x did not back up 3x exactly once.'
        Assert-Equal $expectedRangeX3PakHash (Get-Sha256 $x3Backups[0].FullName) `
            'The backed-up 3x PAK identity changed.'

        # Simulate a previous interrupted/manual state containing two approved
        # product PAKs. Selecting 10x must reconcile both transactionally.
        New-Item -ItemType Directory -Path $tree.PakDirectory -Force | Out-Null
        [IO.File]::WriteAllBytes($x3Path, (Get-ResourceBytes 'Payload.PickupRangeX3.pak'))
        $x10Result = Invoke-Installer $tree.GameExecutable 'HOME' 10
        Assert-Equal 10 $x10Result.RangeMultiplierInstalled 'Installer did not report the selected 10x PAK.'
        Assert-True (-not (Test-Path -LiteralPath $x3Path)) 'Reconciling to 10x did not remove 3x.'
        Assert-True (-not (Test-Path -LiteralPath $x5Path)) 'Reconciling to 10x did not remove 5x.'
        Assert-FileHash $x10Path $expectedRangeX10PakHash 'Installed optional 10x PAK identity failed.'

        $removeResult = Invoke-Installer $tree.GameExecutable 'HOME' 0
        Assert-Equal 0 $removeResult.RangeMultiplierInstalled 'Installer did not report original range.'
        Assert-True (-not $removeResult.RangePakInstalled) 'Installer result did not report PAK removal.'
        Assert-True (-not (Test-Path -LiteralPath $x10Path)) 'Selecting original range did not remove 10x.'
        $x10Backups = @(Get-ChildItem -LiteralPath $removeResult.BackupDirectory -Recurse -Force -File |
            Where-Object { $_.Name -like "*-$rangeX10PakName" })
        Assert-Equal 1 $x10Backups.Count 'Removed 10x PAK was not backed up exactly once.'
        Assert-Equal $expectedRangeX10PakHash (Get-Sha256 $x10Backups[0].FullName) `
            'The optional 10x PAK backup does not match the removed file.'
    }

    Invoke-TestCase 'Recognized same-name 5x PAK is replaced regardless of prior hash' {
        $tree = New-GameTree 'recognized-same-name-x5'
        Initialize-StableLayout $tree
        New-Item -ItemType Directory -Path $tree.PakDirectory -Force | Out-Null
        $pakPath = Join-Path $tree.PakDirectory $rangeX5PakName
        [IO.File]::WriteAllBytes($pakPath, [Text.Encoding]::ASCII.GetBytes('older-same-name-pak'))
        $result = Invoke-Installer $tree.GameExecutable 'F9' 5
        Assert-FileHash $pakPath $expectedRangeX5PakHash `
            'The recognized same-name 5x PAK was not replaced with the embedded release artifact.'
        $backups = @(Get-ChildItem -LiteralPath $result.BackupDirectory -Recurse -Force -File |
            Where-Object { $_.Name -like "*-$rangeX5PakName" })
        Assert-Equal 1 $backups.Count 'The replaced same-name 5x PAK was not backed up exactly once.'
        Assert-Equal 'older-same-name-pak' `
            ([Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($backups[0].FullName))) `
            'The backed-up previous same-name PAK changed.'
    }

    Invoke-TestCase 'Recognized legacy canary PAK is removed regardless of prior hash' {
        $tree = New-GameTree 'recognized-legacy-x3'
        Initialize-StableLayout $tree
        New-Item -ItemType Directory -Path $tree.PakDirectory -Force | Out-Null
        $legacyPakPath = Join-Path $tree.PakDirectory 'DS_PickupRangeX3Canary_P.pak'
        [IO.File]::WriteAllBytes($legacyPakPath, [Text.Encoding]::ASCII.GetBytes('older-legacy-pak'))
        $result = Invoke-Installer $tree.GameExecutable 'F9' 5
        Assert-True (-not (Test-Path -LiteralPath $legacyPakPath)) `
            'The recognized legacy canary PAK was not removed.'
        $backups = @(Get-ChildItem -LiteralPath $result.BackupDirectory -Recurse -Force -File |
            Where-Object { $_.Name -like '*-DS_PickupRangeX3Canary_P.pak' })
        Assert-Equal 1 $backups.Count 'The removed legacy canary PAK was not backed up exactly once.'
        Assert-Equal 'older-legacy-pak' `
            ([Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($backups[0].FullName))) `
            'The backed-up legacy canary PAK changed.'
    }

    Invoke-TestCase 'PAK directory junction is rejected without mutation' {
        $tree = New-GameTree 'pak-junction-rejected'
        Initialize-StableLayout $tree
        $paksDirectory = Join-Path $tree.Root 'DS\Content\Paks'
        $junctionTarget = Join-Path $tree.Root 'DS\Content\JunctionTarget'
        $junctionPath = Join-Path $paksDirectory '~mods'
        New-Item -ItemType Directory -Path $paksDirectory,$junctionTarget -Force | Out-Null
        try {
            New-Item -ItemType Junction -Path $junctionPath -Target $junctionTarget -ErrorAction Stop | Out-Null
        }
        catch {
            Skip-Test ("Directory junction creation is unavailable in this environment: " + $_.Exception.Message)
        }
        Assert-True ((Get-Item -LiteralPath $junctionPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) `
            'The test fixture did not create a reparse point.'
        $before = Get-FileSnapshot $tree.Root
        Invoke-InstallerFailure $tree.GameExecutable 'F9' $false `
            'crosses a symbolic link, junction, or other reparse point' | Out-Null
        $after = Get-FileSnapshot $tree.Root
        Assert-Equal $before $after 'Junction rejection mutated files.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups'))) `
            'Junction rejection created a transaction directory.'
    }

    Invoke-TestCase 'Late write failure rolls back every recorded file mutation' {
        $tree = New-GameTree 'rollback'
        Initialize-StableLayout $tree
        $modsDirectory = Join-Path $tree.Win64 'Mods'
        $modDirectory = Join-Path $modsDirectory $modName
        New-Item -ItemType Directory -Path (Join-Path $modDirectory 'dlls') -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $modDirectory 'INSTALL-RECORD.txt') -Force | Out-Null
        [IO.File]::WriteAllBytes((Join-Path $modDirectory 'dlls\main.dll'), [Text.Encoding]::ASCII.GetBytes('rollback-plugin'))
        [IO.File]::WriteAllText((Join-Path $modDirectory 'config.ini'), 'rollback-config')
        [IO.File]::WriteAllText((Join-Path $modDirectory 'enabled.txt'), 'rollback-enabled')
        [IO.File]::WriteAllText((Join-Path $modsDirectory 'mods.txt'), "OtherMod : 1`r`n$modName : 0`r`n")
        New-Item -ItemType Directory -Path $tree.PakDirectory -Force | Out-Null
        [IO.File]::WriteAllBytes(
            (Join-Path $tree.PakDirectory $rangeX5PakName),
            (Get-ResourceBytes 'Payload.PickupRangeX5.pak'))

        $before = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Invoke-InstallerFailure $tree.GameExecutable 'END' $false 'all recorded file mutations were restored' | Out-Null
        $after = Get-FileSnapshot $tree.Root @('DragonSwordNativeAutoPickup-Backups')
        Assert-Equal $before $after 'File-level transaction rollback did not restore the original tree.'
        $rollbackLogs = @(Get-ChildItem -LiteralPath (Join-Path $tree.Win64 'DragonSwordNativeAutoPickup-Backups') `
            -Recurse -Force -File -Filter 'ROLLBACK-LOG.txt')
        Assert-Equal 1 $rollbackLogs.Count 'Rollback did not emit exactly one rollback log.'
        Assert-True ([IO.File]::ReadAllText($rollbackLogs[0].FullName).Contains('Automatic rollback completed.')) `
            'Rollback log does not confirm successful automatic rollback.'
    }
}
finally {
    if ($KeepArtifacts) {
        Write-Host "Installer test artifacts retained: $script:runRoot"
    }
    elseif (Test-Path -LiteralPath $script:runRoot) {
        $resolvedRunRoot = [IO.Path]::GetFullPath($script:runRoot)
        $resolvedBase = [IO.Path]::GetFullPath($workBase).TrimEnd([IO.Path]::DirectorySeparatorChar) +
            [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedRunRoot.StartsWith($resolvedBase, [StringComparison]::OrdinalIgnoreCase) -or
            -not ([IO.Path]::GetFileName($resolvedRunRoot)).StartsWith('dsnap-installer-tests-', [StringComparison]::Ordinal)) {
            throw "Refusing to clean an unexpected test path: $resolvedRunRoot"
        }
        Remove-Item -LiteralPath $resolvedRunRoot -Recurse -Force
    }
}

Set-Variable -Name expectedTestCount -Value 20 -Option Constant
Write-Host "Installer integration tests: $script:passed passed, $script:failed failed, $script:skipped skipped."
if ($script:failed -ne 0) {
    throw ([string]::Join([Environment]::NewLine, $script:failures.ToArray()))
}
if ($script:passed -ne $expectedTestCount -or $script:skipped -ne 0) {
    throw "Installer integration release gate requires exactly $expectedTestCount passed, 0 failed, and 0 skipped; actual=$script:passed/$script:failed/$script:skipped."
}

[pscustomobject]@{
    expected = $expectedTestCount
    passed = $script:passed
    failed = $script:failed
    skipped = $script:skipped
    release_gate = 'PASSED'
}
