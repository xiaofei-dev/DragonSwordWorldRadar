#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$windowsPowerShell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
if (-not (Test-Path -LiteralPath $windowsPowerShell -PathType Leaf)) {
    throw 'Windows PowerShell 5.1 is required to build a release.'
}

function Invoke-Gate([string]$ScriptName) {
    $scriptPath = Join-Path $PSScriptRoot $ScriptName
    & $windowsPowerShell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath
    if ($LASTEXITCODE -ne 0) {
        throw "Validation gate failed: $ScriptName (exit=$LASTEXITCODE)"
    }
}

& (Join-Path $PSScriptRoot 'Verify-Source.ps1')
Invoke-Gate 'Compile-Source.ps1'
Invoke-Gate 'Test-Refactor.ps1'

$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json
$version = [string]$release.version
if ([string]::IsNullOrWhiteSpace($version)) { throw 'metadata/release.json has no version.' }

$dist = Join-Path $root 'dist'
$stage = Join-Path $dist ("DragonSwordWorldRadar-$version")
$mod = Join-Path $stage 'DragonSwordWorldRadar'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $mod | Out-Null

function Copy-Tree([string]$Source,[string]$Destination) {
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) { throw "Missing source directory: $Source" }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Copy-Item -Path (Join-Path $Source '*') -Destination $Destination -Recurse -Force
}
function Copy-One([string]$Source,[string]$Destination) {
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) { throw "Missing source file: $Source" }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

Copy-One (Join-Path $root 'src\installer\Install.cmd') (Join-Path $mod 'Install.cmd')
Copy-One (Join-Path $root 'src\installer\Install.ps1') (Join-Path $mod 'installer\Install.ps1')
Copy-Tree (Join-Path $root 'src\installer\Core') (Join-Path $mod 'src\installer')
Copy-Tree (Join-Path $root 'src\overlay') (Join-Path $mod 'src\overlay')
Copy-Tree (Join-Path $root 'src\host') (Join-Path $mod 'host')
Copy-Tree (Join-Path $root 'src\tools') (Join-Path $mod 'tools')
Copy-One (Join-Path $root 'vendor\ooz\ooz.exe') (Join-Path $mod 'tools\ooz.exe')
Copy-Tree (Join-Path $root 'src\ue4ss') (Join-Path $mod 'scripts')
Copy-Tree (Join-Path $root 'resources\defaults') (Join-Path $mod 'data\defaults')
Copy-One (Join-Path $root 'vendor\sqlcipher\e_sqlcipher.dll') (Join-Path $mod 'vendor\sqlcipher\e_sqlcipher.dll')
Copy-One (Join-Path $root 'metadata\release.json') (Join-Path $mod 'metadata\release.json')
Copy-One (Join-Path $root 'metadata\data-providers.json') (Join-Path $mod 'metadata\data-providers.json')
Copy-One (Join-Path $root 'metadata\build-validation.json') (Join-Path $mod 'metadata\build-validation.json')
Copy-One (Join-Path $root 'LICENSE') (Join-Path $mod 'licenses\GPL-3.0.txt')
Copy-One (Join-Path $root 'licenses\APACHE-2.0.txt') (Join-Path $mod 'licenses\APACHE-2.0.txt')
Copy-One (Join-Path $root 'licenses\SQLCIPHER.txt') (Join-Path $mod 'licenses\SQLCIPHER.txt')
Copy-One (Join-Path $root 'THIRD_PARTY_NOTICES.txt') (Join-Path $mod 'THIRD_PARTY_NOTICES.txt')
Copy-One (Join-Path $root 'README.md') (Join-Path $mod 'README.txt')

$utf8 = New-Object Text.UTF8Encoding($false)
Copy-One (Join-Path $root 'resources\enabled.txt') (Join-Path $mod 'enabled.txt')
New-Item -ItemType Directory -Force -Path (Join-Path $mod 'data\generated'),(Join-Path $mod 'runtime\bridge'),(Join-Path $mod 'runtime\diagnostics'),(Join-Path $mod 'runtime\logs') | Out-Null

$manifestFiles = @()
foreach ($file in @(Get-ChildItem -LiteralPath $mod -Recurse -File | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($mod.Length).TrimStart('\').Replace('\','/')
    if ($relative -eq 'metadata/build-manifest.json') { continue }
    $manifestFiles += [ordered]@{
        path = $relative
        size = [int64]$file.Length
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$manifest = [ordered]@{
    schema_version = 1
    version = $version
    generated_at_utc = [DateTime]::UtcNow.ToString('O')
    custom_executable_count = 0
    bundled_tool_executable_count = 1
    files = $manifestFiles
}
[IO.File]::WriteAllText((Join-Path $mod 'metadata\build-manifest.json'),($manifest | ConvertTo-Json -Depth 6),$utf8)

$archive = Join-Path $dist ("DragonSwordWorldRadar-v$version.zip")
if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$stream = [IO.File]::Open($archive,[IO.FileMode]::CreateNew,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {
    $zip = New-Object IO.Compression.ZipArchive($stream,[IO.Compression.ZipArchiveMode]::Create,$true)
    try {
        # Preserve the intentionally empty generated/runtime directories in the
        # release ZIP. Windows extraction then creates the complete writable
        # layout before the first game session or diagnostics collection.
        foreach ($directory in @(Get-ChildItem -LiteralPath $stage -Recurse -Directory | Sort-Object FullName)) {
            $child = Get-ChildItem -LiteralPath $directory.FullName -Force | Select-Object -First 1
            if ($null -eq $child) {
                $relativeDirectory = $directory.FullName.Substring($stage.Length).TrimStart('\').Replace('\','/').TrimEnd('/') + '/'
                $null = $zip.CreateEntry($relativeDirectory)
            }
        }
        foreach ($file in @(Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName)) {
            $relative = $file.FullName.Substring($stage.Length).TrimStart('\').Replace('\','/')
            $entry = $zip.CreateEntry($relative,[IO.Compression.CompressionLevel]::Optimal)
            $inputStream = [IO.File]::OpenRead($file.FullName)
            $outputStream = $entry.Open()
            try { $inputStream.CopyTo($outputStream) }
            finally { $outputStream.Dispose(); $inputStream.Dispose() }
        }
    }
    finally { $zip.Dispose() }
}
finally { $stream.Dispose() }

if (-not (Test-Path -LiteralPath $archive -PathType Leaf) -or (Get-Item -LiteralPath $archive).Length -lt 1000000) {
    throw 'Release archive was not created or is unexpectedly small.'
}
Write-Host "Built: $archive"
