#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
if ($PSVersionTable.PSVersion.Major -ne 5 -or
    ($PSVersionTable.PSEdition -and $PSVersionTable.PSEdition -ne 'Desktop')) {
    throw 'Compile-Source.ps1 must run in Windows PowerShell 5.1, matching Install.cmd and the runtime host.'
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$overlaySources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File |
    Sort-Object FullName | Select-Object -ExpandProperty FullName)
if ($overlaySources.Count -ne 31) {
    throw "Overlay source set must contain exactly 31 files; found $($overlaySources.Count)."
}
$overlayReferences = @(
    [System.Windows.Forms.Form].Assembly.Location,
    [System.Drawing.Graphics].Assembly.Location,
    [System.Linq.Enumerable].Assembly.Location,
    [System.Uri].Assembly.Location
) | Select-Object -Unique
Add-Type -Path $overlaySources -ReferencedAssemblies $overlayReferences -ErrorAction Stop
Write-Host "OVERLAY_COMPILE_OK sources=$($overlaySources.Count)"

Add-Type -AssemblyName System.Xml
$installerSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File |
    Sort-Object FullName | Select-Object -ExpandProperty FullName)
if ($installerSources.Count -ne 13) {
    throw "Installer source set must contain exactly 13 files; found $($installerSources.Count)."
}
$installerReferences = @(
    [System.Xml.XmlDocument].Assembly.Location,
    [System.Security.Cryptography.Aes].Assembly.Location,
    [System.Linq.Enumerable].Assembly.Location,
    [System.Uri].Assembly.Location
) | Select-Object -Unique
Add-Type -Path $installerSources -ReferencedAssemblies $installerReferences -ErrorAction Stop
Write-Host "INSTALLER_COMPILE_OK sources=$($installerSources.Count)"
