[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageRoot,
    [string]$ModsRoot = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Binaries\Win64\ue4ss\Mods',
    [switch]$EnableDebugForLocalInstall
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$projectRoot = Split-Path -Parent $PSScriptRoot
throw 'Direct-copy deployment is retired. Build with tools\Build-Release.ps1 and use the Setup inside dist\releases\1.3.0\DragonSwordAutoPickup-v1.3.0-Installer.zip.'
$version = '1.0.1'
$expectedExperimentalUE4SSHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$expectedExperimentalProxyHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'

if (Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue) {
    throw 'Refusing to deploy while DragonSword is running.'
}

$package = (Resolve-Path -LiteralPath $PackageRoot).Path
$source = Join-Path $package 'ue4ss\Mods\DragonSwordNativeAutoPickup'
if (-not (Test-Path -LiteralPath $source -PathType Container)) {
    throw "Package root does not contain the expected UE4SS mod tree: $source"
}
$expected = @(
    'config.ini',
    'dlls\main.dll',
    'Scripts\main.lua',
    'THIRD_PARTY_NOTICES.txt'
)
$actual = @(Get-ChildItem -LiteralPath $source -Recurse -File | ForEach-Object {
    $_.FullName.Substring($source.Length).TrimStart('\')
})
$difference = Compare-Object -ReferenceObject ($expected | Sort-Object) -DifferenceObject ($actual | Sort-Object)
if ($difference) { throw "Unexpected package contents: $($difference | Out-String)" }

$sourceConfigPath = Join-Path $source 'config.ini'
$sourceConfig = [IO.File]::ReadAllText($sourceConfigPath)
$sourceLua = [IO.File]::ReadAllText((Join-Path $source 'Scripts\main.lua'))
if ($sourceConfig -notmatch '(?m)^enabled_on_launch=false\r?$' -or
    $sourceConfig -notmatch '(?m)^toggle_hotkey=F9\r?$' -or
    $sourceConfig -notmatch '(?m)^interaction_key=AUTO\r?$' -or
    $sourceConfig -notmatch '(?m)^interaction_key_fallback=F\r?$' -or
    $sourceConfig -notmatch '(?m)^debug_logging=false\r?$' -or
    $sourceConfig -match '(?im)^\s*(range|radius|multiplier|proxy)[^=]*=' -or
    $sourceLua -notmatch 'DRAGONSWORD_NATIVE_AUTO_PICKUP_1_0_1' -or
    $sourceLua -match '(?i)overlap[_ -]?proxy|range[_ -]?multiplier') {
    throw 'Deployment input violates the 1.0.1 public runtime contract.'
}

& (Join-Path $PSScriptRoot 'Verify-BuiltArtifact.ps1') `
    -DllPath (Join-Path $source 'dlls\main.dll') `
    -UE4SSVariant ExperimentalNested | Out-Null

$resolvedModsRoot = [IO.Path]::GetFullPath($ModsRoot)
$ue4ssRoot = [IO.Path]::GetFullPath((Split-Path -Parent $resolvedModsRoot))
$win64Root = [IO.Path]::GetFullPath((Split-Path -Parent $ue4ssRoot))
if ([IO.Path]::GetFileName($resolvedModsRoot) -ne 'Mods' -or
    [IO.Path]::GetFileName($ue4ssRoot) -ne 'ue4ss') {
    throw 'Local deployment only supports the verified experimental Win64\ue4ss\Mods layout.'
}
$ue4ssDll = Join-Path $ue4ssRoot 'UE4SS.dll'
$proxyDll = Join-Path $win64Root 'dwmapi.dll'
foreach ($loaderFile in @($ue4ssDll, $proxyDll)) {
    if (-not (Test-Path -LiteralPath $loaderFile -PathType Leaf)) {
        throw "The verified experimental UE4SS loader is incomplete: $loaderFile"
    }
}
if ((Get-FileHash -LiteralPath $ue4ssDll -Algorithm SHA256).Hash -ne $expectedExperimentalUE4SSHash -or
    (Get-FileHash -LiteralPath $proxyDll -Algorithm SHA256).Hash -ne $expectedExperimentalProxyHash) {
    throw 'Local deployment stopped because the experimental UE4SS loader fingerprint changed.'
}
$target = [IO.Path]::GetFullPath((Join-Path $resolvedModsRoot 'DragonSwordNativeAutoPickup'))
$targetPrefix = $resolvedModsRoot.TrimEnd('\') + '\'
if (-not $target.StartsWith($targetPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Resolved deployment target escaped ModsRoot.'
}
$modsTxt = Join-Path $resolvedModsRoot 'mods.txt'
if (-not (Test-Path -LiteralPath $modsTxt -PathType Leaf)) {
    throw "The controlling mods.txt was not found: $modsTxt"
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backup = Join-Path $projectRoot ("runtime\rollback\installed-1.0.1-predeploy-" + $stamp)
foreach ($relative in @($expected + @('enabled.txt'))) {
    $existing = Join-Path $target $relative
    if (Test-Path -LiteralPath $existing -PathType Leaf) {
        $backupPath = Join-Path $backup $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $backupPath) -Force | Out-Null
        Copy-Item -LiteralPath $existing -Destination $backupPath -Force
    }
}
New-Item -ItemType Directory -Path $backup -Force | Out-Null
Copy-Item -LiteralPath $modsTxt -Destination (Join-Path $backup 'mods.txt') -Force

foreach ($relative in $expected) {
    $destination = Join-Path $target $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $source $relative) -Destination $destination -Force
}
$legacyEnabled = Join-Path $target 'enabled.txt'
if (Test-Path -LiteralPath $legacyEnabled -PathType Leaf) {
    Remove-Item -LiteralPath $legacyEnabled -Force
}
& (Join-Path $PSScriptRoot 'Update-ModsTxt.ps1') -ModsTxtPath $modsTxt -Enabled 1 | Out-Null

$installedConfigPath = Join-Path $target 'config.ini'
if ($EnableDebugForLocalInstall) {
    $installedConfig = [IO.File]::ReadAllText($installedConfigPath)
    $updatedConfig = [regex]::Replace($installedConfig, '(?m)^debug_logging=false\r?$', 'debug_logging=true')
    if ($updatedConfig -eq $installedConfig) {
        throw 'Local Debug override could not find the public default.'
    }
    [IO.File]::WriteAllText($installedConfigPath, $updatedConfig, [Text.UTF8Encoding]::new($false))
}

foreach ($relative in @('dlls\main.dll', 'Scripts\main.lua', 'THIRD_PARTY_NOTICES.txt')) {
    $sourceHash = (Get-FileHash -LiteralPath (Join-Path $source $relative) -Algorithm SHA256).Hash
    $installedHash = (Get-FileHash -LiteralPath (Join-Path $target $relative) -Algorithm SHA256).Hash
    if ($sourceHash -ne $installedHash) { throw "Installed hash mismatch: $relative" }
}
if (Test-Path -LiteralPath $legacyEnabled) {
    throw 'Legacy enabled.txt still exists after deployment.'
}
$installedConfig = [IO.File]::ReadAllText($installedConfigPath)
if ($EnableDebugForLocalInstall) {
    $normalized = [regex]::Replace($installedConfig, '(?m)^debug_logging=true\r?$', 'debug_logging=false')
    if ($normalized -ne $sourceConfig) { throw 'Installed config differs beyond the local Debug override.' }
} elseif ($installedConfig -ne $sourceConfig) {
    throw 'Installed config does not match the release package.'
}
if ([regex]::Matches([IO.File]::ReadAllText($modsTxt),
        '(?m)^\s*DragonSwordNativeAutoPickup\s*:\s*1\s*\r?$').Count -ne 1) {
    throw 'mods.txt does not contain exactly one enabled AutoPickup entry.'
}

$record = [ordered]@{
    schema_version = 1
    version = $version
    deployed_at_utc = [DateTime]::UtcNow.ToString('O')
    installed_path = $target
    controlling_mods_txt = $modsTxt
    installed_dll_sha256 = (Get-FileHash -LiteralPath (Join-Path $target 'dlls\main.dll') -Algorithm SHA256).Hash
    local_debug_override = [bool]$EnableDebugForLocalInstall
    enabled_txt_present = $false
    auto_pickup_modifies_range = $false
    backup_path = $backup
    static_validation = 'PASSED'
    gameplay_acceptance = 'NOT_VALIDATED_FOR_EXACT_ARTIFACT'
}
$recordRoot = Join-Path $projectRoot 'runtime\deployments'
New-Item -ItemType Directory -Path $recordRoot -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $recordRoot 'latest.json'),
    ($record | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
[pscustomobject]$record
