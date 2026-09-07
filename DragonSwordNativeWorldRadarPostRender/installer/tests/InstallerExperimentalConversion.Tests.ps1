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

$expectedCount = 20
$product = 'DragonSwordNativeWorldRadarPostRender'
$loaderHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$proxyHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$usmapHash = '0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47'
$settingsHash = '4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74'

function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant() }
function Assert([bool]$Value, [string]$Message) { if (-not $Value) { throw $Message } }
function Equal($Actual, $Expected, [string]$Message) {
    if (-not [object]::Equals($Actual, $Expected)) { throw "$Message Expected=[$Expected] Actual=[$Actual]" }
}
function WriteText([string]$Path, [string]$Text) {
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($Path)))
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}
function Prop($Object, [string]$Name) {
    $property = $Object.GetType().GetProperty($Name, [Reflection.BindingFlags]'Instance, Public, NonPublic')
    if ($null -eq $property) { throw "Missing internal property: $Name" }
    $property.GetValue($Object, $null)
}
function ErrorText([Exception]$Exception) {
    $parts = [Collections.Generic.List[string]]::new()
    while ($null -ne $Exception) { $parts.Add($Exception.Message); $Exception = $Exception.InnerException }
    $parts -join ' | '
}
function NewFixture([string]$Root, [ValidateSet('Bare','Experimental','OtherRoot','Dual')]$Mode) {
    $win64 = Join-Path $Root 'DS\Binaries\Win64'
    $nested = Join-Path $win64 'ue4ss'
    $mods = Join-Path $nested 'Mods'
    [void][IO.Directory]::CreateDirectory($win64)
    $game = Join-Path $win64 'DSClient-Win64-Shipping.exe'
    [IO.File]::Copy($script:Game, $game, $false)
    if ($Mode -in @('Experimental','Dual')) {
        [void][IO.Directory]::CreateDirectory($mods)
        [IO.File]::Copy($script:Loader, (Join-Path $nested 'UE4SS.dll'), $false)
        [IO.File]::Copy($script:Proxy, (Join-Path $win64 'dwmapi.dll'), $false)
        $settings = $script:Assembly.GetManifestResourceStream('Payload.ExperimentalUE4SS-settings.ini')
        try {
            $out = [IO.File]::Create((Join-Path $nested 'UE4SS-settings.ini'))
            try { $settings.CopyTo($out) } finally { $out.Dispose() }
        } finally { $settings.Dispose() }
        WriteText (Join-Path $mods 'mods.txt') ''
    }
    if ($Mode -in @('OtherRoot','Dual')) {
        [IO.File]::WriteAllBytes((Join-Path $win64 'UE4SS.dll'), [byte[]](1,2,3,4,5))
        if (-not (Test-Path -LiteralPath (Join-Path $win64 'dwmapi.dll'))) {
            [IO.File]::WriteAllBytes((Join-Path $win64 'dwmapi.dll'), [byte[]](6,7,8,9))
        }
        $rootMods = Join-Path $win64 'Mods'
        [void][IO.Directory]::CreateDirectory($rootMods)
        WriteText (Join-Path $win64 'UE4SS-settings.ini') "[Overrides]`r`n"
        WriteText (Join-Path $win64 'UE4SS.log') "original stable-root log`r`n"
        WriteText (Join-Path $rootMods 'mods.txt') "ExistingMod : 1`r`n"
    }
    [pscustomobject]@{
        Root=$Root; Win64=$win64; Nested=$nested; Mods=$mods; Game=$game
        Target=(Join-Path $mods $product); RootMods=(Join-Path $win64 'Mods')
    }
}
function Install($Fixture) { $script:Install.Invoke($null, [object[]]@([string]$Fixture.Game)) }
function Inspect($Fixture) { $script:Inspect.Invoke($null, [object[]]@([string]$Fixture.Game)) }
function InspectState($Fixture) {
    $script:InspectState.Invoke($null, [object[]]@([string]$Fixture.Game))
}
function InstallConfirmed($Fixture, [string]$Token) {
    $script:InstallConfirmed.Invoke($null, [object[]]@([string]$Fixture.Game, [string]$Token))
}
function InspectKeys($Fixture, [string[]]$Keys) {
    $script:InspectWithHotkeys.Invoke($null, [object[]]@([string]$Fixture.Game, $Keys[0], $Keys[1], $Keys[2]))
}
function InstallKeys($Fixture, [string[]]$Keys, [string]$Token) {
    $script:InstallWithHotkeys.Invoke($null, [object[]]@([string]$Fixture.Game, $Keys[0], $Keys[1], $Keys[2], $Token))
}
function InspectUninstall($Fixture) {
    $script:InspectUninstall.Invoke($null, [object[]]@([string]$Fixture.Game))
}
function UninstallConfirmed($Fixture, [string]$Token) {
    $script:UninstallConfirmed.Invoke(
        $null,
        [object[]]@([string]$Fixture.Game, [string]$Token))
}
function VerifyLoader($Fixture) {
    Equal (Hash (Join-Path $Fixture.Nested 'UE4SS.dll')) $loaderHash 'Wrong Experimental loader.'
    Equal (Hash (Join-Path $Fixture.Win64 'dwmapi.dll')) $proxyHash 'Wrong Experimental proxy.'
    Equal (Hash (Join-Path $Fixture.Nested 'DS-5.3.2-0+UE5-1c1a1497.usmap')) $usmapHash 'Wrong usmap.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $Fixture.Win64 'UE4SS.dll'))) 'Root UE4SS.dll remained active.'
}
function FindBackup($Fixture) {
    $backups = @(Get-ChildItem -LiteralPath $Fixture.Win64 -Directory -Filter 'UE4SS-*-Backup' |
        Sort-Object Name)
    Assert ($backups.Count -eq 1) "Expected exactly one UE4SS conversion backup; found $($backups.Count)."
    $backups[0]
}
function AssertNoPersistentBackup($Fixture) {
    $backups = @(Get-ChildItem -LiteralPath $Fixture.Win64 -Directory -Force |
        Where-Object {
            $_.Name -like 'UE4SS-*-Backup' -or
            $_.Name -like '.dsnwr-transaction-*' -or
            $_.Name -like '.dsnwr-uninstall-transaction-*'
        })
    $names = @($backups | ForEach-Object { $_.Name }) -join ', '
    Assert ($backups.Count -eq 0) "A successful non-conversion install retained a backup directory: $names"
}
function TreeHash([string]$Root) {
    if (-not (Test-Path -LiteralPath $Root)) { return '<missing>' }
    $items = foreach ($file in Get-ChildItem -LiteralPath $Root -Recurse -Force -File | Sort-Object FullName) {
        "$($file.FullName.Substring($Root.Length))=$(Hash $file.FullName)"
    }
    $items -join "`n"
}
function FlipByte([string]$Path, [long]$Offset) {
    $stream = [IO.File]::Open(
        $Path, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite,
        [IO.FileShare]::None)
    try {
        if ($Offset -lt 0) { $Offset = $stream.Length + $Offset }
        if ($Offset -lt 0 -or $Offset -ge $stream.Length) {
            throw "Fixture byte offset is outside the file: $Offset"
        }
        $stream.Position = $Offset
        $value = $stream.ReadByte()
        $stream.Position = $Offset
        $stream.WriteByte([byte]($value -bxor 1))
        $stream.Flush($true)
    } finally { $stream.Dispose() }
}

$script:Installer = (Resolve-Path -LiteralPath $InstallerExe).Path
$script:Game = (Resolve-Path -LiteralPath $SupportedGameExecutable).Path
$script:Loader = (Resolve-Path -LiteralPath $ExperimentalUE4SSDll).Path
$script:Proxy = (Resolve-Path -LiteralPath $ExperimentalDwmapiDll).Path
$working = (Resolve-Path -LiteralPath $WorkingDirectory).Path
$runRoot = Join-Path $working ('DSNWR-ExperimentalConversion-' + [Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($runRoot)
$script:Assembly = [Reflection.Assembly]::LoadFile($script:Installer)
$engine = $script:Assembly.GetType('DragonSwordNativeWorldRadarPostRender.Installer.InstallerEngine', $true, $false)
$script:Install = $engine.GetMethod('Install', [Reflection.BindingFlags]'Static, NonPublic')
$script:Inspect = $engine.GetMethod('Inspect', [Reflection.BindingFlags]'Static, NonPublic')
$script:InspectState = $engine.GetMethod('InspectInstallationState', [Reflection.BindingFlags]'Static, NonPublic')
$script:InspectWithHotkeys = $engine.GetMethod('InspectWithHotkeys', [Reflection.BindingFlags]'Static, NonPublic')
$script:InstallWithHotkeys = $engine.GetMethod('InstallConfirmedWithHotkeys', [Reflection.BindingFlags]'Static, NonPublic')
$script:InstallConfirmed = $engine.GetMethod('InstallConfirmed', [Reflection.BindingFlags]'Static, NonPublic')
$script:InspectUninstall = $engine.GetMethod('InspectUninstall', [Reflection.BindingFlags]'Static, NonPublic')
$script:UninstallConfirmed = $engine.GetMethod('UninstallConfirmed', [Reflection.BindingFlags]'Static, NonPublic')
Assert ($null -ne $script:InspectUninstall) 'InspectUninstall was not found.'
Assert ($null -ne $script:UninstallConfirmed) 'UninstallConfirmed was not found.'
Assert ($null -ne $script:InspectState) 'InspectInstallationState was not found.'
$failurePoint = $engine.GetField('IntegrationTestFailurePoint', [Reflection.BindingFlags]'Static, NonPublic')
$results = [Collections.Generic.List[object]]::new()
$case = 0

function Run([string]$Name, [scriptblock]$Body) {
    $script:case++
    $root = Join-Path $runRoot ('{0:D2}' -f $script:case)
    [void][IO.Directory]::CreateDirectory($root)
    try {
        & $Body $root
        $script:results.Add([pscustomobject]@{name=$Name;status='PASSED';error=$null})
        Write-Host "PASS $($script:case): $Name"
    } catch {
        $script:results.Add([pscustomobject]@{name=$Name;status='FAILED';error=(ErrorText $_.Exception)})
        Write-Host "FAIL $($script:case): $Name - $(ErrorText $_.Exception)"
    } finally {
        if (Test-Path -LiteralPath $root) { [IO.Directory]::Delete($root, $true) }
    }
}

Run 'Installer embeds only the Experimental runtime contract' {
    param($root)
    $names = @($script:Assembly.GetManifestResourceNames() | Sort-Object)
    $expected = @('Payload.Experimental.usmap','Payload.ExperimentalDwmapi.dll','Payload.ExperimentalRuntime.zip',
        'Payload.ExperimentalUE4SS.dll','Payload.ExperimentalUE4SS-settings.ini','Payload.Manifest.ini','Payload.ThirdPartyNotices.txt') | Sort-Object
    Equal ($names -join '|') ($expected -join '|') 'Embedded resource set differs.'
}
Run 'Updated game hash is accepted by structural preflight' {
    param($root)
    $f = NewFixture $root Experimental
    FlipByte $f.Game -1
    $before = TreeHash $root
    $plan = Inspect $f
    Assert ($null -ne $plan) 'A structurally valid updated game image was rejected.'
    $state = InspectState $f
    Assert ([bool](Prop $state 'CanInstall')) 'Updated game hash did not retain Install availability.'
    Assert (-not [bool](Prop $state 'CanUninstall')) 'Updated game hash enabled Uninstall without an owned Radar.'
    Equal (TreeHash $root) $before 'Updated-game preflight changed files.'
}
Run 'Malformed game image is rejected without mutation' {
    param($root)
    $f = NewFixture $root Experimental
    FlipByte $f.Game 0
    $before = TreeHash $root
    $caught = $null
    try { [void](Inspect $f) } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'A malformed game image was accepted.'
    Assert ((ErrorText $caught) -match 'not a valid x64 PE32\+ image') `
        'Malformed game image failed for an unexpected reason.'
    Equal (TreeHash $root) $before 'Malformed-game preflight changed files.'
}
Run 'Compatible Experimental preflight is read only' {
    param($root)
    $f = NewFixture $root Experimental; $before = TreeHash $root; $plan = Inspect $f
    Assert (-not [bool](Prop $plan 'ConvertsUE4SS')) 'Compatible loader was marked for conversion.'
    Equal (TreeHash $root) $before 'Preflight changed files.'
    WriteText (Join-Path $f.Target 'unknown.bin') 'not-owned'
    $state = InspectState $f
    Assert ([bool](Prop $state 'IsInstalled')) 'Unknown same-name Radar directory was not detected.'
    Assert (-not [bool](Prop $state 'IsOwned')) 'Unknown same-name Radar directory was treated as owned.'
    Assert (-not [bool](Prop $state 'CanInstall')) 'Unknown same-name Radar directory enabled Install.'
    Assert (-not [bool](Prop $state 'CanUpdate')) 'Unknown same-name Radar directory enabled Update.'
    Assert (-not [bool](Prop $state 'CanUninstall')) 'Unknown same-name Radar directory enabled Uninstall.'
}
Run 'Structurally valid changed loader hashes remain update compatible' {
    param($root)
    $f = NewFixture $root Experimental
    FlipByte (Join-Path $f.Nested 'UE4SS.dll') -1
    FlipByte (Join-Path $f.Win64 'dwmapi.dll') -1
    $loaderBefore = Hash (Join-Path $f.Nested 'UE4SS.dll')
    $proxyBefore = Hash (Join-Path $f.Win64 'dwmapi.dll')
    Assert ($loaderBefore -ne $loaderHash) 'Changed loader fixture retained the pinned hash.'
    Assert ($proxyBefore -ne $proxyHash) 'Changed proxy fixture retained the pinned hash.'
    $plan = Inspect $f
    Assert (-not [bool](Prop $plan 'ConvertsUE4SS')) 'A structurally valid changed loader was marked for conversion.'
    [void](Install $f)
    Equal (Hash (Join-Path $f.Nested 'UE4SS.dll')) $loaderBefore 'Update overwrote the existing loader.'
    Equal (Hash (Join-Path $f.Win64 'dwmapi.dll')) $proxyBefore 'Update overwrote the existing proxy.'
    AssertNoPersistentBackup $f
}
Run 'Bare preflight selects pinned Experimental bootstrap' {
    param($root)
    $f = NewFixture $root Bare; $plan = Inspect $f
    Assert ([bool](Prop $plan 'BootstrapsUE4SS')) 'Bare install did not select bootstrap.'
    Assert ((Prop $plan 'ActionDescription') -match 'ExperimentalNested') 'Wrong bootstrap action.'
}
Run 'Other UE4SS preflight requires conversion' {
    param($root)
    $f = NewFixture $root OtherRoot; $before = TreeHash $root; $plan = Inspect $f
    Assert ([bool](Prop $plan 'ConvertsUE4SS')) 'Other loader was not marked for conversion.'
    Equal (TreeHash $root) $before 'Conversion preflight changed files.'
}
Run 'Stale confirmation token is rejected' {
    param($root)
    $f = NewFixture $root Experimental; $before = TreeHash $root; $caught = $null
    try { [void](InstallConfirmed $f ('0' * 64)) } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'Stale token was accepted.'; Equal (TreeHash $root) $before 'Stale token changed files.'
}
Run 'Bare install creates complete Experimental loader and Radar' {
    param($root)
    $f = NewFixture $root Bare; [void](Install $f); VerifyLoader $f
    Assert (Test-Path -LiteralPath (Join-Path $f.Target 'dlls\main.dll')) 'Radar DLL missing.'
    Assert ((Get-Content (Join-Path $f.Mods 'mods.txt') -Raw) -match "$product\s*:\s*1") 'Radar authority missing.'
    Equal (Hash (Join-Path $f.Nested 'UE4SS-settings.ini')) $settingsHash 'Bare install settings differ.'
    $customFixture = NewFixture (Join-Path $root 'custom-keys') Bare
    $keys = @('INSERT','HOME','PAGEUP')
    $before = TreeHash $customFixture.Root
    $plan = InspectKeys $customFixture $keys
    Equal (TreeHash $customFixture.Root) $before 'Custom-key inspection changed a clean target.'
    Equal ([string](Prop (Prop $plan 'Hotkeys') 'Settings')) 'INSERT' 'Plan omitted selected Settings key.'
    $rejected = $false
    try { [void](InstallKeys $customFixture @('F6','F7','F8') ([string](Prop $plan 'IdentityToken'))) } catch { $rejected = $true }
    Assert $rejected 'Changed selected keys were accepted with an old confirmation token.'
    Equal (TreeHash $customFixture.Root) $before 'Rejected selected keys changed a clean target.'
    [void](InstallKeys $customFixture $keys ([string](Prop $plan 'IdentityToken')))
    VerifyLoader $customFixture
    $customState = InspectState $customFixture
    Equal ([string](Prop (Prop $customState 'Hotkeys') 'Enable')) 'HOME' 'Installed keys did not populate UI state.'
    Equal ([string](Prop (Prop $customState 'Hotkeys') 'Disable')) 'PAGEUP' 'Installed disable key was lost.'
}
Run 'Other UE4SS converts and migrates Mods and configuration' {
    param($root)
    $f = NewFixture $root OtherRoot
    WriteText (Join-Path $f.RootMods 'ExistingMod\config.ini') "value=kept`r`n"
    $longBase = Join-Path $f.RootMods 'LongConfig'
    $longNameLength = [Math]::Max(20, 205 - $longBase.Length - 5)
    $longConfig = Join-Path $longBase ((('x' * $longNameLength)) + '.ini')
    WriteText $longConfig "long=value`r`n"
    [void](Install $f); VerifyLoader $f
    Equal (Get-Content (Join-Path $f.Mods 'ExistingMod\config.ini') -Raw) "value=kept`r`n" 'Mod config was not migrated.'
    Equal (Get-Content (Join-Path $f.Mods ('LongConfig\' + [IO.Path]::GetFileName($longConfig))) -Raw) "long=value`r`n" 'Long-path Mod config was not migrated.'
    Assert ((Get-Content (Join-Path $f.Mods 'mods.txt') -Raw) -match 'ExistingMod\s*:\s*1') 'Existing mods.txt entry was lost.'
    Assert (-not (Test-Path -LiteralPath $f.RootMods)) 'Old root Mods layout remained active after conversion.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $f.Win64 'UE4SS-settings.ini'))) 'Old root settings remained active.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $f.Win64 'UE4SS.log'))) 'Old root UE4SS log remained active.'
    $backup = FindBackup $f
    $snapshot = $backup.FullName
    Equal (Get-Content (Join-Path $snapshot 'Mods\ExistingMod\config.ini') -Raw) "value=kept`r`n" 'Complete backup lost Mod config.'
    Equal (Get-Content (Join-Path $snapshot 'UE4SS-settings.ini') -Raw) "[Overrides]`r`n" 'Complete backup lost root settings.'
    Equal (Get-Content (Join-Path $snapshot 'UE4SS.log') -Raw) "original stable-root log`r`n" 'Complete backup lost root UE4SS log.'
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'COMPLETE-UE4SS-BACKUP.txt')) 'Verified backup manifest missing.'
}
Run 'Conversion keeps rollback files and complete original-layout backup' {
    param($root)
    $f = NewFixture $root OtherRoot; [void](Install $f)
    $backup = FindBackup $f
    Assert ($null -ne $backup) 'Backup directory missing.'
    Assert (@(Get-ChildItem -LiteralPath (Join-Path $backup.FullName '.rollback') -File).Count -gt 0) 'Short-name transaction rollback copies are missing.'
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'UE4SS.dll')) 'Complete root loader backup missing.'
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'dwmapi.dll')) 'Complete proxy backup missing.'
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'Mods\mods.txt')) 'Complete root Mods backup missing.'
}
Run 'Dual layout converts when Mod files do not conflict' {
    param($root)
    $f = NewFixture $root Dual
    WriteText (Join-Path $f.RootMods 'RootMod\config.ini') 'root=1'
    WriteText (Join-Path $f.Nested 'old-loader-only.txt') 'remove me'
    [void](Install $f); VerifyLoader $f
    Assert (Test-Path -LiteralPath (Join-Path $f.Mods 'RootMod\config.ini')) 'Root Mod was not migrated.'
    Assert (-not (Test-Path -LiteralPath (Join-Path $f.Nested 'old-loader-only.txt'))) 'Old nested loader file remained active.'
    Assert (-not (Test-Path -LiteralPath $f.RootMods)) 'Old root Mods layout remained active after dual conversion.'
    $backup = FindBackup $f
    Assert (Test-Path -LiteralPath (Join-Path $backup.FullName 'ue4ss\old-loader-only.txt')) 'Complete nested UE4SS backup missing.'
}
Run 'Different duplicate Mod files stop before mutation' {
    param($root)
    $f = NewFixture $root Dual
    WriteText (Join-Path $f.RootMods 'Shared\config.ini') 'root'
    WriteText (Join-Path $f.Mods 'Shared\config.ini') 'nested'
    $before = TreeHash $root; $caught = $null
    try { [void](Inspect $f) } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'Conflicting duplicate files were accepted.'
    Equal (TreeHash $root) $before 'Conflict preflight changed files.'
}
Run 'Exact Experimental custom Mods override remains authoritative' {
    param($root)
    $f = NewFixture $root Experimental
    $custom = Join-Path $f.Win64 'CustomMods'; [void][IO.Directory]::CreateDirectory($custom)
    WriteText (Join-Path $custom 'mods.txt') ''
    $settings = Get-Content (Join-Path $f.Nested 'UE4SS-settings.ini') -Raw
    $settings = $settings -replace '(?m)^ModsFolderPath[^\n]*', "ModsFolderPath = CustomMods`r"
    WriteText (Join-Path $f.Nested 'UE4SS-settings.ini') $settings
    $plan = Inspect $f
    $plannedModDirectory = [string](Prop $plan 'ModDirectory')
    Assert (-not [bool](Prop $plan 'ConvertsUE4SS')) 'Compatible Experimental layout was incorrectly marked for conversion.'
    Assert ($plannedModDirectory.StartsWith($custom, [StringComparison]::OrdinalIgnoreCase)) (
        "Custom Mods directory was not selected during preflight. Actual: " + $plannedModDirectory)
    $result = Install $f
    $actualModDirectory = [string](Prop $result 'ModDirectory')
    Assert ($actualModDirectory.StartsWith($custom, [StringComparison]::OrdinalIgnoreCase)) (
        "Custom Mods directory was not retained. Actual: " + $actualModDirectory)
}
function Test-UppercaseLegacyVisibilityKeys {
    param($root)
    $variants = @(
        [pscustomobject]@{
            Name = 'uppercase-schema'
            Text = "SCHEMA_VERSION=4`r`ncompact_mask=127`r`nworld_mask=62`r`n" +
                "area_quest_mode=available`r`nassault_mode=available`r`n"
        },
        [pscustomobject]@{
            Name = 'uppercase-mask'
            Text = "schema_version=4`r`nCOMPACT_MASK=127`r`nworld_mask=62`r`n" +
                "area_quest_mode=available`r`nassault_mode=available`r`n"
        }
    )
    foreach ($variant in $variants) {
        $f = NewFixture (Join-Path $root $variant.Name) Experimental
        [void](Install $f)
        WriteText (Join-Path $f.Target 'config\visibility.ini') $variant.Text
        $before = TreeHash $f.Root
        $caught = $null
        try { [void](Install $f) } catch { $caught = $_.Exception }
        Assert ($null -ne $caught) (
            "Uppercase legacy visibility key was accepted: " + $variant.Name)
        Assert ((ErrorText $caught) -match 'missing or unsupported setting') (
            "Uppercase legacy visibility key failed for an unexpected reason: " +
            (ErrorText $caught))
        Equal (TreeHash $f.Root) $before (
            "Rejected uppercase legacy visibility changed the fixture: " + $variant.Name)
    }
}
Run 'Update repair preserves user settings and refreshes bundled catalogs without backup' {
    param($root)
    Test-UppercaseLegacyVisibilityKeys (Join-Path $root 'uppercase-key-validation')
    $f = NewFixture $root Experimental
    [void](Install $f)
    $invalidVisibility = "schema_version=3`r`ncompact_mask=7`r`nworld_mask=10`r`narea_quest_mode=hidden`r`n"
    WriteText (Join-Path $f.Target 'config\visibility.ini') $invalidVisibility
    $invalidBefore = TreeHash $root
    $invalidError = $null
    try { [void](Install $f) } catch { $invalidError = $_.Exception }
    Assert ($null -ne $invalidError) 'An unsupported area_quest_mode was accepted.'
    $invalidErrorText = ErrorText $invalidError
    Assert ($invalidErrorText -match "area_quest_mode must be exactly 'available' or 'all'") `
        ("Unsupported area_quest_mode failed for an unexpected reason: " + $invalidErrorText)
    Equal (TreeHash $root) $invalidBefore `
        'Rejected area_quest_mode changed the installed tree.'

    $missingAssaultMode = "schema_version=4`r`ncompact_mask=7`r`nworld_mask=10`r`narea_quest_mode=all`r`n"
    WriteText (Join-Path $f.Target 'config\visibility.ini') $missingAssaultMode
    $missingAssaultBefore = TreeHash $root
    $missingAssaultError = $null
    try { [void](Install $f) } catch { $missingAssaultError = $_.Exception }
    Assert ($null -ne $missingAssaultError) 'Visibility schema 4 without assault_mode was accepted.'
    Assert ((ErrorText $missingAssaultError) -match 'Visibility schema 4 requires exactly one assault_mode') `
        'Missing schema 4 assault_mode failed for an unexpected reason.'
    Equal (TreeHash $root) $missingAssaultBefore `
        'Rejected schema 4 without assault_mode changed the installed tree.'

    $missingAreaQuestMode = "schema_version=4`r`ncompact_mask=7`r`nworld_mask=10`r`nassault_mode=all`r`n"
    WriteText (Join-Path $f.Target 'config\visibility.ini') $missingAreaQuestMode
    $missingAreaBefore = TreeHash $root
    $missingAreaError = $null
    try { [void](Install $f) } catch { $missingAreaError = $_.Exception }
    Assert ($null -ne $missingAreaError) 'Visibility schema 4 without area_quest_mode was accepted.'
    Assert ((ErrorText $missingAreaError) -match 'Visibility schemas 3 and 4 require exactly one area_quest_mode') `
        'Missing schema 4 area_quest_mode failed for an unexpected reason.'
    Equal (TreeHash $root) $missingAreaBefore `
        'Rejected schema 4 without area_quest_mode changed the installed tree.'

    $invalidAssaultMode = "schema_version=4`r`ncompact_mask=7`r`nworld_mask=10`r`narea_quest_mode=all`r`nassault_mode=future`r`n"
    WriteText (Join-Path $f.Target 'config\visibility.ini') $invalidAssaultMode
    $invalidAssaultBefore = TreeHash $root
    $invalidAssaultError = $null
    try { [void](Install $f) } catch { $invalidAssaultError = $_.Exception }
    Assert ($null -ne $invalidAssaultError) 'An unsupported assault_mode was accepted.'
    Assert ((ErrorText $invalidAssaultError) -match "assault_mode must be exactly 'available' or 'all'") `
        'Unsupported assault_mode failed for an unexpected reason.'
    Equal (TreeHash $root) $invalidAssaultBefore `
        'Rejected assault_mode changed the installed tree.'

    $visibility = "schema_version=4`r`ncompact_mask=7`r`nworld_mask=10`r`narea_quest_mode=all`r`nassault_mode=all`r`n"
    $diagnostics = "event_log_enabled=true`r`n"
    $treasureOverrides = "# User treasure exclusions`r`nignore 11230106`r`nignore 12345678`r`n"
    WriteText (Join-Path $f.Target 'config\visibility.ini') $visibility
    WriteText (Join-Path $f.Target 'config\diagnostics.ini') $diagnostics
    foreach ($badHotkeys in @(
        "[hotkeys]`nsettings_hotkey=Insert`nenable_hotkey=Home`ndisable_hotkey=HOME`n",
        "[hotkeys]`nsettings_hotkey=F25`nenable_hotkey=Home`ndisable_hotkey=PageUp`n",
        "[hotkeys]`nsettings_hotkey=Insert`nenable_hotkey=Home`n",
        ('#' + ('x' * 4096)))) {
        WriteText (Join-Path $f.Target 'config\hotkeys.ini') $badHotkeys
        $hotkeyBefore = TreeHash $root
        $hotkeyError = $null
        try { [void](Install $f) } catch { $hotkeyError = $_.Exception }
        Assert ($null -ne $hotkeyError) 'Invalid hotkeys were accepted.'
        Assert ((ErrorText $hotkeyError) -match 'hotkeys.ini') 'Unexpected hotkey validation failure.'
        Equal (TreeHash $root) $hotkeyBefore 'Rejected hotkeys changed the installed tree.'
        $invalidState = InspectState $f
        Assert (-not [bool](Prop $invalidState 'CanUpdate')) 'Invalid existing hotkeys enabled GUI Repair.'
        Assert ([bool](Prop $invalidState 'CanUninstall')) 'Invalid hotkeys blocked safe uninstall of an otherwise owned Mod.'
    }
    $hotkeys = "# Keep my bindings`r`n[hotkeys]`r`nsettings_hotkey=Insert`r`nenable_hotkey=Home`r`ndisable_hotkey=PageUp`r`n"
    WriteText (Join-Path $f.Target 'config\hotkeys.ini') $hotkeys
    WriteText (Join-Path $f.Target 'data\defaults\treasure_overrides.txt') $treasureOverrides
    $catalog = Join-Path $f.Target 'data\generated\treasures.lua'
    [IO.File]::SetLastWriteTimeUtc($catalog, [DateTime]::Parse('2000-01-01T00:00:00Z').ToUniversalTime())
    $plan = Inspect $f
    Assert ([bool](Prop $plan 'UpdatesExistingRadar')) 'Owned existing Radar was not classified as Update / Repair.'
    $state = InspectState $f
    Assert ([bool](Prop $state 'CanUpdate')) 'Owned current Radar did not enable Repair.'
    Assert ([bool](Prop $state 'CanUninstall')) 'Owned current Radar did not enable Uninstall.'
    Equal ([string](Prop $state 'InstalledVersion')) '2.3.0' 'Current Radar version was not detected.'
    $result = Install $f
    Assert ([bool](Prop $result 'UpdatedExistingRadar')) 'Install result did not report Update / Repair.'
    Equal (Get-Content (Join-Path $f.Target 'config\visibility.ini') -Raw) $visibility 'Visibility settings were overwritten.'
    Equal (Get-Content (Join-Path $f.Target 'config\diagnostics.ini') -Raw) $diagnostics 'Diagnostics settings were overwritten.'
    Equal (Get-Content (Join-Path $f.Target 'config\hotkeys.ini') -Raw) $hotkeys 'Hotkey settings were overwritten.'
    Equal (Get-Content (Join-Path $f.Target 'data\defaults\treasure_overrides.txt') -Raw) $treasureOverrides 'Treasure ignores were overwritten.'
    Assert ([IO.File]::GetLastWriteTimeUtc($catalog).Year -ne 2000) 'Bundled treasure catalog was not refreshed.'
    Assert ([string]::IsNullOrEmpty([string](Prop $result 'BackupDirectory'))) 'Update / Repair reported a persistent backup.'
    AssertNoPersistentBackup $f

    # Exercise the same explicit-selection API used by the GUI, not only the
    # legacy preserve-settings entry point. Unchanged selections preserve bytes.
    $unchangedKeys = @('INSERT','HOME','PAGEUP')
    $plan = InspectKeys $f $unchangedKeys
    [void](InstallKeys $f $unchangedKeys ([string](Prop $plan 'IdentityToken')))
    $keysPath = Join-Path $f.Target 'config\hotkeys.ini'
    Equal (Get-Content -LiteralPath $keysPath -Raw) $hotkeys 'Unchanged GUI selection reformatted hotkeys.'
    $before = TreeHash $root
    $caught = $null
    try { [void](InspectKeys $f @('HOME','home','F8')) } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'Duplicate GUI keys were accepted.'
    Equal (TreeHash $root) $before 'Rejected GUI keys mutated the fixture.'

    $newKeys = @('F6','F7','F8')
    $plan = InspectKeys $f $newKeys
    WriteText $keysPath ($hotkeys + "# edited after confirmation`r`n")
    $before = TreeHash $root
    $caught = $null
    try { [void](InstallKeys $f $newKeys ([string](Prop $plan 'IdentityToken'))) } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'An externally changed hotkey file did not invalidate confirmation.'
    Assert ((ErrorText $caught) -match 'changed after') 'Stale hotkey confirmation failed for an unexpected reason.'
    Equal (TreeHash $root) $before 'Stale hotkey confirmation mutated the fixture.'
    WriteText $keysPath $hotkeys

    # Failed transactions intentionally keep their recovery journal. Isolate
    # that fixture so successful repair's no-persistent-backup check stays exact.
    $rollbackFixture = NewFixture (Join-Path $root 'hotkey-rollback') Experimental
    $plan = InspectKeys $rollbackFixture $unchangedKeys
    [void](InstallKeys $rollbackFixture $unchangedKeys ([string](Prop $plan 'IdentityToken')))
    $plan = InspectKeys $rollbackFixture $newKeys
    $beforeTarget = TreeHash $rollbackFixture.Target
    $beforeMods = Get-Content -LiteralPath (Join-Path $rollbackFixture.Mods 'mods.txt') -Raw
    $failurePoint.SetValue($null, 'after-recorded-mutations')
    $caught = $null
    try { [void](InstallKeys $rollbackFixture $newKeys ([string](Prop $plan 'IdentityToken'))) }
    catch { $caught = $_.Exception } finally { $failurePoint.SetValue($null, $null) }
    Assert ($null -ne $caught) 'Injected hotkey repair failure was not triggered.'
    Equal (TreeHash $rollbackFixture.Target) $beforeTarget 'Repair rollback did not restore old hotkeys and other Radar files.'
    Equal (Get-Content -LiteralPath (Join-Path $rollbackFixture.Mods 'mods.txt') -Raw) $beforeMods 'Repair rollback changed mods.txt.'

    $plan = InspectKeys $f $newKeys
    [void](InstallKeys $f $newKeys ([string](Prop $plan 'IdentityToken')))
    $expectedKeys = $hotkeys.Replace('=Insert','=F6').Replace('=Home','=F7').Replace('=PageUp','=F8')
    Equal (Get-Content -LiteralPath $keysPath -Raw) $expectedKeys 'Repair did not apply the selected keys or preserve comments.'
    Equal (Get-Content (Join-Path $f.Target 'config\visibility.ini') -Raw) $visibility 'Key repair changed visibility.'
    Equal (Get-Content (Join-Path $f.Target 'config\diagnostics.ini') -Raw) $diagnostics 'Key repair changed diagnostics.'
    Equal (Get-Content (Join-Path $f.Target 'data\defaults\treasure_overrides.txt') -Raw) $treasureOverrides 'Key repair changed treasure ignores.'
    $state = InspectState $f
    Equal ([string](Prop (Prop $state 'Hotkeys') 'Settings')) 'F6' 'Reopened Setup did not read the repaired key.'
    AssertNoPersistentBackup $f
}
Run 'Older structurally owned Radar version is accepted for update' {
    param($root)
    $f = NewFixture $root Experimental
    [void](Install $f)
    $releasePath = Join-Path $f.Target 'metadata\release.json'
    $manifestPath = Join-Path $f.Target 'metadata\package-manifest.json'
    $recordPath = Join-Path $f.Target 'INSTALL-RECORD.txt'
    $release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
    $release.version = '2.1.1'
    WriteText $releasePath ($release | ConvertTo-Json -Depth 100)
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.version = '2.1.1'
    $releaseEntry = @($manifest.files | Where-Object path -eq 'metadata/release.json')
    Equal $releaseEntry.Count 1 'Release manifest entry is not unique.'
    $releaseEntry[0].size = (Get-Item -LiteralPath $releasePath).Length
    $releaseEntry[0].sha256 = Hash $releasePath
    WriteText $manifestPath ($manifest | ConvertTo-Json -Depth 100)
    $record = (Get-Content -LiteralPath $recordPath -Raw) -replace 'Version: 2\.3\.0', 'Version: 2.1.1'
    WriteText $recordPath $record
    # Pre-2.3 installations have no hotkey file. Show defaults, then allow an
    # explicit custom selection on Update rather than silently ignoring it.
    [IO.File]::Delete((Join-Path $f.Target 'config\hotkeys.ini'))
    $state = InspectState $f
    Assert ([bool](Prop $state 'CanUpdate')) 'Owned older Radar did not enable Update.'
    Assert ([bool](Prop $state 'CanUninstall')) 'Owned older Radar did not enable Uninstall.'
    Equal ([string](Prop $state 'InstalledVersion')) '2.1.1' 'Older Radar version was not detected.'
    Equal ([string](Prop (Prop $state 'Hotkeys') 'Settings')) 'F6' 'Missing old config did not show default keys.'
    $upgradeKeys = @('INSERT','HOME','PAGEUP')
    $plan = InspectKeys $f $upgradeKeys
    Assert ([bool](Prop $plan 'UpdatesExistingRadar')) 'Older owned Radar was not accepted for Update / Repair.'
    [void](InstallKeys $f $upgradeKeys ([string](Prop $plan 'IdentityToken')))
    $updatedRelease = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
    Equal ([string]$updatedRelease.version) '2.3.0' 'Update did not restore the current release version.'
    $upgradedKeys = Get-Content (Join-Path $f.Target 'config\hotkeys.ini') -Raw
    Assert ($upgradedKeys -match '(?m)^settings_hotkey=INSERT\r?$' -and
        $upgradedKeys -match '(?m)^enable_hotkey=HOME\r?$' -and
        $upgradedKeys -match '(?m)^disable_hotkey=PAGEUP\r?$') 'Upgrade did not apply selected hotkeys.'
    AssertNoPersistentBackup $f
}
Run 'Injected late failure restores converted loader and Mods state' {
    param($root)
    $f = NewFixture $root OtherRoot; WriteText (Join-Path $f.RootMods 'ExistingMod\config.ini') 'keep'
    $before = TreeHash $root; $failurePoint.SetValue($null, 'after-recorded-mutations'); $caught = $null
    try { [void](Install $f) } catch { $caught = $_.Exception } finally { $failurePoint.SetValue($null, $null) }
    Assert ($null -ne $caught) 'Injected failure did not fail.'
    $afterWithoutBackups = @((Get-ChildItem $root -Recurse -Force -File | Where-Object { $_.FullName -notmatch '\\UE4SS-[^\\]+-Backup\\' } | Sort-Object FullName) |
        ForEach-Object { "$($_.FullName.Substring($root.Length))=$(Hash $_.FullName)" }) -join "`n"
    $beforeWithoutBackups = @((($before -split "`n") | Where-Object { $_ -notmatch '\\UE4SS-[^\\]+-Backup\\' })) -join "`n"
    Equal $afterWithoutBackups $beforeWithoutBackups 'Rollback did not restore the original tree.'
}
Run 'Confirmed uninstall removes only strictly owned Radar state' {
    param($root)
    $f = NewFixture $root Experimental
    [void](Install $f)
    WriteText (Join-Path $f.Mods 'UnrelatedMod\config.ini') "keep=true`r`n"
    $modsText = Get-Content -LiteralPath (Join-Path $f.Mods 'mods.txt') -Raw
    $modsNewLine = if ($modsText.Contains("`r`n")) { "`r`n" } else { "`n" }
    WriteText (Join-Path $f.Mods 'mods.txt') ($modsText + "UnrelatedMod : 1" + $modsNewLine)
    $saved = Join-Path $root 'DS\Saved\SaveGames\slot.sav'
    WriteText $saved 'preserve-save'
    $loaderBefore = Hash (Join-Path $f.Nested 'UE4SS.dll')
    $proxyBefore = Hash (Join-Path $f.Win64 'dwmapi.dll')

    $plan = InspectUninstall $f
    Equal ([string](Prop $plan 'ModDirectory')) $f.Target `
        'Uninstall plan selected the wrong Radar directory.'
    $result = UninstallConfirmed $f ([string](Prop $plan 'IdentityToken'))

    Assert (-not (Test-Path -LiteralPath $f.Target)) `
        'Confirmed uninstall retained the Radar directory.'
    $updatedMods = Get-Content -LiteralPath (Join-Path $f.Mods 'mods.txt') -Raw
    Assert ($updatedMods -notmatch [regex]::Escape($product)) `
        'Confirmed uninstall retained the Radar mods.txt authority.'
    Assert ($updatedMods -match 'UnrelatedMod\s*:\s*1') `
        'Confirmed uninstall removed unrelated mods.txt content.'
    Equal (Get-Content -LiteralPath (Join-Path $f.Mods 'UnrelatedMod\config.ini') -Raw) `
        "keep=true`r`n" 'Confirmed uninstall changed an unrelated Mod.'
    Equal (Get-Content -LiteralPath $saved -Raw) 'preserve-save' `
        'Confirmed uninstall changed the game save.'
    Equal (Hash (Join-Path $f.Nested 'UE4SS.dll')) $loaderBefore `
        'Confirmed uninstall changed UE4SS.dll.'
    Equal (Hash (Join-Path $f.Win64 'dwmapi.dll')) $proxyBefore `
        'Confirmed uninstall changed the UE4SS proxy.'
    Equal ([string](Prop $result 'ModsTxtPath')) (Join-Path $f.Mods 'mods.txt') `
        'Uninstall result reported the wrong mods.txt.'
    $state = InspectState $f
    Assert ([bool](Prop $state 'CanInstall')) 'Uninstall did not restore Install availability.'
    Assert (-not [bool](Prop $state 'CanUninstall')) 'Uninstall remained enabled after removal.'
    AssertNoPersistentBackup $f
}
Run 'Stale uninstall confirmation is rejected without mutation' {
    param($root)
    $f = NewFixture $root Experimental
    [void](Install $f)
    $plan = InspectUninstall $f
    $modsPath = Join-Path $f.Mods 'mods.txt'
    $modsText = Get-Content -LiteralPath $modsPath -Raw
    $modsNewLine = if ($modsText.Contains("`r`n")) { "`r`n" } else { "`n" }
    WriteText $modsPath ($modsText + "LateMod : 0" + $modsNewLine)
    $before = TreeHash $root
    $caught = $null
    try {
        [void](UninstallConfirmed $f ([string](Prop $plan 'IdentityToken')))
    } catch { $caught = $_.Exception }
    Assert ($null -ne $caught) 'A stale uninstall confirmation was accepted.'
    Assert ((ErrorText $caught) -match 'changed after the uninstall prompt') `
        'Stale uninstall confirmation failed for an unexpected reason.'
    Equal (TreeHash $root) $before `
        'Rejected stale uninstall changed the installation.'
}
Run 'Injected uninstall failure restores Radar and mods.txt' {
    param($root)
    $f = NewFixture $root Experimental
    [void](Install $f)
    $plan = InspectUninstall $f
    $beforeTarget = TreeHash $f.Target
    $beforeMods = Get-Content -LiteralPath (Join-Path $f.Mods 'mods.txt') -Raw
    $failurePoint.SetValue($null, 'after-uninstall-mutations')
    $caught = $null
    try {
        [void](UninstallConfirmed $f ([string](Prop $plan 'IdentityToken')))
    } catch { $caught = $_.Exception } finally {
        $failurePoint.SetValue($null, $null)
    }
    Assert ($null -ne $caught) 'Injected uninstall failure did not fail.'
    Equal (TreeHash $f.Target) $beforeTarget `
        'Uninstall rollback did not restore the Radar directory.'
    Equal (Get-Content -LiteralPath (Join-Path $f.Mods 'mods.txt') -Raw) `
        $beforeMods 'Uninstall rollback did not restore mods.txt.'
}

$passed = @($results | Where-Object status -eq 'PASSED').Count
$failed = @($results | Where-Object status -eq 'FAILED').Count
try { [IO.Directory]::Delete($runRoot, $true); $clean = -not (Test-Path -LiteralPath $runRoot) } catch { $clean = $false }
[pscustomobject]@{
    expected=$expectedCount; passed=$passed; failed=$failed; skipped=0
    release_gate=if($passed -eq $expectedCount -and $failed -eq 0 -and $clean){'PASSED'}else{'FAILED'}
    sources_unchanged=$true; fixtures_cleaned=$clean; details=@($results)
}
