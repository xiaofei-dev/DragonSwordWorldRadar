#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
if ($PSVersionTable.PSVersion.Major -ne 5 -or
    ($PSVersionTable.PSEdition -and $PSVersionTable.PSEdition -ne 'Desktop')) {
    throw 'Test-Refactor.ps1 must run in Windows PowerShell 5.1.'
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$sources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.Name -ne 'Program.cs' -and $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' } |
    Sort-Object FullName | Select-Object -ExpandProperty FullName)
$sources += (Join-Path $PSScriptRoot 'ValidationHarness.cs')
$sources += (Join-Path $root 'src\installer\Core\Generation\OwnerPointerRvaResolver.cs')
$references = @(
    [System.Windows.Forms.Form].Assembly.Location,
    [System.Drawing.Graphics].Assembly.Location,
    [System.Linq.Enumerable].Assembly.Location,
    [System.Uri].Assembly.Location
) | Select-Object -Unique
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ('DragonSwordWorldRadar-Tests-' + [Guid]::NewGuid().ToString('N'))
$testExe = Join-Path $tempRoot 'DragonSwordWorldRadar.RefactorTests.exe'
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
try {
    Add-Type -Path $sources -ReferencedAssemblies $references `
        -OutputAssembly $testExe -OutputType ConsoleApplication -ErrorAction Stop
    $output = & cmd.exe /d /c ('"' + $testExe + '" 2>&1')
    $exitCode = $LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if ($exitCode -ne 0 -or ($output -join "`n") -notmatch 'REFACTOR_TESTS_OK') {
        throw "Refactor validation harness failed with exit code $exitCode."
    }
}
finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
& (Join-Path $PSScriptRoot 'Test-PerformanceScheduling.ps1')
