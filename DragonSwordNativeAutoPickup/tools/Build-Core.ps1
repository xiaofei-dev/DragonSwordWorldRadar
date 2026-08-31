[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$cmake = Join-Path $projectRoot '.sdk\cmake-3.29.6\cmake-3.29.6-windows-x86_64\bin\cmake.exe'
$ninja = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$vsDevCmd = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
foreach ($required in @($cmake, $ninja, $vsDevCmd)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required core-build dependency was not found: $required"
    }
}
$devEnvironment = & $env:ComSpec /d /s /c "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.44 >nul && set"
if ($LASTEXITCODE -ne 0) { throw "Visual Studio 2022 environment setup failed: $LASTEXITCODE" }
$seenEnvironmentNames = [Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase)
foreach ($line in $devEnvironment) {
    if ($line -match '^([^=][^=]*)=(.*)$') {
        $name = $matches[1]
        if ($seenEnvironmentNames.Add($name)) {
            Set-Item -Path "Env:$name" -Value $matches[2]
        }
    }
}
$compiler = (Get-Command cl.exe -ErrorAction Stop).Source
$buildDirectory = Join-Path $projectRoot 'out\core'
& $cmake --fresh -S $projectRoot -B $buildDirectory -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DCMAKE_CXX_COMPILER=$compiler" `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    -DDSNAP_BUILD_TESTS=ON -DDSNAP_BUILD_UE4SS=OFF
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
& $cmake --build $buildDirectory
if ($LASTEXITCODE -ne 0) { throw "Core build failed: $LASTEXITCODE" }
& $cmake --build $buildDirectory --target test
if ($LASTEXITCODE -ne 0) { throw "Core tests failed: $LASTEXITCODE" }

Write-Host 'Core build and tests passed.'
