[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("dsnap-package-test-" + [Guid]::NewGuid().ToString('N'))
$inputRoot = Join-Path $tempRoot 'input'
$outputRoot = Join-Path $tempRoot 'output'
try {
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    $dummyDll = Join-Path $inputRoot 'main.dll'
    [System.IO.File]::WriteAllBytes($dummyDll, [byte[]](0x4D, 0x5A))
    & (Join-Path $PSScriptRoot 'Stage-Package.ps1') -DllPath $dummyDll -OutputDirectory $outputRoot
    $requiredScript = Join-Path $outputRoot 'ue4ss\Mods\DragonSwordNativeAutoPickup\Scripts\main.lua'
    if (-not (Test-Path -LiteralPath $requiredScript)) { throw 'Marker-only Lua entry point is missing.' }
    $forbidden = Get-ChildItem -LiteralPath $outputRoot -Recurse -File | Where-Object {
        $_.Name -match 'MnMRadar|reference|UE4SS\.dll|DSClient'
    }
    if ($forbidden) { throw 'Package contains forbidden reference or runtime files.' }
    Write-Host 'Native-pulse package layout test passed.'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) { Remove-Item -LiteralPath $tempRoot -Recurse -Force }
}
