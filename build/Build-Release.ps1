$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot 'Verify-Source.ps1')
$release = Get-Content -LiteralPath (Join-Path $root 'metadata\release.json') -Raw | ConvertFrom-Json
$version = [string]$release.version
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist ("DragonSwordWorldRadar-$version")
$mod = Join-Path $stage 'DragonSwordWorldRadar'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $mod | Out-Null

function Copy-Tree([string]$Source,[string]$Destination) {
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Copy-Item -Path (Join-Path $Source '*') -Destination $Destination -Recurse -Force
}
function Copy-One([string]$Source,[string]$Destination) {
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
[IO.File]::WriteAllText((Join-Path $mod 'enabled.txt'),"1`n",[Text.UTF8Encoding]::new($false))
New-Item -ItemType Directory -Force -Path (Join-Path $mod 'data\generated'),(Join-Path $mod 'runtime\bridge'),(Join-Path $mod 'runtime\logs') | Out-Null

$manifestFiles = @()
foreach ($file in @(Get-ChildItem -LiteralPath $mod -Recurse -File | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($mod.Length).TrimStart('\')
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
    files = $manifestFiles
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $mod 'metadata\build-manifest.json') -Encoding UTF8

$archive = Join-Path $dist ("DragonSwordWorldRadar-v$version.zip")
if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
Compress-Archive -LiteralPath $mod -DestinationPath $archive -CompressionLevel Optimal
Write-Host "Built: $archive"
