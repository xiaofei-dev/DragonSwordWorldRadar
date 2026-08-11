[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$DllPath,
    [Parameter(Mandatory)]
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$resolvedDll = (Resolve-Path -LiteralPath $DllPath).Path
$resolvedOutput = [System.IO.Path]::GetFullPath($OutputDirectory)
if ([System.IO.Path]::GetFileName($resolvedDll) -ne 'main.dll') {
    throw 'The UE4SS native output must be named main.dll.'
}

$modRoot = Join-Path $resolvedOutput 'ue4ss\Mods\DragonSwordNativeAutoPickup'
$dllDirectory = Join-Path $modRoot 'dlls'
$scriptsDirectory = Join-Path $modRoot 'Scripts'
New-Item -ItemType Directory -Path $dllDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $scriptsDirectory -Force | Out-Null
Copy-Item -LiteralPath $resolvedDll -Destination (Join-Path $dllDirectory 'main.dll') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'Scripts\main.lua') -Destination (Join-Path $scriptsDirectory 'main.lua') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'config\default.ini') -Destination (Join-Path $modRoot 'config.ini') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'config\enabled.txt') -Destination (Join-Path $modRoot 'enabled.txt') -Force

$files = Get-ChildItem -LiteralPath $resolvedOutput -Recurse -File | ForEach-Object {
    $_.FullName.Substring($resolvedOutput.Length).TrimStart('\').Replace('\', '/')
}
$expected = @(
    'ue4ss/Mods/DragonSwordNativeAutoPickup/config.ini',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/enabled.txt',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/dlls/main.dll',
    'ue4ss/Mods/DragonSwordNativeAutoPickup/Scripts/main.lua'
)
$difference = Compare-Object -ReferenceObject $expected -DifferenceObject $files
if ($difference) { throw "Unexpected package contents: $($difference | Out-String)" }

Write-Host "Package staged at $resolvedOutput"
