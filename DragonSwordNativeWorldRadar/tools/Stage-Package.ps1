[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$DllPath,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$dll = (Resolve-Path -LiteralPath $DllPath).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
$modRoot = Join-Path $output 'Mods\DragonSwordNativeWorldRadar'
$expectedRoot = [IO.Path]::GetFullPath((Join-Path $output 'Mods\DragonSwordNativeWorldRadar'))
if (-not $modRoot.Equals($expectedRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not $modRoot.StartsWith(($output.TrimEnd('\') + '\'), [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe package target: $modRoot"
}
if (Test-Path -LiteralPath $modRoot) { Remove-Item -LiteralPath $modRoot -Recurse -Force }
New-Item -ItemType Directory -Path (Join-Path $modRoot 'dlls') -Force | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $modRoot 'dlls\main.dll')
Copy-Item -LiteralPath (Join-Path $projectRoot 'config\default.ini') -Destination (Join-Path $modRoot 'config.ini')
Copy-Item -LiteralPath (Join-Path $projectRoot 'config\enabled.txt') -Destination (Join-Path $modRoot 'enabled.txt')
Copy-Item -LiteralPath (Join-Path $projectRoot 'scripts') -Destination (Join-Path $modRoot 'scripts') -Recurse
Copy-Item -LiteralPath (Join-Path $projectRoot 'host') -Destination (Join-Path $modRoot 'host') -Recurse
New-Item -ItemType Directory -Path (Join-Path $modRoot 'src') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\overlay') -Destination (Join-Path $modRoot 'src\overlay') -Recurse
Copy-Item -LiteralPath (Join-Path $projectRoot 'assets\data') -Destination (Join-Path $modRoot 'data') -Recurse
Copy-Item -LiteralPath (Join-Path $projectRoot 'assets\vendor') -Destination (Join-Path $modRoot 'vendor') -Recurse

$required = @(
    'dlls\main.dll', 'config.ini', 'enabled.txt',
    'host\DragonSwordNativeWorldRadar.Host.ps1',
    'src\overlay\Program.cs', 'data\generated\treasures.lua',
    'data\generated\bosses.lua', 'data\generated\assaults.lua',
    'data\generated\moles.lua', 'vendor\sqlcipher\e_sqlcipher.dll'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $modRoot $relative))) { throw "Missing package file: $relative" }
}
Write-Host "PACKAGE_STAGE_OK root=$modRoot files=$((Get-ChildItem -LiteralPath $modRoot -Recurse -File).Count)"
