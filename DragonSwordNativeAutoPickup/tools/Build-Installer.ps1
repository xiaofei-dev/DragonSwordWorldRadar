[CmdletBinding()]
param(
    [string]$ExperimentalPluginDll,
    [Parameter(Mandatory)][string]$ExperimentalUE4SSRuntimeZip,
    [string]$PickupRangeX3Pak,
    [string]$PickupRangeX5Pak,
    [string]$PickupRangeX10Pak,
    [string]$PickupRangeX15Pak,
    [string]$PickupRangeX20Pak,
    [string]$ConfigPath,
    [string]$LuaScriptPath,
    [string]$ThirdPartyNoticesPath,
    [string]$OutputDirectory,
    [string]$CscPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$installerRoot = Join-Path $projectRoot 'installer'
$version = '1.3.0'
$runtimeZipHash = 'AB765EF93BD0DB109D7224C0E2487C68A1CE8748F20B597128D43E247B4AFA77'
$experimentalUE4SSDllHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$experimentalDwmapiHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$experimentalUsmapHash = '0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47'
$experimentalSettingsHash = '4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74'
$approvedRangeX3PakHash = '6BB99A1E35C06EB0284370B9D7BD2F34E90CB6DCA7479CF10A477C68EA0103E8'
$approvedRangeX5PakHash = 'DB9E129D8F8FCCA025864EC908C13C70F950AD779C37CF13A41164476587CECD'
$approvedRangeX10PakHash = '6A1ADB7592BA0C70A17984DB3AC01348086AABE196F0FDAF914B3F52C7A395F1'
$approvedRangeX15PakHash = '16CA8F2353D40BCED8ACBBC95FE4CE8A57E304DEB76D758517E716FF43740100'
$approvedRangeX20PakHash = 'C10E1B252849B1D5DE5C94F468B60841E412487055AB7115B2E73E0D50AFF9BA'

if (-not $ExperimentalPluginDll) { $ExperimentalPluginDll = Join-Path $projectRoot 'out\native\ExperimentalNested\main.dll' }
$rangeProjectDist = Join-Path (Split-Path -Parent $projectRoot) 'DragonSwordPickupRangeExpansion\dist'
if (-not $PickupRangeX3Pak) { $PickupRangeX3Pak = Join-Path $rangeProjectDist 'DS_PickupRangeX3_P.pak' }
if (-not $PickupRangeX5Pak) { $PickupRangeX5Pak = Join-Path $rangeProjectDist 'DS_PickupRangeX5_P.pak' }
if (-not $PickupRangeX10Pak) { $PickupRangeX10Pak = Join-Path $rangeProjectDist 'DS_PickupRangeX10_P.pak' }
if (-not $PickupRangeX15Pak) { $PickupRangeX15Pak = Join-Path $rangeProjectDist 'DS_PickupRangeX15_P.pak' }
if (-not $PickupRangeX20Pak) { $PickupRangeX20Pak = Join-Path $rangeProjectDist 'DS_PickupRangeX20_P.pak' }
if (-not $ConfigPath) { $ConfigPath = Join-Path $projectRoot 'config\default.ini' }
if (-not $LuaScriptPath) { $LuaScriptPath = Join-Path $projectRoot 'Scripts\main.lua' }
if (-not $ThirdPartyNoticesPath) { $ThirdPartyNoticesPath = Join-Path $installerRoot 'THIRD_PARTY_NOTICES.txt' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'out\installer\1.3.0' }

function Resolve-RequiredFile {
    param([string]$Path, [string]$Description)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToUpperInvariant()
}

function Assert-X64PeDll {
    param([string]$Path, [string]$Description)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 256 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
        throw "$Description is not a PE image: $Path"
    }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    if ($peOffset -lt 0 -or $peOffset + 24 -gt $bytes.Length -or
        $bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
        $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
        throw "$Description has an invalid PE header: $Path"
    }
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    $characteristics = [BitConverter]::ToUInt16($bytes, $peOffset + 22)
    if ($machine -ne 0x8664 -or ($characteristics -band 0x2000) -eq 0) {
        throw "$Description must be an x64 DLL: $Path"
    }
    $ascii = [Text.Encoding]::ASCII.GetString($bytes)
    if (-not $ascii.Contains('DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0')) {
        throw "$Description does not contain the 1.3.0 release identity marker: $Path"
    }
}

function Get-ZipEntrySha256 {
    param([string]$ZipPath, [string]$EntryName)
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if (-not $entry) { throw "Archive entry was not found: $EntryName" }
        $stream = $entry.Open()
        try {
            $sha = [Security.Cryptography.SHA256]::Create()
            try { return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '') }
            finally { $sha.Dispose() }
        }
        finally { $stream.Dispose() }
    }
    finally { $archive.Dispose() }
}

$experimentalDll = Resolve-RequiredFile $ExperimentalPluginDll 'Experimental-nested Auto Pickup DLL'
$ue4ssZip = Resolve-RequiredFile $ExperimentalUE4SSRuntimeZip 'Pinned ExperimentalNested UE4SS runtime zip'
$rangeX3Pak = Resolve-RequiredFile $PickupRangeX3Pak 'Optional 3x pickup range PAK'
$rangeX5Pak = Resolve-RequiredFile $PickupRangeX5Pak 'Optional 5x pickup range PAK'
$rangeX10Pak = Resolve-RequiredFile $PickupRangeX10Pak 'Optional 10x pickup range PAK'
$rangeX15Pak = Resolve-RequiredFile $PickupRangeX15Pak 'Optional 15x pickup range PAK'
$rangeX20Pak = Resolve-RequiredFile $PickupRangeX20Pak 'Optional 20x pickup range PAK'
$config = Resolve-RequiredFile $ConfigPath 'Public Auto Pickup config'
$lua = Resolve-RequiredFile $LuaScriptPath 'Passive Auto Pickup Lua entry point'
$notices = Resolve-RequiredFile $ThirdPartyNoticesPath 'Third-party notices'

Assert-X64PeDll $experimentalDll 'Experimental-nested Auto Pickup DLL'
$experimentalPluginHash = Get-Sha256 $experimentalDll

if ((Get-Sha256 $ue4ssZip) -ne $runtimeZipHash) {
    throw 'The UE4SS zip is not the approved ExperimentalNested compatibility runtime.'
}
if ((Get-ZipEntrySha256 $ue4ssZip 'ue4ss\UE4SS.dll') -ne $experimentalUE4SSDllHash -or
    (Get-ZipEntrySha256 $ue4ssZip 'dwmapi.dll') -ne $experimentalDwmapiHash -or
    (Get-ZipEntrySha256 $ue4ssZip 'ue4ss\DS-5.3.2-0+UE5-1c1a1497.usmap') -ne $experimentalUsmapHash -or
    (Get-ZipEntrySha256 $ue4ssZip 'ue4ss\UE4SS-settings.ini') -ne $experimentalSettingsHash) {
    throw 'The pinned ExperimentalNested UE4SS archive contains unexpected runtime files.'
}
foreach ($rangeGate in @(
    @($rangeX3Pak, $approvedRangeX3PakHash, '3x'),
    @($rangeX5Pak, $approvedRangeX5PakHash, '5x'),
    @($rangeX10Pak, $approvedRangeX10PakHash, '10x'),
    @($rangeX15Pak, $approvedRangeX15PakHash, '15x'),
    @($rangeX20Pak, $approvedRangeX20PakHash, '20x'))) {
    if ((Get-Sha256 $rangeGate[0]) -ne $rangeGate[1]) {
        throw "The optional $($rangeGate[2]) PAK is not the approved release artifact."
    }
}

$configText = [IO.File]::ReadAllText($config, [Text.UTF8Encoding]::new($false, $true))
foreach ($requiredSetting in @(
    '(?m)^[\t ]*enabled_on_launch[\t ]*=[\t ]*false[\t ]*(?=\r?$)',
    '(?m)^[\t ]*automatic_pickup[\t ]*=[\t ]*true[\t ]*(?=\r?$)',
    '(?m)^[\t ]*toggle_hotkey[\t ]*=[\t ]*F9[\t ]*(?=\r?$)',
    '(?m)^[\t ]*interaction_key[\t ]*=[\t ]*AUTO[\t ]*(?=\r?$)',
    '(?m)^[\t ]*interaction_key_fallback[\t ]*=[\t ]*F[\t ]*(?=\r?$)',
    '(?m)^[\t ]*debug_logging[\t ]*=[\t ]*false[\t ]*(?=\r?$)')) {
    if (-not [regex]::IsMatch($configText, $requiredSetting)) {
        throw "Public config release gate failed: $requiredSetting"
    }
}
if ($configText -match '(?m)^[\t ]*enabled_on_launch[\t ]*=[\t ]*true[\t ]*(?=\r?$)' -or
    $configText -match '(?m)^[\t ]*debug_logging[\t ]*=[\t ]*true[\t ]*(?=\r?$)') {
    throw 'Public config must remain startup-off and debug-off.'
}
$luaText = [IO.File]::ReadAllText($lua, [Text.UTF8Encoding]::new($false, $true))
if (-not $luaText.Contains('DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0')) {
    throw 'Lua entry point does not contain the 1.3.0 release identity marker.'
}

$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$generatedRoot = Join-Path $outputRoot '.generated'
New-Item -ItemType Directory -Path $generatedRoot -Force | Out-Null
$payloadManifest = Join-Path $generatedRoot 'payload-manifest.ini'
$manifestLines = @(
    "version=$version",
    "game_hash_policy=diagnostic_only",
    "selector_resolution_policy=runtime_reflection_dual_caller_rel32_consensus_fail_closed",
    "selector_server_anchor=reflected_virtual_slot_runtime_bounded_implementation",
    "selector_ui_anchor=reflected_pdata_terminal_rel32_runtime_bounded_implementation",
    "selector_consensus=exact_server_ui_target_match",
    "experimental_plugin_sha256=$experimentalPluginHash",
    "config_sha256=$(Get-Sha256 $config)",
    "lua_sha256=$(Get-Sha256 $lua)",
    "experimental_ue4ss_runtime_zip_sha256=$(Get-Sha256 $ue4ssZip)",
    "pickup_range_x3_sha256=$(Get-Sha256 $rangeX3Pak)",
    "pickup_range_x5_sha256=$(Get-Sha256 $rangeX5Pak)",
    "pickup_range_x10_sha256=$(Get-Sha256 $rangeX10Pak)",
    "pickup_range_x15_sha256=$(Get-Sha256 $rangeX15Pak)",
    "pickup_range_x20_sha256=$(Get-Sha256 $rangeX20Pak)",
    "third_party_notices_sha256=$(Get-Sha256 $notices)"
)
[IO.File]::WriteAllLines($payloadManifest, $manifestLines, [Text.UTF8Encoding]::new($false))

if (-not $CscPath) {
    $roslynCsc = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\Roslyn\csc.exe'
    if (Test-Path -LiteralPath $roslynCsc -PathType Leaf) { $CscPath = $roslynCsc }
}
$csc = Resolve-RequiredFile $CscPath 'Visual Studio Roslyn C# compiler'
$referenceRoot = Resolve-RequiredFile `
    'C:\Program Files (x86)\Reference Assemblies\Microsoft\Framework\.NETFramework\v4.8\mscorlib.dll' `
    '.NET Framework 4.8 reference assemblies'
$referenceRoot = Split-Path -Parent $referenceRoot

$outputExe = Join-Path $outputRoot "DragonSwordNativeAutoPickup-Setup-$version.exe"
$sources = Get-ChildItem -LiteralPath $installerRoot -Filter '*.cs' -File |
    Where-Object { $_.Name -ne 'InstallerEngine.cs' } |
    Sort-Object Name
if ($sources.Count -lt 3) { throw 'Installer C# sources are incomplete.' }

$compilerArguments = [Collections.Generic.List[string]]::new()
foreach ($argument in @(
    '/nologo', '/target:winexe', '/platform:anycpu', '/optimize+', '/debug-',
    '/deterministic+', '/highentropyva+', '/utf8output', '/nostdlib+',
    "/out:$outputExe", "/win32manifest:$(Join-Path $installerRoot 'app.manifest')")) {
    $compilerArguments.Add($argument)
}
foreach ($reference in @(
    'mscorlib.dll', 'System.dll', 'System.Core.dll', 'System.Drawing.dll',
    'System.Windows.Forms.dll', 'System.IO.Compression.dll',
    'System.IO.Compression.FileSystem.dll')) {
    $compilerArguments.Add("/reference:$(Join-Path $referenceRoot $reference)")
}
foreach ($resource in @(
    @($payloadManifest, 'Payload.Manifest.ini'),
    @($experimentalDll, 'Payload.ExperimentalPlugin.dll'),
    @($config, 'Payload.DefaultConfig.ini'),
    @($lua, 'Payload.Main.lua'),
    @($ue4ssZip, 'Payload.ExperimentalUE4SSRuntime.zip'),
    @($rangeX3Pak, 'Payload.PickupRangeX3.pak'),
    @($rangeX5Pak, 'Payload.PickupRangeX5.pak'),
    @($rangeX10Pak, 'Payload.PickupRangeX10.pak'),
    @($rangeX15Pak, 'Payload.PickupRangeX15.pak'),
    @($rangeX20Pak, 'Payload.PickupRangeX20.pak'),
    @($notices, 'Payload.ThirdPartyNotices.txt'))) {
    $compilerArguments.Add("/resource:$($resource[0]),$($resource[1])")
}
foreach ($source in $sources) { $compilerArguments.Add($source.FullName) }

& $csc $compilerArguments.ToArray()
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $outputExe -PathType Leaf)) {
    throw "Installer compilation failed: $LASTEXITCODE"
}

$installerHash = Get-Sha256 $outputExe
$checksumPath = "$outputExe.sha256"
[IO.File]::WriteAllText(
    $checksumPath,
    "$installerHash  $([IO.Path]::GetFileName($outputExe))`r`n",
    [Text.UTF8Encoding]::new($false))

Write-Host "Installer build passed: $outputExe"
Write-Host "SHA-256: $installerHash"
Write-Host "Experimental plugin SHA-256: $experimentalPluginHash"
Write-Host 'The executable is unsigned. Build success is not installation or in-game acceptance.'
