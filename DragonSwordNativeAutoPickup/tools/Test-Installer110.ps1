[CmdletBinding()]
param(
    [string]$InstallerExe,
    [string]$GameExecutable = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Binaries\Win64\DSClient-Win64-Shipping.exe',
    [string]$WorkingDirectory,
    [switch]$KeepArtifacts
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$expectedCount = 11
$product = 'DragonSwordNativeAutoPickup'
$project = Split-Path -Parent $PSScriptRoot

function Assert([bool]$Value, [string]$Message) {
    if (-not $Value) { throw $Message }
}

function Equal($Actual, $Expected, [string]$Message) {
    if (-not [object]::Equals($Actual, $Expected)) {
        throw "$Message Expected=[$Expected] Actual=[$Actual]"
    }
}

function Hash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function HashBytes([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '') }
    finally { $sha.Dispose() }
}

function WriteText([string]$Path, [string]$Text) {
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($Path)))
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

function Prop($Object, [string]$Name) {
    if ($null -eq $Object) { throw "Cannot read property '$Name' from a null object." }
    $property = $Object.GetType().GetProperty(
        $Name,
        [Reflection.BindingFlags]'Instance, Public, NonPublic')
    if ($null -eq $property) { throw "Missing installer contract property: $Name" }
    $property.GetValue($Object, $null)
}

function ErrorText([Exception]$Exception) {
    $parts = [Collections.Generic.List[string]]::new()
    while ($null -ne $Exception) {
        if (-not [string]::IsNullOrWhiteSpace($Exception.Message)) { $parts.Add($Exception.Message) }
        $Exception = $Exception.InnerException
    }
    $parts -join ' | '
}

function FindMethod([Type]$Type, [string[]]$Names, [int]$ParameterCount) {
    $flags = [Reflection.BindingFlags]'Static, Public, NonPublic'
    foreach ($name in $Names) {
        $matches = @($Type.GetMethods($flags) | Where-Object {
            $_.Name -eq $name -and $_.GetParameters().Count -eq $ParameterCount
        })
        if ($matches.Count -eq 1) { return $matches[0] }
        if ($matches.Count -gt 1) { throw "Ambiguous installer method: $name/$ParameterCount" }
    }
    throw "Missing installer contract method: $($Names -join ' or ')/$ParameterCount"
}

function FindType([Reflection.Assembly]$Assembly, [string[]]$Names) {
    foreach ($name in $Names) {
        $type = $Assembly.GetType($name, $false, $false)
        if ($null -ne $type) { return $type }
    }
    throw "Missing installer contract type: $($Names -join ' or ')"
}

function GetResourceBytes([string]$Name) {
    $stream = $script:Assembly.GetManifestResourceStream($Name)
    if ($null -eq $stream) { throw "Missing embedded installer resource: $Name" }
    try {
        $memory = [IO.MemoryStream]::new()
        try { $stream.CopyTo($memory); $memory.ToArray() }
        finally { $memory.Dispose() }
    } finally { $stream.Dispose() }
}

function TreeHash([string]$Root) {
    if (-not (Test-Path -LiteralPath $Root)) { return '<missing>' }
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $entries = [Collections.Generic.List[string]]::new()
    foreach ($directory in Get-ChildItem -LiteralPath $rootFull -Recurse -Force -Directory | Sort-Object FullName) {
        $entries.Add('D:' + $directory.FullName.Substring($rootFull.Length))
    }
    foreach ($file in Get-ChildItem -LiteralPath $rootFull -Recurse -Force -File | Sort-Object FullName) {
        $entries.Add('F:' + $file.FullName.Substring($rootFull.Length) + '=' + (Hash $file.FullName))
    }
    $entries -join "`n"
}

function AssertNoPersistentBackup($Fixture) {
    $directories = @(Get-ChildItem -LiteralPath $Fixture.Win64 -Recurse -Force -Directory | Where-Object {
        $_.Name -like 'UE4SS-*-Backup' -or
        $_.Name -like '.dsnap-transaction-*' -or
        $_.Name -like '.DragonSwordNativeAutoPickup-transaction-*' -or
        $_.Name -like 'DragonSwordNativeAutoPickup-Backup*' -or
        $_.Name -eq 'DragonSwordNativeAutoPickup-Backups'
    })
    $names = @($directories | ForEach-Object { $_.FullName }) -join ', '
    Assert ($directories.Count -eq 0) "Unexpected persistent backup or transaction directory: $names"
}

function FindPersistentConversionBackup($Fixture) {
    $backups = @(Get-ChildItem -LiteralPath $Fixture.Win64 -Directory -Force | Where-Object {
        $_.Name -like 'UE4SS-*-Backup'
    } | Sort-Object Name)
    Assert ($backups.Count -eq 1) "Expected one persistent conversion backup; found $($backups.Count)."
    $backups[0]
}

function ExpandEmbeddedRuntime([string]$Win64) {
    $bytes = GetResourceBytes 'Payload.ExperimentalUE4SSRuntime.zip'
    $memory = [IO.MemoryStream]::new($bytes, $false)
    try {
        $archive = [IO.Compression.ZipArchive]::new(
            $memory,
            [IO.Compression.ZipArchiveMode]::Read,
            $false)
        try {
            $win64Full = [IO.Path]::GetFullPath($Win64).TrimEnd('\') + '\'
            foreach ($entry in $archive.Entries) {
                if ([string]::IsNullOrEmpty($entry.Name)) { continue }
                $name = $entry.FullName.Replace('\', '/')
                if (-not [string]::Equals($name, 'dwmapi.dll', [StringComparison]::OrdinalIgnoreCase) -and
                    -not $name.StartsWith('ue4ss/', [StringComparison]::OrdinalIgnoreCase)) { continue }
                $target = [IO.Path]::GetFullPath((Join-Path $Win64 $name.Replace('/', '\')))
                Assert ($target.StartsWith($win64Full, [StringComparison]::OrdinalIgnoreCase)) `
                    "Runtime fixture escaped Win64: $name"
                [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target))
                $input = $entry.Open()
                try {
                    $output = [IO.File]::Create($target)
                    try { $input.CopyTo($output) } finally { $output.Dispose() }
                } finally { $input.Dispose() }
            }
        } finally { $archive.Dispose() }
    } finally { $memory.Dispose() }
}

function NewFixture([string]$Root, [ValidateSet('Bare','Exact','OtherRoot')]$Mode) {
    $win64 = Join-Path $Root 'DS\Binaries\Win64'
    [void][IO.Directory]::CreateDirectory($win64)
    $game = Join-Path $win64 'DSClient-Win64-Shipping.exe'
    try {
        [void](New-Item -ItemType HardLink -Path $game -Target $script:Game)
    } catch {
        [IO.File]::Copy($script:Game, $game, $false)
    }

    if ($Mode -eq 'Exact') {
        ExpandEmbeddedRuntime $win64
        $mods = Join-Path $win64 'ue4ss\Mods'
        [void][IO.Directory]::CreateDirectory((Join-Path $mods 'UserExample'))
        WriteText (Join-Path $mods 'UserExample\config.ini') "preserve=true`r`n"
        WriteText (Join-Path $mods 'mods.txt') "UserExample : 1`r`n"
    } elseif ($Mode -eq 'OtherRoot') {
        [IO.File]::WriteAllBytes((Join-Path $win64 'UE4SS.dll'), [byte[]](1,2,3,4,5))
        [IO.File]::WriteAllBytes((Join-Path $win64 'dwmapi.dll'), [byte[]](6,7,8,9))
        WriteText (Join-Path $win64 'UE4SS-settings.ini') "[Overrides]`r`n"
        WriteText (Join-Path $win64 'UE4SS.log') "legacy-log`r`n"
        $mods = Join-Path $win64 'Mods'
        [void][IO.Directory]::CreateDirectory((Join-Path $mods 'UserExample'))
        WriteText (Join-Path $mods 'UserExample\config.ini') "preserve=true`r`n"
        WriteText (Join-Path $mods 'mods.txt') "UserExample : 1`r`n"
    }

    $nested = Join-Path $win64 'ue4ss'
    $activeMods = Join-Path $nested 'Mods'
    [pscustomobject]@{
        Root = $Root
        Win64 = $win64
        Nested = $nested
        Mods = $activeMods
        Game = $game
        Target = Join-Path $activeMods $product
        ModsTxt = Join-Path $activeMods 'mods.txt'
        PakDirectory = Join-Path $Root 'DS\Content\Paks\~mods'
    }
}

function State($Fixture) {
    $script:InspectState.Invoke($null, [object[]]@([string]$Fixture.Game))
}

function InspectPlan($Fixture, $Range, [string]$Hotkey = 'F9', [string]$Interaction = 'F') {
    $script:Inspect.Invoke($null, [object[]]@(
        [string]$Fixture.Game,
        [string]$Hotkey,
        [string]$Interaction,
        $Range))
}

function Install($Fixture, $Range, [string]$Hotkey = 'F9', [string]$Interaction = 'F') {
    $plan = InspectPlan $Fixture $Range $Hotkey $Interaction
    $token = [string](Prop $plan 'IdentityToken')
    $result = $script:InstallConfirmed.Invoke($null, [object[]]@(
        [string]$Fixture.Game,
        [string]$Hotkey,
        [string]$Interaction,
        $Range,
        $token))
    [pscustomobject]@{ Plan = $plan; Result = $result }
}

function Uninstall($Fixture, [string]$Identity) {
    $script:UninstallConfirmed.Invoke($null, [object[]]@(
        [string]$Fixture.Game,
        [string]$Identity))
}

function AssertFreshState($State) {
    Assert (-not [bool](Prop $State 'IsInstalled')) 'Absent product was reported as installed.'
    Assert (-not [bool](Prop $State 'IsOwned')) 'Absent product was reported as owned.'
    Assert ([bool](Prop $State 'CanInstall')) 'Fresh install is not enabled.'
    Assert (-not [bool](Prop $State 'CanUpgrade')) 'Fresh install incorrectly enables upgrade.'
    Assert (-not [bool](Prop $State 'CanRepair')) 'Fresh install incorrectly enables repair.'
    Assert (-not [bool](Prop $State 'CanUninstall')) 'Fresh install incorrectly enables uninstall.'
    Assert ([string]::IsNullOrWhiteSpace([string](Prop $State 'InstalledVersion'))) `
        'Fresh install reported an installed version.'
}

function AssertOwnedState($State) {
    Assert ([bool](Prop $State 'IsInstalled')) 'Installed product was reported as absent.'
    Assert ([bool](Prop $State 'IsOwned')) 'Installed product was not recognized as owned.'
    Assert (-not [bool](Prop $State 'CanInstall')) 'Owned product incorrectly enables fresh install.'
    Assert (-not [bool](Prop $State 'CanUpgrade')) 'Current owned product incorrectly enables upgrade.'
    Assert ([bool](Prop $State 'CanRepair')) 'Current owned product does not enable repair.'
    Assert ([bool](Prop $State 'CanUninstall')) 'Owned product does not enable uninstall.'
    Equal ([string](Prop $State 'InstalledVersion')) '1.3.1' `
        'Current owned product reported the wrong installed version.'
    Assert (-not [string]::IsNullOrWhiteSpace([string](Prop $State 'IdentityToken'))) `
        'Owned product did not expose an uninstall identity token.'
}

function AssertUnknownState($State) {
    Assert ([bool](Prop $State 'IsInstalled')) 'Unknown same-name target was reported as absent.'
    Assert (-not [bool](Prop $State 'IsOwned')) 'Unknown same-name target was accepted as owned.'
    Assert (-not [bool](Prop $State 'CanInstall')) 'Unknown same-name target enables install.'
    Assert (-not [bool](Prop $State 'CanUpgrade')) 'Unknown same-name target enables upgrade.'
    Assert (-not [bool](Prop $State 'CanRepair')) 'Unknown same-name target enables repair.'
    Assert (-not [bool](Prop $State 'CanUninstall')) 'Unknown same-name target enables uninstall.'
}

function ProductEntryCount([string]$ModsTxt) {
    if (-not (Test-Path -LiteralPath $ModsTxt)) { return 0 }
    @([IO.File]::ReadAllLines($ModsTxt) | Where-Object {
        $_ -match '^\s*DragonSwordNativeAutoPickup\s*:\s*[01]\s*(?:[#;].*)?$'
    }).Count
}

if ([string]::IsNullOrWhiteSpace($InstallerExe)) {
    $candidate = Join-Path $project 'out\installer\1.3.1\DragonSwordNativeAutoPickup-Setup-1.3.1.exe'
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "InstallerExe was not supplied and the expected 1.3.1 build is missing: $candidate"
    }
    $InstallerExe = $candidate
}
if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
    $WorkingDirectory = Join-Path $project 'out\tests\installer'
}

$script:Installer = (Resolve-Path -LiteralPath $InstallerExe).Path
& (Join-Path $project 'installer\tests\HotkeyConfiguration.Tests.ps1') -InstallerExe $script:Installer
$script:Game = (Resolve-Path -LiteralPath $GameExecutable).Path
[void][IO.Directory]::CreateDirectory($WorkingDirectory)
$working = (Resolve-Path -LiteralPath $WorkingDirectory).Path
$runRoot = Join-Path $working ('DSNAP-Installer131-' + [Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($runRoot)

$script:Assembly = [Reflection.Assembly]::LoadFile($script:Installer)
$engine = FindType $script:Assembly @(
    'DragonSwordNativeAutoPickup.Installer.InstallerEngine110')
$rangeType = FindType $script:Assembly @(
    'DragonSwordNativeAutoPickup.Installer.InstallerEngine+RangeSelection',
    'DragonSwordNativeAutoPickup.Installer.InstallerEngine110+RangeSelection',
    'DragonSwordNativeAutoPickup.Installer.RangeSelection')
$script:Inspect = FindMethod $engine @('Inspect') 4
$script:InstallConfirmed = FindMethod $engine @('InstallConfirmed') 5
$script:InspectState = FindMethod $engine @('InspectInstallationState','InspectInstalledState') 1
$script:UninstallConfirmed = FindMethod $engine @('UninstallConfirmed') 2
$script:MatchesKnownOwnedPayload = FindMethod $engine @('MatchesKnownOwnedPayload') 3
$script:Ranges = @{}
foreach ($name in @('None','X3','X5','X10','X15','X20')) {
    $script:Ranges[$name] = [Enum]::Parse($rangeType, $name, $false)
}

$rangeFiles = [ordered]@{
    X3 = 'DS_PickupRangeX3_P.pak'
    X5 = 'DS_PickupRangeX5_P.pak'
    X10 = 'DS_PickupRangeX10_P.pak'
    X15 = 'DS_PickupRangeX15_P.pak'
    X20 = 'DS_PickupRangeX20_P.pak'
}
$rangeResources = [ordered]@{
    X3 = 'Payload.PickupRangeX3.pak'
    X5 = 'Payload.PickupRangeX5.pak'
    X10 = 'Payload.PickupRangeX10.pak'
    X15 = 'Payload.PickupRangeX15.pak'
    X20 = 'Payload.PickupRangeX20.pak'
}

$results = [Collections.Generic.List[object]]::new()
$case = 0
function Run([string]$Name, [scriptblock]$Body) {
    $script:case++
    $root = Join-Path $runRoot ('{0:D2}' -f $script:case)
    [void][IO.Directory]::CreateDirectory($root)
    try {
        & $Body $root
        $script:results.Add([pscustomobject]@{ name=$Name; status='PASSED'; error=$null })
        Write-Host "PASS $($script:case): $Name"
    } catch {
        $message = ErrorText $_.Exception
        $script:results.Add([pscustomobject]@{ name=$Name; status='FAILED'; error=$message })
        Write-Host "FAIL $($script:case): $Name - $message"
    } finally {
        if (-not $KeepArtifacts -and (Test-Path -LiteralPath $root)) {
            [IO.Directory]::Delete($root, $true)
        }
    }
}

Run '1.3.1 lifecycle, range, and historical upgrade contracts are present' {
    param($root)
    foreach ($resource in @(
        'Payload.ExperimentalUE4SSRuntime.zip',
        'Payload.ExperimentalPlugin.dll',
        'Payload.DefaultConfig.ini',
        'Payload.Main.lua',
        'Payload.ThirdPartyNotices.txt',
        'Payload.PickupRangeX15.pak',
        'Payload.PickupRangeX20.pak')) {
        Assert ((GetResourceBytes $resource).Length -gt 0) "Embedded resource is empty: $resource"
    }
    Assert ($null -ne $script:InspectState) 'Installation-state API is missing.'
    Assert ($null -ne $script:UninstallConfirmed) 'Owned uninstall API is missing.'
    $deployed120 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.2.0',
        '9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6',
        '62E47E954D771C9933E5CB02E8E4A8B48EFCFF5B4A3A54190E6CD270063821BF'))
    Assert ([bool]$deployed120) 'The exact deployed 1.2.0 payload is not recognized for an owned in-place upgrade.'
    $released130 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.0',
        '6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert ([bool]$released130) `
        'The exact released 1.3.0 payload is not recognized for an owned repair.'
    $reflectionCandidate130 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.0',
        '4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert ([bool]$reflectionCandidate130) `
        'The installed 1.3.0 reflection-fix candidate is not recognized for an owned repair.'
    $acceptedActionProperty130 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.0',
        '01A6E1358FBFE0B9DB35B4ECAAB2F1F08425265F47D6B1873872AA56D9E002AC',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert ([bool]$acceptedActionProperty130) `
        'The owner-accepted 1.3.0 action-property payload is not recognized for an owned repair.'
    $confirmationWindowTuning130 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.0',
        '09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert ([bool]$confirmationWindowTuning130) `
        'The installed 1.3.0 confirmation-window tuning payload is not recognized for an owned repair.'
    $finalActionLifecycle130 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.0',
        '38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert ([bool]$finalActionLifecycle130) `
        'The installed 1.3.0 final action-lifecycle payload is not recognized for an owned repair.'
    $immediatePredecessor130 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.0',
        '5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert ([bool]$immediatePredecessor130) `
        'The exact installed predecessor is not recognized for one-time ownership-record migration.'
    $oldHashMislabelled131 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.1',
        '6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1',
        '0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A'))
    Assert (-not [bool]$oldHashMislabelled131) `
        'A historical 1.3.0 payload was incorrectly relabelled as current 1.3.1 ownership.'
    $current131 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.3.1',
        (HashBytes (GetResourceBytes 'Payload.ExperimentalPlugin.dll')),
        (HashBytes (GetResourceBytes 'Payload.Main.lua'))))
    Assert ([bool]$current131) 'The embedded 1.3.1 payload is not recognized for owned repair.'
    $mutated120 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.2.0',
        '8AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6',
        '62E47E954D771C9933E5CB02E8E4A8B48EFCFF5B4A3A54190E6CD270063821BF'))
    Assert (-not [bool]$mutated120) 'A mutated 1.2.0 plugin hash was incorrectly accepted as owned.'
    $unproven120 = $script:MatchesKnownOwnedPayload.Invoke($null, [object[]]@(
        '1.2.0',
        'DAEAC64E3FEE280653E300741E5B2FD40CE219C6F243DE32C333F167CE8DCBBD',
        '62E47E954D771C9933E5CB02E8E4A8B48EFCFF5B4A3A54190E6CD270063821BF'))
    Assert (-not [bool]$unproven120) 'An unproven 1.2.0 plugin hash was incorrectly accepted as owned.'
}

Run 'Exact runtime fresh install is backup-free and exposes Repair and Uninstall state' {
    param($root)
    $f = NewFixture $root Exact
    [IO.File]::WriteAllText(
        $f.ModsTxt,
        ('; preserve-this-comment' + [char]10 + 'UserExample : 1' + [char]10),
        [Text.UTF8Encoding]::new($true))
    AssertFreshState (State $f)
    $loader = Hash (Join-Path $f.Nested 'UE4SS.dll')
    $proxy = Hash (Join-Path $f.Win64 'dwmapi.dll')
    $settings = Hash (Join-Path $f.Nested 'UE4SS-settings.ini')
    $operation = Install $f $script:Ranges.None
    Assert (-not [bool](Prop $operation.Plan 'UpdatesExistingAutoPickup')) `
        'Fresh install was classified as an upgrade.'
    Assert ([string]::IsNullOrWhiteSpace([string](Prop $operation.Result 'BackupDirectory'))) `
        'Fresh exact-runtime install reported a persistent backup.'
    Assert (Test-Path -LiteralPath (Join-Path $f.Target 'dlls\main.dll')) 'Installed plugin is missing.'
    Equal (ProductEntryCount $f.ModsTxt) 1 'mods.txt does not contain exactly one product authority entry.'
    Assert ((Get-Content -LiteralPath $f.ModsTxt -Raw) -match '(?m)^\s*UserExample\s*:\s*1\s*$') `
        'Fresh install removed the unrelated mods.txt entry.'
    $modsBytes = [IO.File]::ReadAllBytes($f.ModsTxt)
    Assert ($modsBytes.Length -ge 3 -and $modsBytes[0] -eq 0xEF -and
        $modsBytes[1] -eq 0xBB -and $modsBytes[2] -eq 0xBF) 'Fresh install did not preserve the mods.txt UTF-8 BOM.'
    $modsBody = [Text.Encoding]::UTF8.GetString($modsBytes, 3, $modsBytes.Length - 3)
    Assert ($modsBody.Contains('; preserve-this-comment' + [char]10)) 'Fresh install changed an unrelated mods.txt comment.'
    Assert (-not $modsBody.Contains(([string][char]13) + [char]10)) 'Fresh install changed the mods.txt LF newline style.'
    Equal (Hash (Join-Path $f.Nested 'UE4SS.dll')) $loader 'Fresh install changed the exact loader.'
    Equal (Hash (Join-Path $f.Win64 'dwmapi.dll')) $proxy 'Fresh install changed the exact proxy.'
    Equal (Hash (Join-Path $f.Nested 'UE4SS-settings.ini')) $settings 'Fresh install changed UE4SS settings.'
    AssertNoPersistentBackup $f
    AssertOwnedState (State $f)
    $record = Get-Content -LiteralPath (Join-Path $f.Target 'INSTALL-RECORD.txt') -Raw
    Assert ($record -match '(?m)^Ownership Schema:\s*2\s*$') `
        'Fresh install did not write the durable ownership schema.'
    Assert ($record -match '(?m)^Ownership ID:\s*8F4282F9-25C4-4EFC-A150-1C4A812C85B4\s*$') `
        'Fresh install did not write the stable product ownership ID.'
    Assert ($record -match ('(?m)^Plugin SHA-256:\s*' + [regex]::Escape((Hash (Join-Path $f.Target 'dlls\main.dll'))) + '\s*$')) `
        'Fresh ownership record does not bind the installed plugin hash.'
    Assert ($record -match ('(?m)^Lua SHA-256:\s*' + [regex]::Escape((Hash (Join-Path $f.Target 'Scripts\main.lua'))) + '\s*$')) `
        'Fresh ownership record does not bind the installed Lua hash.'
    Assert ($record -match ('(?m)^Notices SHA-256:\s*' + [regex]::Escape((Hash (Join-Path $f.Target 'THIRD_PARTY_NOTICES.txt'))) + '\s*$')) `
        'Fresh ownership record does not bind the installed notices hash.'

    $malformed = NewFixture (Join-Path $root 'malformed') Exact
    [IO.File]::WriteAllText(
        $malformed.ModsTxt,
        ('UserExample : 1' + [char]10 + 'DragonSwordNativeAutoPickup : invalid' + [char]10),
        [Text.UTF8Encoding]::new($true))
    $malformedBefore = TreeHash $malformed.Root
    $malformedError = $null
    try { [void](Install $malformed $script:Ranges.None) } catch { $malformedError = ErrorText $_.Exception }
    Assert ($malformedError -like '*malformed*') 'Malformed same-name mods.txt entry was not rejected.'
    Equal (TreeHash $malformed.Root) $malformedBefore 'Malformed mods.txt rejection changed files.'
    AssertNoPersistentBackup $malformed

    $bare = NewFixture (Join-Path $root 'bare') Bare
    $bareOperation = Install $bare $script:Ranges.None 'INSERT' 'HOME'
    Equal ([string](Prop $bareOperation.Plan 'ToggleHotkey')) 'INSERT' 'Fresh confirmation lost the chosen key.'
    Equal ([string](Prop (State $bare) 'ToggleHotkey')) 'INSERT' 'Fresh install ignored the chosen toggle.'
    Equal ([string](Prop (State $bare) 'InteractionKeyFallback')) 'HOME' 'Fresh install ignored the chosen fallback.'
    Assert ([bool](Prop $bareOperation.Plan 'BootstrapsUE4SS')) 'Bare fixture was not classified for UE4SS bootstrap.'
    $bareMods = [IO.File]::ReadAllText($bare.ModsTxt)
    Assert ($bareMods -match '(?m)^\s*Keybinds\s*:\s*1\s*$') 'UE4SS bootstrap lost its built-in mods.txt authority entries.'
    Equal (ProductEntryCount $bare.ModsTxt) 1 'UE4SS bootstrap did not add exactly one Auto Pickup authority entry.'
    AssertNoPersistentBackup $bare
}

Run 'Owned repair applies confirmed keys, preserves other settings, and rejects stale plans' {
    param($root)
    $f = NewFixture $root Exact
    [void](Install $f $script:Ranges.None)
    $configPath = Join-Path $f.Target 'config.ini'
    $config = Get-Content -LiteralPath $configPath -Raw
    $config = $config -replace '(?mi)^\s*toggle_hotkey\s*=.*$', 'toggle_hotkey = F12'
    $config = $config -replace '(?mi)^\s*debug_logging\s*=.*$', 'debug_logging = true'
    $config += "`r`n; installer-lifecycle-preserve=true`r`n"
    WriteText $configPath $config
    WriteText (Join-Path $f.Mods 'OtherMod\settings.ini') "keep=true`r`n"
    [IO.File]::WriteAllText((Join-Path $f.Target 'enabled.txt'), '1', [Text.UTF8Encoding]::new($false))
    AssertOwnedState (State $f)
    $modsText = Get-Content -LiteralPath $f.ModsTxt -Raw
    if ($modsText -notmatch '(?m)^\s*OtherMod\s*:') {
        WriteText $f.ModsTxt ($modsText.TrimEnd() + "`r`nOtherMod : 1`r`n")
    }
    $plan = InspectPlan $f $script:Ranges.None 'F10' 'K'
    $beforePlan = TreeHash $f.Root
    [void](InspectPlan $f $script:Ranges.None 'INSERT' 'HOME')
    Equal (TreeHash $f.Root) $beforePlan 'Preview / cancellation changed files.'
    foreach ($invalid in @('', 'CTRL+K', "F9`n")) {
        $caught = $null
        try { [void](InspectPlan $f $script:Ranges.None $invalid 'F') } catch { $caught = $_.Exception }
        Assert ($null -ne $caught) 'Invalid selection was accepted.'
        Equal (TreeHash $f.Root) $beforePlan 'Invalid selection changed files.'
    }
    Assert ([bool](Prop $plan 'UpdatesExistingAutoPickup')) 'Owned install was not classified as Upgrade / Repair.'
    $token = [string](Prop $plan 'IdentityToken')
    $stale = $null
    try { [void]$script:InstallConfirmed.Invoke($null, [object[]]@(
        [string]$f.Game, 'INSERT', 'HOME', $script:Ranges.None, $token)) } catch { $stale = $_.Exception }
    Assert ($null -ne $stale) 'Changed selections reused the old confirmation.'
    Equal (TreeHash $f.Root) $beforePlan 'Stale selection rejection changed files.'
    $result = $script:InstallConfirmed.Invoke($null, [object[]]@(
        [string]$f.Game, [string]'F10', [string]'K', $script:Ranges.None, $token))
    $expectedConfig = $config.Replace('toggle_hotkey = F12', 'toggle_hotkey = F10').Replace('interaction_key_fallback=F', 'interaction_key_fallback=K')
    Equal ([IO.File]::ReadAllText($configPath)) $expectedConfig 'Repair changed unrelated configuration bytes or ignored the selected keys.'
    Equal ([string](Prop $result 'ToggleHotkey')) 'F10' 'Result reported the wrong toggle.'
    Equal ([string](Prop (State $f) 'InteractionKeyFallback')) 'K' 'Reinspection did not see the selected fallback.'
    $unchangedHash = Hash $configPath
    [void](Install $f $script:Ranges.None 'F10' 'K')
    Equal (Hash $configPath) $unchangedHash 'Repair with unchanged keys did not preserve exact config bytes.'

    # Exercise the actual form state without showing a window or accepting a dialog.
    Add-Type -AssemblyName System.Windows.Forms
    $formType = $script:Assembly.GetType('DragonSwordNativeAutoPickup.Installer.InstallerForm', $true)
    $form = [Activator]::CreateInstance($formType, $true)
    try {
        $flags = [Reflection.BindingFlags]'Instance,NonPublic'
        $pathControl = $formType.GetField('_gamePath', $flags).GetValue($form)
        $toggleControl = $formType.GetField('_hotkey', $flags).GetValue($form)
        $fallbackControl = $formType.GetField('_interactionKeyFallback', $flags).GetValue($form)
        $refresh = $formType.GetMethod('RefreshInstallationState', $flags)
        $pathControl.Text = $f.Game
        [void]$refresh.Invoke($form, [object[]]@($true))
        Equal $toggleControl.Text 'F10' 'Form did not load the installed toggle.'
        Equal $fallbackControl.Text 'K' 'Form did not load the installed fallback.'
        $toggleControl.Text = 'INSERT'
        $fallbackControl.Text = 'HOME'
        [void]$refresh.Invoke($form, [object[]]@($false))
        Equal $toggleControl.Text 'INSERT' 'Same-path refresh discarded uncommitted keys.'
        Equal $fallbackControl.Text 'HOME' 'Cancel/error refresh discarded the fallback.'
        $otherPath = NewFixture (Join-Path $root 'other-game') Exact
        $pathControl.Text = $otherPath.Game
        [void]$refresh.Invoke($form, [object[]]@($false))
        Equal $toggleControl.Text 'F9' 'Changed game path retained keys from the previous game.'
        Equal $fallbackControl.Text 'F' 'Changed game path retained the previous fallback.'
        $pathControl.Text = $f.Game
        [void]$refresh.Invoke($form, [object[]]@($false))
        Equal $toggleControl.Text 'F10' 'Returning to an installed game did not load its values.'
    } finally { $form.Dispose() }

    $stalePlan = InspectPlan $f $script:Ranges.None 'INSERT' 'HOME'
    WriteText $configPath ($expectedConfig + '; changed after confirmation')
    $afterEdit = TreeHash $f.Root
    $stale = $null
    try { [void]$script:InstallConfirmed.Invoke($null, [object[]]@(
        [string]$f.Game, 'INSERT', 'HOME', $script:Ranges.None, [string](Prop $stalePlan 'IdentityToken'))) } catch { $stale = $_.Exception }
    Assert ($null -ne $stale) 'Changed config reused an old confirmation.'
    Equal (TreeHash $f.Root) $afterEdit 'Stale config rejection changed files.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $f.Target 'enabled.txt'))) 'Owned upgrade did not remove exact legacy enabled.txt.'
    Assert (Test-Path -LiteralPath (Join-Path $f.Mods 'OtherMod\settings.ini')) 'Owned upgrade removed another mod.'
    Assert ((Get-Content -LiteralPath $f.ModsTxt -Raw) -match '(?m)^\s*OtherMod\s*:\s*1\s*$') `
        'Owned upgrade removed another mods.txt entry.'
    Equal (ProductEntryCount $f.ModsTxt) 1 'Owned upgrade duplicated the product mods.txt entry.'
    Assert ([string]::IsNullOrWhiteSpace([string](Prop $result 'BackupDirectory'))) `
        'Owned upgrade reported a persistent backup.'
    AssertNoPersistentBackup $f
    AssertOwnedState (State $f)

    $rollback = NewFixture (Join-Path $root 'rollback') Exact
    [void](Install $rollback $script:Ranges.X3 'HOME' 'K')
    $rollbackConfig = Join-Path $rollback.Target 'config.ini'
    WriteText $rollbackConfig (([IO.File]::ReadAllText($rollbackConfig)) + "; preserve failure settings`r`n")
    $configBefore = Hash $rollbackConfig
    $modBefore = TreeHash $rollback.Target
    $pakBefore = TreeHash $rollback.PakDirectory
    $modsBefore = Hash $rollback.ModsTxt
    $loaderBefore = Hash (Join-Path $rollback.Nested 'UE4SS.dll')
    $failureField = $engine.GetField('IntegrationTestFailurePoint', [Reflection.BindingFlags]'Static,NonPublic')
    $failure = $null
    try {
        $failureField.SetValue($null, 'after-recorded-mutations')
        [void](Install $rollback $script:Ranges.X20 'INSERT' 'Gamepad_FaceButton_Bottom')
    } catch { $failure = ErrorText $_.Exception }
    finally { $failureField.SetValue($null, $null) }
    Assert ($failure -like '*Injected installer test failure*') 'Rollback injection did not reach post-write failure.'
    Equal (Hash $rollbackConfig) $configBefore 'Rollback did not restore config.'
    Equal (TreeHash $rollback.Target) $modBefore 'Rollback did not restore the owned mod.'
    Equal (TreeHash $rollback.PakDirectory) $pakBefore 'Rollback did not restore range PAK.'
    Equal (Hash $rollback.ModsTxt) $modsBefore 'Rollback did not restore mods.txt.'
    Equal (Hash (Join-Path $rollback.Nested 'UE4SS.dll')) $loaderBefore 'Rollback changed runtime.'
}

Run 'Recorded schema-2 1.3.0 upgrades to 1.3.1 and replaces a same-name range PAK' {
    param($root)
    $f = NewFixture $root Exact
    [void](Install $f $script:Ranges.X15)
    $configPath = Join-Path $f.Target 'config.ini'
    $config = (Get-Content -LiteralPath $configPath -Raw) + "`r`n; preserve-130-upgrade=true`r`n"
    WriteText $configPath $config
    WriteText (Join-Path $f.Mods 'OtherMod\settings.ini') "keep=true`r`n"
    $modsText = Get-Content -LiteralPath $f.ModsTxt -Raw
    WriteText $f.ModsTxt ($modsText.TrimEnd() + "`r`nOtherMod : 1`r`n")
    $loaderHash = Hash (Join-Path $f.Nested 'UE4SS.dll')
    $proxyHash = Hash (Join-Path $f.Win64 'dwmapi.dll')
    $recordPath = Join-Path $f.Target 'INSTALL-RECORD.txt'
    $record = Get-Content -LiteralPath $recordPath -Raw
    $record = [regex]::Replace($record, '(?m)^Version:\s*1\.3\.1\s*$', 'Version: 1.3.0')
    Assert ($record -match '(?m)^Version:\s*1\.3\.0\s*$') 'Could not create the recorded 1.3.0 upgrade fixture.'
    WriteText $recordPath $record
    $rangePath = Join-Path $f.PakDirectory $rangeFiles.X15
    WriteText $rangePath 'same-name-1.3.0-range-payload'

    $before = State $f
    Assert ([bool](Prop $before 'IsOwned')) 'Recorded schema-2 1.3.0 fixture was not recognized as owned.'
    Assert ([bool](Prop $before 'CanUpgrade')) 'Recorded schema-2 1.3.0 fixture did not expose Upgrade.'
    Assert (-not [bool](Prop $before 'CanRepair')) 'Recorded schema-2 1.3.0 fixture incorrectly exposed Repair.'
    Equal ([string](Prop $before 'InstalledVersion')) '1.3.0' 'Upgrade fixture reported the wrong version.'

    $operation = Install $f $script:Ranges.X15 'INSERT' 'HOME'
    Assert ([bool](Prop $operation.Plan 'UpdatesExistingAutoPickup')) '1.3.0 fixture was not classified as an upgrade.'
    Equal ([IO.File]::ReadAllText($configPath)) ($config.Replace('toggle_hotkey=F9', 'toggle_hotkey=INSERT').Replace('interaction_key_fallback=F', 'interaction_key_fallback=HOME')) 'Update did not apply only the selected keys.'
    Equal (Hash $rangePath) (HashBytes (GetResourceBytes $rangeResources.X15)) `
        '1.3.0 to 1.3.1 upgrade did not replace the same-name 15x PAK.'
    Assert (Test-Path -LiteralPath (Join-Path $f.Mods 'OtherMod\settings.ini')) `
        '1.3.0 to 1.3.1 upgrade removed another Mod.'
    Assert ((Get-Content -LiteralPath $f.ModsTxt -Raw) -match '(?m)^\s*OtherMod\s*:\s*1\s*$') `
        '1.3.0 to 1.3.1 upgrade removed another mods.txt entry.'
    Equal (Hash (Join-Path $f.Nested 'UE4SS.dll')) $loaderHash 'Upgrade changed UE4SS.dll.'
    Equal (Hash (Join-Path $f.Win64 'dwmapi.dll')) $proxyHash 'Upgrade changed dwmapi.dll.'
    AssertOwnedState (State $f)
    AssertNoPersistentBackup $f
}

Run 'Unknown same-name target is rejected with zero mutation' {
    param($root)
    $f = NewFixture $root Exact
    WriteText (Join-Path $f.Target 'INSTALL-RECORD.txt') (
        'Product: DragonSword Native Auto Pickup' + [char]13 + [char]10 +
        'Version: 1.2.0' + [char]13 + [char]10)
    WriteText (Join-Path $f.Target 'dlls\main.dll') 'FOREIGN DRAGONSWORD_NATIVE_AUTO_PICKUP_1_2_0'
    [IO.File]::WriteAllBytes((Join-Path $f.Target 'config.ini'), (GetResourceBytes 'Payload.DefaultConfig.ini'))
    [void][IO.Directory]::CreateDirectory((Join-Path $f.Target 'Scripts'))
    [IO.File]::WriteAllBytes((Join-Path $f.Target 'Scripts\main.lua'), (GetResourceBytes 'Payload.Main.lua'))
    [IO.File]::WriteAllBytes(
        (Join-Path $f.Target 'THIRD_PARTY_NOTICES.txt'),
        (GetResourceBytes 'Payload.ThirdPartyNotices.txt'))
    $before = TreeHash $root
    AssertUnknownState (State $f)
    $caught = $null
    try { [void](InspectPlan $f $script:Ranges.None) } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'Unknown same-name target was accepted for install or upgrade.'
    Equal (TreeHash $root) $before 'Unknown same-name rejection changed files.'
    AssertNoPersistentBackup $f

    $unknownEnabled = NewFixture (Join-Path $root 'unknown-enabled') Exact
    [void](Install $unknownEnabled $script:Ranges.None)
    WriteText (Join-Path $unknownEnabled.Target 'enabled.txt') 'foreign-enable-control'
    $unknownEnabledBefore = TreeHash $unknownEnabled.Root
    AssertUnknownState (State $unknownEnabled)
    $unknownEnabledError = $null
    try { [void](InspectPlan $unknownEnabled $script:Ranges.None) } catch { $unknownEnabledError = $_.Exception }
    Assert ($null -ne $unknownEnabledError) 'Unknown enabled.txt content was accepted for upgrade.'
    Equal (TreeHash $unknownEnabled.Root) $unknownEnabledBefore 'Unknown enabled.txt rejection changed files.'
    AssertNoPersistentBackup $unknownEnabled

    $tampered = NewFixture (Join-Path $root 'tampered-recorded-payload') Exact
    [void](Install $tampered $script:Ranges.None)
    WriteText (Join-Path $tampered.Target 'dlls\main.dll') 'tampered-after-owned-install'
    $tamperedBefore = TreeHash $tampered.Root
    AssertUnknownState (State $tampered)
    $tamperedError = $null
    try { [void](InspectPlan $tampered $script:Ranges.None) } catch { $tamperedError = $_.Exception }
    Assert ($null -ne $tamperedError) 'A payload changed after recorded install was accepted for overwrite.'
    Equal (TreeHash $tampered.Root) $tamperedBefore 'Recorded-payload rejection changed files.'
    AssertNoPersistentBackup $tampered
}

Run 'Owned uninstall removes product, range, and authority while preserving UE4SS and other mods' {
    param($root)
    $f = NewFixture $root Exact
    [void](Install $f $script:Ranges.X15)
    WriteText (Join-Path $f.Mods 'OtherMod\settings.ini') "keep=true`r`n"
    $modsText = Get-Content -LiteralPath $f.ModsTxt -Raw
    WriteText $f.ModsTxt ($modsText.TrimEnd() + "`r`nOtherMod : 1`r`n")
    $loader = Hash (Join-Path $f.Nested 'UE4SS.dll')
    $proxy = Hash (Join-Path $f.Win64 'dwmapi.dll')
    $settings = Hash (Join-Path $f.Nested 'UE4SS-settings.ini')
    $other = Hash (Join-Path $f.Mods 'OtherMod\settings.ini')
    $state = State $f
    AssertOwnedState $state
    [void](Uninstall $f ([string](Prop $state 'IdentityToken')))
    Assert (-not (Test-Path -LiteralPath $f.Target)) 'Owned product directory remained after uninstall.'
    Equal (ProductEntryCount $f.ModsTxt) 0 'Product mods.txt authority remained after uninstall.'
    Assert ((Get-Content -LiteralPath $f.ModsTxt -Raw) -match '(?m)^\s*OtherMod\s*:\s*1\s*$') `
        'Uninstall removed another mods.txt entry.'
    foreach ($file in $rangeFiles.Values) {
        Assert (-not (Test-Path -LiteralPath (Join-Path $f.PakDirectory $file))) `
            "Owned range PAK remained after uninstall: $file"
    }
    Equal (Hash (Join-Path $f.Nested 'UE4SS.dll')) $loader 'Uninstall changed UE4SS.dll.'
    Equal (Hash (Join-Path $f.Win64 'dwmapi.dll')) $proxy 'Uninstall changed dwmapi.dll.'
    Equal (Hash (Join-Path $f.Nested 'UE4SS-settings.ini')) $settings 'Uninstall changed UE4SS settings.'
    Equal (Hash (Join-Path $f.Mods 'OtherMod\settings.ini')) $other 'Uninstall changed another mod.'
    AssertNoPersistentBackup $f
    AssertFreshState (State $f)
}

Run 'Absent uninstall is disabled and rejected without mutation' {
    param($root)
    $f = NewFixture $root Exact
    $state = State $f
    AssertFreshState $state
    $before = TreeHash $root
    $caught = $null
    try { [void](Uninstall $f 'absent-invalid-identity') } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'Absent product accepted uninstall.'
    Equal (TreeHash $root) $before 'Rejected absent uninstall changed files.'
    AssertNoPersistentBackup $f
}

Run 'Unknown same-name uninstall is disabled and rejected without mutation' {
    param($root)
    $f = NewFixture $root Exact
    WriteText (Join-Path $f.Target 'foreign.bin') 'not-owned-by-this-installer'
    $state = State $f
    AssertUnknownState $state
    $before = TreeHash $root
    $caught = $null
    try { [void](Uninstall $f 'unknown-invalid-identity') } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'Unknown same-name target accepted uninstall.'
    Equal (TreeHash $root) $before 'Rejected unknown uninstall changed files.'
    AssertNoPersistentBackup $f
}

Run 'All range choices are mutually exclusive and exact filenames are product-owned' {
    param($root)
    $f = NewFixture $root Exact
    foreach ($name in @('X3','X5','X10')) {
        [void](Install $f $script:Ranges[$name])
        $selectedPath = Join-Path $f.PakDirectory $rangeFiles[$name]
        Assert (Test-Path -LiteralPath $selectedPath -PathType Leaf) "$name range PAK was not installed."
        Equal (Hash $selectedPath) (HashBytes (GetResourceBytes $rangeResources[$name])) "$name range PAK payload differs."
        $installed = @(Get-ChildItem -LiteralPath $f.PakDirectory -Filter 'DS_PickupRangeX*_P.pak' -File)
        Equal $installed.Count 1 "$name selection left more than one range PAK."
    }
    [void](Install $f $script:Ranges.X15)
    $x15 = Join-Path $f.PakDirectory $rangeFiles.X15
    $x20 = Join-Path $f.PakDirectory $rangeFiles.X20
    Assert (Test-Path -LiteralPath $x15 -PathType Leaf) '15x range PAK was not installed.'
    Equal (Hash $x15) (HashBytes (GetResourceBytes $rangeResources.X15)) '15x range PAK payload differs.'
    foreach ($name in @('X3','X5','X10')) {
        Assert (-not (Test-Path -LiteralPath (Join-Path $f.PakDirectory $rangeFiles[$name]))) "Stale range PAK remains after selecting 15x: $name"
    }
    Assert (-not (Test-Path -LiteralPath $x20)) '20x range PAK was installed with 15x.'

    [void](Install $f $script:Ranges.X20)
    Assert (-not (Test-Path -LiteralPath $x15)) '15x range PAK remained after switching to 20x.'
    Assert (Test-Path -LiteralPath $x20 -PathType Leaf) '20x range PAK was not installed.'
    Equal (Hash $x20) (HashBytes (GetResourceBytes $rangeResources.X20)) '20x range PAK payload differs.'
    foreach ($name in @('X3','X5','X10','X15')) {
        Assert (-not (Test-Path -LiteralPath (Join-Path $f.PakDirectory $rangeFiles[$name]))) `
            "Stale range PAK remains after selecting 20x: $name"
    }

    [void](Install $f $script:Ranges.None)
    foreach ($file in $rangeFiles.Values) {
        Assert (-not (Test-Path -LiteralPath (Join-Path $f.PakDirectory $file))) `
            "Range PAK remains after selecting None: $file"
    }
    AssertNoPersistentBackup $f

    $sameName = NewFixture (Join-Path $root 'same-name-prior-hash') Exact
    [void][IO.Directory]::CreateDirectory($sameName.PakDirectory)
    $sameNameX5 = Join-Path $sameName.PakDirectory $rangeFiles.X5
    WriteText $sameNameX5 'older-same-name-pak'
    [void](Install $sameName $script:Ranges.X5)
    Equal (Hash $sameNameX5) (HashBytes (GetResourceBytes $rangeResources.X5)) `
        'Recognized same-name 5x PAK was not replaced with the embedded release artifact.'
    AssertNoPersistentBackup $sameName

    $legacy = NewFixture (Join-Path $root 'legacy-canary') Exact
    $legacySource = Join-Path (Split-Path -Parent $project) 'DragonSwordPickupRangeExpansion\dist\DS_PickupRangeX3Canary_P.pak'
    Assert (Test-Path -LiteralPath $legacySource -PathType Leaf) 'Known legacy canary fixture is missing.'
    [void][IO.Directory]::CreateDirectory($legacy.PakDirectory)
    [IO.File]::Copy($legacySource, (Join-Path $legacy.PakDirectory 'DS_PickupRangeX3Canary_P.pak'), $false)
    [void](Install $legacy $script:Ranges.None)
    Assert (-not (Test-Path -LiteralPath (Join-Path $legacy.PakDirectory 'DS_PickupRangeX3Canary_P.pak'))) 'Selecting Original did not remove the known owned legacy canary PAK.'
    AssertNoPersistentBackup $legacy

    $priorCanary = NewFixture (Join-Path $root 'prior-canary-hash') Exact
    [void][IO.Directory]::CreateDirectory($priorCanary.PakDirectory)
    $priorCanaryPath = Join-Path $priorCanary.PakDirectory 'DS_PickupRangeX3Canary_P.pak'
    WriteText $priorCanaryPath 'older-canary-pak'
    [void](Install $priorCanary $script:Ranges.None)
    Assert (-not (Test-Path -LiteralPath $priorCanaryPath)) `
        'Recognized legacy canary filename was not removed when selecting Original.'
    AssertNoPersistentBackup $priorCanary

    $junction = NewFixture (Join-Path $root 'pak-junction') Exact
    $outsidePak = Join-Path $junction.Root 'outside-range-target'
    [void][IO.Directory]::CreateDirectory($outsidePak)
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($junction.PakDirectory))
    [void](New-Item -ItemType Junction -Path $junction.PakDirectory -Target $outsidePak)
    try {
        $junctionError = $null
        try { [void](InspectPlan $junction $script:Ranges.X3) } catch { $junctionError = ErrorText $_.Exception }
        Assert ($junctionError -like '*reparse point*') 'A range PAK ancestor junction was not rejected.'
        $outsideEntries = @(Get-ChildItem -LiteralPath $outsidePak -Force)
        Equal $outsideEntries.Count 0 'PAK junction rejection wrote outside the game tree.'
        AssertNoPersistentBackup $junction
    } finally {
        if (Test-Path -LiteralPath $junction.PakDirectory) {
            [IO.Directory]::Delete($junction.PakDirectory, $false)
        }
    }
}

Run 'Other UE4SS layout still converts with persistent rollback backup and migrated mods' {
    param($root)
    $f = NewFixture $root OtherRoot
    $plan = InspectPlan $f $script:Ranges.None
    Assert ([bool](Prop $plan 'ConvertsUE4SS')) 'Other UE4SS layout was not classified for conversion.'
    $token = [string](Prop $plan 'IdentityToken')
    $result = $script:InstallConfirmed.Invoke($null, [object[]]@(
        [string]$f.Game, [string]'F9', [string]'F', $script:Ranges.None, $token))
    $backupPath = [string](Prop $result 'BackupDirectory')
    Assert (-not [string]::IsNullOrWhiteSpace($backupPath)) 'Conversion did not report its persistent backup.'
    $backup = FindPersistentConversionBackup $f
    Equal $backup.FullName $backupPath 'Reported conversion backup differs from the retained backup.'
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'UE4SS.dll')) 'Conversion backup lost root UE4SS.dll.'
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'Mods\UserExample\config.ini')) `
        'Conversion backup lost the original Mod configuration.'
    Assert (Test-Path -LiteralPath (Join-Path $f.Mods 'UserExample\config.ini')) `
        'Conversion did not migrate the existing Mod configuration.'
    Assert (Test-Path -LiteralPath (Join-Path $f.Target 'dlls\main.dll')) 'Conversion did not install Auto Pickup.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $f.Win64 'UE4SS.dll'))) 'Old root UE4SS.dll remained active.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $f.Win64 'Mods'))) 'Old root Mods directory remained active.'
    AssertOwnedState (State $f)

    $fileCollision = NewFixture (Join-Path $root 'file-collision') OtherRoot
    $fileCollisionNestedMods = Join-Path $fileCollision.Nested 'Mods'
    [void][IO.Directory]::CreateDirectory((Join-Path $fileCollisionNestedMods 'UserExample'))
    WriteText (Join-Path $fileCollisionNestedMods 'UserExample\config.ini') 'different=nested'
    [IO.File]::Copy(
        (Join-Path $fileCollision.Win64 'Mods\mods.txt'),
        (Join-Path $fileCollisionNestedMods 'mods.txt'),
        $false)
    $fileCollisionBefore = TreeHash $fileCollision.Root
    $fileCollisionError = $null
    try { [void](InspectPlan $fileCollision $script:Ranges.None) } catch { $fileCollisionError = ErrorText $_.Exception }
    Assert ($fileCollisionError -like '*migration collision*') 'Different same-relative-path Mod files were not rejected.'
    Equal (TreeHash $fileCollision.Root) $fileCollisionBefore 'File migration collision rejection changed files.'
    AssertNoPersistentBackup $fileCollision

    $modsTxtCollision = NewFixture (Join-Path $root 'mods-txt-collision') OtherRoot
    $modsTxtCollisionNestedMods = Join-Path $modsTxtCollision.Nested 'Mods'
    [void][IO.Directory]::CreateDirectory((Join-Path $modsTxtCollisionNestedMods 'UserExample'))
    [IO.File]::Copy(
        (Join-Path $modsTxtCollision.Win64 'Mods\UserExample\config.ini'),
        (Join-Path $modsTxtCollisionNestedMods 'UserExample\config.ini'),
        $false)
    WriteText (Join-Path $modsTxtCollisionNestedMods 'mods.txt') 'NestedOnly : 1'
    $modsTxtCollisionBefore = TreeHash $modsTxtCollision.Root
    $modsTxtCollisionError = $null
    try { [void](InspectPlan $modsTxtCollision $script:Ranges.None) } catch { $modsTxtCollisionError = ErrorText $_.Exception }
    Assert ($modsTxtCollisionError -like '*different mods.txt*') 'Different mixed-layout mods.txt files were not rejected.'
    Equal (TreeHash $modsTxtCollision.Root) $modsTxtCollisionBefore 'mods.txt migration collision rejection changed files.'
    AssertNoPersistentBackup $modsTxtCollision
}

Run 'Conversion backup name collisions use a stable numeric suffix' {
    param($root)
    $f = NewFixture $root OtherRoot
    $label = 'Root-' + (Hash (Join-Path $f.Win64 'UE4SS.dll')).Substring(0, 8)
    $collisionRoots = @()
    foreach ($offset in -5..20) {
        $stamp = [DateTime]::UtcNow.AddSeconds($offset).ToString('yyyyMMdd-HHmmss', [Globalization.CultureInfo]::InvariantCulture)
        $candidate = Join-Path $f.Win64 ("UE4SS-$label-$stamp-Backup")
        [void][IO.Directory]::CreateDirectory($candidate)
        $collisionRoots += $candidate
    }

    $plan = InspectPlan $f $script:Ranges.None
    $token = [string](Prop $plan 'IdentityToken')
    $result = $script:InstallConfirmed.Invoke($null, [object[]]@(
        [string]$f.Game, [string]'F9', [string]'F', $script:Ranges.None, $token))
    $backupPath = [string](Prop $result 'BackupDirectory')

    Assert ($backupPath.EndsWith('_2', [StringComparison]::Ordinal)) `
        "Expected a collision-safe _2 backup suffix; actual path: $backupPath"
    Assert (Test-Path -LiteralPath $backupPath -PathType Container) `
        'The suffixed conversion backup was not retained.'
    Assert (Test-Path -LiteralPath (Join-Path $backupPath 'UE4SS.dll') -PathType Leaf) `
        'The suffixed conversion backup did not preserve UE4SS.dll.'
    $missingCollisions = @($collisionRoots | Where-Object {
        -not (Test-Path -LiteralPath $_ -PathType Container)
    }).Count
    Assert ($missingCollisions -eq 0) 'A pre-existing colliding backup directory was modified or removed.'
}

$passed = @($results | Where-Object status -eq 'PASSED').Count
$failed = @($results | Where-Object status -eq 'FAILED').Count
$clean = $false
if ($KeepArtifacts) {
    $clean = $true
} else {
    try {
        if (Test-Path -LiteralPath $runRoot) { [IO.Directory]::Delete($runRoot, $true) }
        $clean = -not (Test-Path -LiteralPath $runRoot)
    } catch { $clean = $false }
}

[pscustomobject]@{
    expected = $expectedCount
    passed = $passed
    failed = $failed
    skipped = 0
    release_gate = if ($passed -eq $expectedCount -and $failed -eq 0 -and $clean) { 'PASSED' } else { 'FAILED' }
    installer = $script:Installer
    fixtures = if ($KeepArtifacts) { $runRoot } else { '<cleaned>' }
    fixtures_cleaned = $clean
    details = @($results)
}
