Set-StrictMode -Version 2.0

function Resolve-DragonSwordWorldRadarGameLayout {
    param(
        [Parameter(Mandatory=$true)][string]$ModDir,
        [switch]$RequireOodle
    )

    $candidate = Get-Item -LiteralPath ([IO.Path]::GetFullPath($ModDir))
    for ($level = 0; $level -lt 14 -and $candidate -ne $null; $level++) {
        $gameRoot = $candidate.FullName
        $win64Root = Join-Path $gameRoot 'DS\Binaries\Win64'
        $executable = Join-Path $win64Root 'DSClient-Win64-Shipping.exe'
        $pak = Join-Path $gameRoot 'DS\Content\Paks\pakchunk109-WindowsClient.pak'
        if ((Test-Path -LiteralPath $executable) -and (Test-Path -LiteralPath $pak)) {
            # DragonSword does not ship an oo2core DLL. The established 1.6.1
            # installer uses the open-source ooz.exe decoder instead. Prefer a tool
            # bundled with DragonSwordWorldRadar, then the existing UE4SS Mods-root copy.
            $oozCandidates = @(
                (Join-Path $ModDir 'tools\ooz.exe'),
                (Join-Path $win64Root 'Mods\ooz.exe'),
                (Join-Path $gameRoot 'ooz.exe')
            ) | Select-Object -Unique
            $ooz = $oozCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
            if ($RequireOodle -and -not $ooz) {
                throw "The Oodle decoder ooz.exe was not found. It is only required when regenerating data for a new game version."
            }
            return [pscustomobject]@{
                GameRoot = $gameRoot
                Win64Root = $win64Root
                ExecutablePath = $executable
                PakPath = $pak
                OodleLibraryPath = if ($ooz) { [IO.Path]::GetFullPath($ooz) } else { $null }
            }
        }
        $candidate = $candidate.Parent
    }
    throw 'Could not resolve the DragonSword Awakening game root from the DragonSwordWorldRadar folder.'
}

function Get-DragonSwordWorldRadarGameIdentity {
    param([Parameter(Mandatory=$true)]$Layout)

    $exe = Get-Item -LiteralPath $Layout.ExecutablePath
    $pak = Get-Item -LiteralPath $Layout.PakPath
    $fileVersion = [string]$exe.VersionInfo.FileVersion
    $productVersion = [string]$exe.VersionInfo.ProductVersion
    $canonical = @(
        $fileVersion,
        $productVersion,
        [string]$exe.Length,
        $exe.LastWriteTimeUtc.Ticks.ToString(),
        [string]$pak.Length,
        $pak.LastWriteTimeUtc.Ticks.ToString()
    ) -join '|'
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($canonical)
        $fingerprint = ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
    } finally {
        $sha.Dispose()
    }
    $displayVersion = if ($productVersion) { $productVersion } elseif ($fileVersion) { $fileVersion } else { $fingerprint.Substring(0,12) }
    return [ordered]@{
        display_version = $displayVersion
        file_version = $fileVersion
        product_version = $productVersion
        fingerprint = $fingerprint
        executable_length = [int64]$exe.Length
        executable_last_write_utc = $exe.LastWriteTimeUtc.ToString('O')
        pak_length = [int64]$pak.Length
        pak_last_write_utc = $pak.LastWriteTimeUtc.ToString('O')
    }
}

function Write-DragonSwordWorldRadarJson {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)]$Value,
        [int]$Depth = 8
    )
    $directory = Split-Path -Parent $Path
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    $temporary = $Path + '.' + [Guid]::NewGuid().ToString('N') + '.tmp'
    $json = $Value | ConvertTo-Json -Depth $Depth
    [IO.File]::WriteAllText($temporary, $json, [Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $temporary -Destination $Path -Force
}

function Test-DragonSwordWorldRadarInstalledGame {
    param(
        [Parameter(Mandatory=$true)][string]$ModDir,
        [Parameter(Mandatory=$true)]$Layout
    )
    $statePath = Join-Path $ModDir 'metadata\install-state.json'
    if (-not (Test-Path -LiteralPath $statePath)) {
        return [pscustomobject]@{ Compatible = $false; Reason = 'install-state-missing'; Installed = $null; Current = (Get-DragonSwordWorldRadarGameIdentity $Layout) }
    }
    try {
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        $current = Get-DragonSwordWorldRadarGameIdentity $Layout
        $installedFingerprint = [string]$state.game.fingerprint
        return [pscustomobject]@{
            Compatible = ($installedFingerprint -and $installedFingerprint -eq [string]$current.fingerprint)
            Reason = if ($installedFingerprint -eq [string]$current.fingerprint) { 'match' } else { 'game-version-changed' }
            Installed = $state.game
            Current = $current
        }
    } catch {
        return [pscustomobject]@{ Compatible = $false; Reason = 'install-state-invalid'; Installed = $null; Current = (Get-DragonSwordWorldRadarGameIdentity $Layout) }
    }
}
