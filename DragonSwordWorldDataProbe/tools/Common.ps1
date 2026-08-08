# Shared Windows PowerShell 5.1-compatible helpers.
# This file is dot-sourced by both visible command entry points.

function Resolve-ProbeRoot {
    param([Parameter(Mandatory=$true)][string]$PathValue)

    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        throw 'The probe root path is empty.'
    }

    $candidate = [Environment]::ExpandEnvironmentVariables($PathValue.Trim())
    if ($candidate.Length -ge 2 -and $candidate.StartsWith('"') -and $candidate.EndsWith('"')) {
        $candidate = $candidate.Substring(1, $candidate.Length - 2)
    }
    $candidate = $candidate.TrimEnd([char[]]@(34))
    $candidate = $candidate.TrimEnd([char[]]@(92,47))

    if ([string]::IsNullOrWhiteSpace($candidate)) {
        throw 'The probe root path became empty after normalization.'
    }

    $item = Get-Item -LiteralPath $candidate -Force -ErrorAction Stop
    if (-not $item.PSIsContainer) {
        throw ('The probe root is not a directory: {0}' -f $candidate)
    }

    return $item.FullName.TrimEnd([char[]]@(92,47))
}

function Assert-ProbeLayout {
    param([Parameter(Mandatory=$true)][string]$ProbeRoot)

    $required = @(
        'enabled.txt',
        'scripts\main.lua',
        'scripts\config.lua',
        'scripts\core\probe_manager.lua',
        'scripts\core\module_api.lua',
        'config\suite.json',
        'config\modules.json',
        'config\profiles\assault-mole-research.json',
        'config\static-targets\assault.json',
        'metadata\release.json',
        'tools\Common.ps1',
        'tools\ModuleHost.ps1',
        'tools\Start-Monitor.ps1',
        'tools\Collect-Diagnostics.ps1',
        'tools\core\ModuleApi.ps1',
        'tools\core\StaticCache.ps1',
        'tools\modules\pak_static\Run.ps1',
        'tools\modules\assault_catalog\Run.ps1',
        'tools\modules\mole_game_discovery\Run.ps1',
        'tools\modules\mole_game_discovery\Analyze.ps1',
        'tools\modules\mole_state_discovery\Run.ps1',
        'tools\parsers\Build-AssaultCatalog.py',
        'tools\vendor\PakReaderCore.exe',
        'tools\vendor\ooz.exe'
    )
    $missing = New-Object System.Collections.Generic.List[string]
    foreach ($relative in $required) {
        $path = Join-Path $ProbeRoot $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $missing.Add($relative) }
    }
    if ($missing.Count -gt 0) {
        throw ('The modular probe directory is incomplete or nested incorrectly. Missing: {0}' -f ($missing -join ', '))
    }
}

function Get-ExceptionSummary {
    param(
        [Parameter(Mandatory=$true)][object]$ErrorObject,
        [int]$MaximumLength = 700
    )

    $message = $null
    try {
        if ($ErrorObject -is [System.Management.Automation.ErrorRecord]) {
            $message = $ErrorObject.Exception.Message
        }
        elseif ($ErrorObject -is [Exception]) {
            $message = $ErrorObject.Message
        }
        else {
            $message = [string]$ErrorObject
        }
    }
    catch {
        $message = 'unknown error'
    }

    if ([string]::IsNullOrWhiteSpace($message)) {
        $message = 'unknown error'
    }
    $message = $message.Replace("`r", ' ').Replace("`n", ' ').Trim()
    while ($message.Contains('  ')) {
        $message = $message.Replace('  ', ' ')
    }
    if ($message.Length -gt $MaximumLength) {
        $message = $message.Substring(0, $MaximumLength) + '...'
    }
    return $message
}

function Read-Utf8TextStrict {
    param([Parameter(Mandatory=$true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw ('File does not exist: {0}' -f $Path)
    }

    $bytes = [IO.File]::ReadAllBytes($Path)
    $offset = 0
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $offset = 3
    }

    try {
        $encoding = New-Object -TypeName System.Text.UTF8Encoding -ArgumentList $false, $true
        return $encoding.GetString($bytes, $offset, $bytes.Length - $offset)
    }
    catch {
        throw ('File is not valid UTF-8: {0}; {1}' -f $Path,(Get-ExceptionSummary -ErrorObject $_))
    }
}

function Read-JsonUtf8Strict {
    param([Parameter(Mandatory=$true)][string]$Path)

    $text = Read-Utf8TextStrict -Path $Path
    try {
        return ($text | ConvertFrom-Json -ErrorAction Stop)
    }
    catch {
        $hash = Get-Sha256Safe -Path $Path
        throw ('Invalid JSON: {0}; sha256={1}; {2}' -f $Path,$hash,(Get-ExceptionSummary -ErrorObject $_))
    }
}

function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][AllowEmptyString()][string]$Text
    )

    $parent = Split-Path -Parent $Path
    if ($parent) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $encoding = New-Object -TypeName System.Text.UTF8Encoding -ArgumentList $false
    [IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Write-JsonUtf8NoBom {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][object]$Value,
        [int]$Depth = 6
    )

    $json = $Value | ConvertTo-Json -Depth $Depth
    Write-Utf8NoBom -Path $Path -Text ($json + "`n")
}

function Get-Sha256Safe {
    param([Parameter(Mandatory=$true)][string]$Path)

    try {
        $command = Get-Command Get-FileHash -ErrorAction SilentlyContinue
        if ($command) {
            return (Get-FileHash -LiteralPath $Path -Algorithm SHA256 -ErrorAction Stop).Hash.ToLowerInvariant()
        }

        $stream = [IO.File]::OpenRead($Path)
        try {
            $sha = [Security.Cryptography.SHA256]::Create()
            try {
                $bytes = $sha.ComputeHash($stream)
                return ([BitConverter]::ToString($bytes)).Replace('-', '').ToLowerInvariant()
            }
            finally {
                $sha.Dispose()
            }
        }
        finally {
            $stream.Dispose()
        }
    }
    catch {
        return 'hash_failed'
    }
}

function Test-ProbeManifest {
    param([Parameter(Mandatory=$true)][string]$ProbeRoot)

    $manifestPath = Join-Path $ProbeRoot 'metadata\manifest.sha256'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        return [pscustomobject]@{ Present=$false; Checked=0; Failed=0; Details='manifest_missing' }
    }

    $lines = (Read-Utf8TextStrict -Path $manifestPath) -split "`r?`n"
    $checked = 0
    $failed = New-Object System.Collections.Generic.List[string]
    foreach ($line in $lines) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^([0-9a-fA-F]{64}) (?:\*| )(.+)$') {
            $failed.Add('invalid_manifest_line')
            continue
        }
        $expected = $Matches[1].ToLowerInvariant()
        $relative = $Matches[2].Replace('/', '\')
        if ($relative -eq 'enabled.txt' -or $relative.StartsWith('runtime\', [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        $path = Join-Path $ProbeRoot $relative
        $checked++
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            $failed.Add('missing:' + $relative)
            continue
        }
        $actual = Get-Sha256Safe -Path $path
        if ($actual -ne $expected) {
            $failed.Add('hash:' + $relative)
        }
    }

    return [pscustomobject]@{
        Present=$true
        Checked=$checked
        Failed=$failed.Count
        Details=($failed -join ',')
    }
}

function New-ZipFromDirectory {
    param(
        [Parameter(Mandatory=$true)][string]$SourceDirectory,
        [Parameter(Mandatory=$true)][string]$DestinationZip
    )

    if (-not (Test-Path -LiteralPath $SourceDirectory -PathType Container)) {
        throw ('ZIP source directory does not exist: {0}' -f $SourceDirectory)
    }
    if (Test-Path -LiteralPath $DestinationZip) {
        Remove-Item -LiteralPath $DestinationZip -Force -ErrorAction Stop
    }

    Add-Type -AssemblyName System.IO.Compression -ErrorAction SilentlyContinue
    Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction SilentlyContinue
    [IO.Compression.ZipFile]::CreateFromDirectory(
        $SourceDirectory,
        $DestinationZip,
        [IO.Compression.CompressionLevel]::Optimal,
        $false)
}

function Test-ZipArchive {
    param(
        [Parameter(Mandatory=$true)][string]$ZipPath,
        [string]$RequiredEntry = 'collection-summary.json'
    )

    if (-not (Test-Path -LiteralPath $ZipPath -PathType Leaf)) {
        throw ('ZIP output was not created: {0}' -f $ZipPath)
    }
    $file = Get-Item -LiteralPath $ZipPath -Force
    if ($file.Length -le 22) {
        throw ('ZIP output is unexpectedly small: {0} bytes' -f $file.Length)
    }

    Add-Type -AssemblyName System.IO.Compression -ErrorAction SilentlyContinue
    Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction SilentlyContinue
    $archive = [IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $entries = @($archive.Entries)
        if ($entries.Count -eq 0) {
            throw 'The generated ZIP contains no entries.'
        }
        if (-not [string]::IsNullOrWhiteSpace($RequiredEntry)) {
            $found = @($entries | Where-Object { $_.FullName -eq $RequiredEntry }).Count -gt 0
            if (-not $found) {
                throw ('The generated ZIP is missing required entry: {0}' -f $RequiredEntry)
            }
        }
        return $entries.Count
    }
    finally {
        $archive.Dispose()
    }
}

function Get-GameProcessesCompat {
    $all = @()
    foreach ($name in @('DSClient-Win64-Shipping','DSClient_Win64_Shipping','DSClient')) {
        $all += @(Get-Process -Name $name -ErrorAction SilentlyContinue)
    }
    return @($all | Sort-Object Id -Unique)
}
