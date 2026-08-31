#requires -Version 5.1
param(
    [string]$Version
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw |
    ConvertFrom-Json
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = [string]$release.version
}
if ([string]::IsNullOrWhiteSpace($Version) -or
    $Version -ne [string]$release.version) {
    throw 'Requested package version does not match metadata/release.json.'
}

$stage = Join-Path $root ("dist\DragonSwordWorldRadar-$Version")
$mod = Join-Path $stage 'DragonSwordWorldRadar'
$archive = Join-Path $root ("dist\DragonSwordWorldRadar-v$Version.zip")
if (-not (Test-Path -LiteralPath $mod -PathType Container)) {
    throw "Release staging directory is missing: $mod"
}
if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
    throw "Release archive is missing: $archive"
}

function Get-RelativePath([string]$Base, [string]$Path) {
    return $Path.Substring($Base.Length).TrimStart('\').Replace('\','/')
}
function Get-StreamHash([IO.Stream]$Stream) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = $sha.ComputeHash($Stream)
        return ([BitConverter]::ToString($bytes)).Replace('-','').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}
function Assert-SafeRelativePath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path) -or
        $Path.Contains('\') -or
        $Path.StartsWith('/') -or
        $Path -match '^[A-Za-z]:' -or
        $Path.StartsWith('//')) {
        throw "Unsafe ZIP path: $Path"
    }
    $trimmed = $Path.TrimEnd('/')
    foreach ($part in $trimmed.Split('/')) {
        if ([string]::IsNullOrWhiteSpace($part) -or
            $part -eq '.' -or
            $part -eq '..') {
            throw "Unsafe ZIP path segment: $Path"
        }
    }
}

$manifestPath = Join-Path $mod 'metadata\build-manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ([string]$manifest.version -ne $Version) {
    throw 'Build manifest version does not match the release version.'
}
$manifestByPath = @{}
foreach ($record in @($manifest.files)) {
    $path = ([string]$record.path).Replace('\','/')
    Assert-SafeRelativePath $path
    $key = $path.ToLowerInvariant()
    if ($manifestByPath.ContainsKey($key)) {
        throw "Duplicate manifest path: $path"
    }
    $manifestByPath[$key] = $record
}

$stageFiles = @(Get-ChildItem -LiteralPath $mod -Recurse -File |
    Sort-Object FullName)
$stageByPath = @{}
foreach ($file in $stageFiles) {
    $relative = Get-RelativePath $mod $file.FullName
    Assert-SafeRelativePath $relative
    $key = $relative.ToLowerInvariant()
    if ($stageByPath.ContainsKey($key)) {
        throw "Case-insensitive duplicate staging path: $relative"
    }
    $stageByPath[$key] = $file
    if ($relative -ieq 'metadata/build-manifest.json') {
        continue
    }
    if (-not $manifestByPath.ContainsKey($key)) {
        throw "Staging file is absent from the manifest: $relative"
    }
    $record = $manifestByPath[$key]
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    if ([int64]$record.size -ne [int64]$file.Length -or
        [string]$record.sha256 -ne $hash) {
        throw "Manifest size/hash mismatch: $relative"
    }
}
if ($manifestByPath.Count -ne ($stageByPath.Count - 1)) {
    throw 'Build manifest and staging file sets differ.'
}

foreach ($emptyRelative in @(
    'data\generated',
    'runtime\bridge',
    'runtime\diagnostics',
    'runtime\logs')) {
    $emptyPath = Join-Path $mod $emptyRelative
    if (-not (Test-Path -LiteralPath $emptyPath -PathType Container) -or
        $null -ne (Get-ChildItem -LiteralPath $emptyPath -Force |
            Select-Object -First 1)) {
        throw "Required writable directory is absent or not empty: $emptyRelative"
    }
}

$configFiles = @(Get-ChildItem -LiteralPath $mod -Recurse -File |
    Where-Object { $_.Name -ieq 'config.lua' })
if ($configFiles.Count -ne 1 -or
    (Get-RelativePath $mod $configFiles[0].FullName) -ine 'scripts/config.lua') {
    throw 'Release must contain exactly scripts/config.lua.'
}
if (@(Get-ChildItem -LiteralPath $mod -Recurse -File |
        Where-Object { $_.Name -ieq 'config.default.lua' }).Count -ne 0) {
    throw 'Release contains retired config.default.lua.'
}
if (@($stageFiles | Where-Object {
        (Get-RelativePath $mod $_.FullName) -match '(?i)(^|/)(obj|bin)/'
    }).Count -ne 0) {
    throw 'Release staging contains obj/bin build output.'
}
$executables = @($stageFiles | Where-Object { $_.Extension -ieq '.exe' } |
    ForEach-Object { Get-RelativePath $mod $_.FullName })
$libraries = @($stageFiles | Where-Object { $_.Extension -ieq '.dll' } |
    ForEach-Object { Get-RelativePath $mod $_.FullName })
if ($executables.Count -ne 1 -or $executables[0] -ine 'tools/ooz.exe') {
    throw 'Unexpected executable set in release staging.'
}
if ($libraries.Count -ne 1 -or
    $libraries[0] -ine 'vendor/sqlcipher/e_sqlcipher.dll') {
    throw 'Unexpected DLL set in release staging.'
}

$versionFiles = @(
    'scripts\main.lua',
    'host\DragonSwordWorldRadar.Host.ps1',
    'host\DragonSwordWorldRadar.Watcher.ps1',
    'host\DragonSwordWorldRadar.Watcher.vbs')
foreach ($relative in $versionFiles) {
    $text = Get-Content -LiteralPath (Join-Path $mod $relative) -Raw
    if (-not $text.Contains($Version)) {
        throw "Release runtime file has no current version: $relative"
    }
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $expectedPrefix = 'DragonSwordWorldRadar/'
    $expectedDirectories = @{}
    foreach ($requiredDirectory in @(
        'DragonSwordWorldRadar/data/generated/',
        'DragonSwordWorldRadar/runtime/bridge/',
        'DragonSwordWorldRadar/runtime/diagnostics/',
        'DragonSwordWorldRadar/runtime/logs/')) {
        $expectedDirectories[$requiredDirectory.ToLowerInvariant()] = $true
    }
    $zipPaths = @{}
    $zipObjectPaths = @{}
    $zipFileCount = 0
    foreach ($entry in $zip.Entries) {
        $name = $entry.FullName
        Assert-SafeRelativePath $name
        if (-not $name.StartsWith(
                $expectedPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            throw "ZIP entry is outside the Mod root: $name"
        }
        $key = $name.ToLowerInvariant()
        if ($zipPaths.ContainsKey($key)) {
            throw "Case-insensitive duplicate ZIP path: $name"
        }
        $zipPaths[$key] = $true
        $isDirectory = $name.EndsWith('/')
        $objectKey = $name.TrimEnd('/').ToLowerInvariant()
        if ($zipObjectPaths.ContainsKey($objectKey)) {
            throw "ZIP file/directory path collision: $name"
        }
        $zipObjectPaths[$objectKey] = $true
        if ($isDirectory) {
            if ($entry.Length -ne 0) {
                throw "ZIP directory entry has content: $name"
            }
            if (-not $expectedDirectories.ContainsKey($key)) {
                throw "Unexpected ZIP directory entry: $name"
            }
            continue
        }

        $zipFileCount++
        $relative = $name.Substring($expectedPrefix.Length)
        $stageKey = $relative.ToLowerInvariant()
        if (-not $stageByPath.ContainsKey($stageKey)) {
            throw "ZIP file is absent from staging: $name"
        }
        $entryStream = $entry.Open()
        try {
            $entryHash = Get-StreamHash $entryStream
        }
        finally {
            $entryStream.Dispose()
        }
        $stageFile = $stageByPath[$stageKey]
        $stageHash = (Get-FileHash -LiteralPath $stageFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        if ([int64]$entry.Length -ne [int64]$stageFile.Length -or
            $entryHash -ne $stageHash) {
            throw "ZIP/staging byte mismatch: $name"
        }
    }

    if ($zipFileCount -ne $stageFiles.Count) {
        throw 'ZIP and staging file counts differ.'
    }
    $zipDirectoryCount = @($zipPaths.Keys |
        Where-Object { $_.EndsWith('/') }).Count
    if ($zipDirectoryCount -ne $expectedDirectories.Count) {
        throw 'ZIP directory entry count differs from the expected set.'
    }
    foreach ($requiredDirectory in $expectedDirectories.Keys) {
        if (-not $zipPaths.ContainsKey($requiredDirectory)) {
            throw "ZIP is missing empty directory: $requiredDirectory"
        }
    }
    Write-Host ("RELEASE_PACKAGE_TESTS_OK version={0}; entries={1}; files={2}; manifest={3}" -f
        $Version,
        $zip.Entries.Count,
        $zipFileCount,
        $manifestByPath.Count)
}
finally {
    $zip.Dispose()
}
