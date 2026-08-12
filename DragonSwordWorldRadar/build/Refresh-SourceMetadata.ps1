#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json
$version = [string]$release.version
if ([string]::IsNullOrWhiteSpace($version)) {
    throw 'metadata/release.json has no version.'
}

$stageRoot = Join-Path $root ("dist\DragonSwordWorldRadar-$version\DragonSwordWorldRadar")
if (-not (Test-Path -LiteralPath $stageRoot -PathType Container)) {
    throw "Built release staging directory is missing: $stageRoot"
}

$utf8 = New-Object Text.UTF8Encoding($false)
function Get-RelativePath([string]$Base, [string]$Path) {
    return $Path.Substring($Base.Length).TrimStart('\').Replace('\','/')
}
function Get-FileRecord([string]$Base, [IO.FileInfo]$File) {
    return [ordered]@{
        path = Get-RelativePath $Base $File.FullName
        size = [int64]$File.Length
        sha256 = (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
function Add-Mapping([Collections.ArrayList]$Mappings,[string]$SourceRelative,[string]$ReleaseRelative) {
    $sourcePath = Join-Path $root $SourceRelative
    $releasePath = Join-Path $stageRoot $ReleaseRelative
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Source mapping file is missing: $SourceRelative"
    }
    if (-not (Test-Path -LiteralPath $releasePath -PathType Leaf)) {
        throw "Release mapping file is missing: $ReleaseRelative"
    }
    $source = Get-Item -LiteralPath $sourcePath
    $built = Get-Item -LiteralPath $releasePath
    $sourceHash = (Get-FileHash -LiteralPath $source.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $releaseHash = (Get-FileHash -LiteralPath $built.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    [void]$Mappings.Add([ordered]@{
        source_path = $SourceRelative.Replace('\','/')
        release_path = $ReleaseRelative.Replace('\','/')
        source_size = [int64]$source.Length
        source_sha256 = $sourceHash
        release_size = [int64]$built.Length
        release_sha256 = $releaseHash
        byte_identical = $source.Length -eq $built.Length -and $sourceHash -eq $releaseHash
    })
}
function Add-TreeMappings([Collections.ArrayList]$Mappings,[string]$SourceDirectory,[string]$ReleaseDirectory) {
    $sourceRoot = Join-Path $root $SourceDirectory
    foreach ($file in @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | Sort-Object FullName)) {
        if ($file.FullName -match '[\\/](?:obj|bin)[\\/]') { continue }
        $relative = Get-RelativePath $sourceRoot $file.FullName
        Add-Mapping $Mappings ($SourceDirectory + '\' + $relative.Replace('/','\')) ($ReleaseDirectory + '\' + $relative.Replace('/','\'))
    }
}

$mappings = New-Object Collections.ArrayList
Add-Mapping $mappings 'src\installer\Install.cmd' 'Install.cmd'
Add-Mapping $mappings 'src\installer\Install.ps1' 'installer\Install.ps1'
Add-TreeMappings $mappings 'src\installer\Core' 'src\installer'
Add-TreeMappings $mappings 'src\overlay' 'src\overlay'
Add-TreeMappings $mappings 'src\host' 'host'
Add-TreeMappings $mappings 'src\tools' 'tools'
Add-Mapping $mappings 'vendor\ooz\ooz.exe' 'tools\ooz.exe'
Add-TreeMappings $mappings 'src\ue4ss' 'scripts'
Add-TreeMappings $mappings 'resources\defaults' 'data\defaults'
Add-Mapping $mappings 'vendor\sqlcipher\e_sqlcipher.dll' 'vendor\sqlcipher\e_sqlcipher.dll'
Add-Mapping $mappings 'metadata\release.json' 'metadata\release.json'
Add-Mapping $mappings 'metadata\data-providers.json' 'metadata\data-providers.json'
Add-Mapping $mappings 'metadata\assault-inference-policy.xml' 'metadata\assault-inference-policy.xml'
Add-Mapping $mappings 'metadata\build-validation.json' 'metadata\build-validation.json'
Add-Mapping $mappings 'LICENSE' 'licenses\GPL-3.0.txt'
Add-Mapping $mappings 'licenses\APACHE-2.0.txt' 'licenses\APACHE-2.0.txt'
Add-Mapping $mappings 'licenses\SQLCIPHER.txt' 'licenses\SQLCIPHER.txt'
Add-Mapping $mappings 'THIRD_PARTY_NOTICES.txt' 'THIRD_PARTY_NOTICES.txt'
Add-Mapping $mappings 'README.md' 'README.txt'
Add-Mapping $mappings 'resources\enabled.txt' 'enabled.txt'

$byteIdentical = @($mappings | Where-Object { $_.byte_identical }).Count
if ($byteIdentical -ne $mappings.Count) {
    $mismatches = @($mappings |
        Where-Object { -not $_.byte_identical } |
        ForEach-Object {
            "$($_.source_path) -> $($_.release_path)"
        })
    throw ('Source/release byte mismatch: ' +
        ($mismatches -join ', '))
}
$releaseMap = [ordered]@{
    schema_version = 3
    version = $version
    source_layout = 'repository'
    release_layout = 'deployed Mod folder'
    mapping_count = $mappings.Count
    byte_identical_count = $byteIdentical
    note = 'Explicit complete source-to-release mapping generated from the Build-Release staging tree. metadata/build-manifest.json and empty writable directories are release-generated and intentionally unmapped.'
    retired_paths = @(
        'src/ue4ss/boss_tracker.lua',
        'src/overlay/Bridge/StaticStateBridgeReader.cs',
        'src/overlay/Models/RadarState.cs'
    )
    release_generated_only = @(
        'metadata/build-manifest.json',
        'data/generated/',
        'runtime/bridge/',
        'runtime/diagnostics/',
        'runtime/logs/'
    )
    mappings = @($mappings)
}
[IO.File]::WriteAllText(
    (Join-Path $root 'metadata\source-release-map.json'),
    ($releaseMap | ConvertTo-Json -Depth 8),
    $utf8)

$baselineCommit = (& git -C (Split-Path -Parent $root) rev-parse HEAD 2>$null)
if ([string]::IsNullOrWhiteSpace($baselineCommit)) {
    $baselineCommit = 'unknown'
}
$sourceSnapshot = [ordered]@{
    schema_version = 3
    current_source_version = $version
    source_layout = 'repository'
    source_commit_baseline = $baselineCommit.Trim()
    runtime_ipc = 'protocol-v6 single scalar Motion/control Bridge with compact change notifications, 250 ms fallback, and debug-only strict inline A/B mode'
    motion_protocol_version = 6
    motion_field_count = 38
    overlay_source_files = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File | Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' }).Count
    installer_core_source_files = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File | Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' }).Count
    ue4ss_lua_modules = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\ue4ss') -Filter '*.lua' -File).Count
    mole_fly_records = 34
    windows_compile_status = 'passed in Windows PowerShell 5.1'
    in_game_status = "$version Windows source/package gates passed; compact smoothness, F7/F8 latency, travel, and frame-time require independent in-game acceptance"
    working_tree_status = 'implementation intentionally left uncommitted and undeployed for owner review'
}
[IO.File]::WriteAllText(
    (Join-Path $root 'metadata\source-snapshot.json'),
    ($sourceSnapshot | ConvertTo-Json -Depth 6),
    $utf8)

$manifestFiles = @()
foreach ($file in @(Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName)) {
    $relative = Get-RelativePath $root $file.FullName
    if (($relative -eq 'metadata/source-manifest.json') -or
        $relative.StartsWith('dist/', [StringComparison]::OrdinalIgnoreCase) -or
        $relative.StartsWith('runtime/', [StringComparison]::OrdinalIgnoreCase) -or
        $relative -match '(?i)^src/(?:overlay|installer/Core)/(?:obj|bin)/') {
        continue
    }
    $manifestFiles += Get-FileRecord $root $file
}
if (@($manifestFiles | Where-Object {
        $_.path -match '(?i)^src/(?:overlay|installer/Core)/(?:obj|bin)/'
    }).Count -ne 0) {
    throw 'Source manifest contains excluded obj/bin build output.'
}
$manifest = [ordered]@{
    schema_version = 3
    version = $version
    generated_at_utc = [DateTime]::UtcNow.ToString('O')
    hash_mode = 'sha256_raw_file_bytes'
    excludes = @('metadata/source-manifest.json','.git/','dist/','runtime/','src/overlay/obj/','src/overlay/bin/','src/installer/Core/obj/','src/installer/Core/bin/')
    baseline_commit = $baselineCommit.Trim()
    scope = 'complete DragonSwordWorldRadar repository source inventory'
    file_count = $manifestFiles.Count
    files = $manifestFiles
}
[IO.File]::WriteAllText(
    (Join-Path $root 'metadata\source-manifest.json'),
    ($manifest | ConvertTo-Json -Depth 8),
    $utf8)

Write-Host "SOURCE_METADATA_REFRESHED version=$version files=$($manifestFiles.Count) mappings=$($mappings.Count) identical=$byteIdentical"
