[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$CMakePath,
    [string]$NinjaPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe',
    [string]$VsDevCmdPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$cmakeCandidates = @(@(
    $CMakePath,
    (Get-Command cmake.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1),
    'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })

if (-not $cmakeCandidates) {
    throw 'CMake was not found.'
}

$cmake = $cmakeCandidates[0]
$ninja = (Resolve-Path -LiteralPath $NinjaPath -ErrorAction Stop).Path
$vsDevCmd = (Resolve-Path -LiteralPath $VsDevCmdPath -ErrorAction Stop).Path
$environmentSnapshot =
    [System.Collections.Generic.Dictionary[string, string]]::new(
        [System.StringComparer]::Ordinal)
foreach ($entry in [Environment]::GetEnvironmentVariables(
        [EnvironmentVariableTarget]::Process).GetEnumerator()) {
    $environmentSnapshot[[string]$entry.Key] = [string]$entry.Value
}
try {
$devEnvironment = & $env:ComSpec /d /s /c "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.44 >nul && set"
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio 2022 environment setup failed: $LASTEXITCODE"
}
foreach ($line in $devEnvironment) {
    if ($line -match '^([^=][^=]*)=(.*)$') {
        $preserveEmptySnapshot =
            $environmentSnapshot.ContainsKey($matches[1]) `
            -and $environmentSnapshot[$matches[1]].Length -eq 0
        if ($matches[1] -ine 'Path' -and $matches[2].Length -gt 0 `
            -and -not $preserveEmptySnapshot) {
            Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
        }
    }
}
$developerPath = $devEnvironment | Where-Object {
    $_ -match '^(?i:PATH)=.*\\VC\\Tools\\MSVC\\14\.44\.'
} | Select-Object -First 1
if (-not $developerPath) {
    throw 'Visual Studio 2022 environment did not publish an MSVC 14.44 compiler path.'
}
$env:PATH = $developerPath.Substring($developerPath.IndexOf('=') + 1)

$buildDirectory = Join-Path $projectRoot 'dist\work\build\native-core'
& $cmake --fresh -S $projectRoot -B $buildDirectory -G Ninja `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    '-DDSNWRPR_BUILD_UE4SS=OFF'
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
& $cmake --build $buildDirectory --config $Configuration --clean-first
if ($LASTEXITCODE -ne 0) { throw "Core build failed: $LASTEXITCODE" }
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest)) {
    throw "CTest was not found beside CMake: $ctest"
}
& $ctest --test-dir $buildDirectory --output-on-failure -C $Configuration
if ($LASTEXITCODE -ne 0) { throw "Core tests failed: $LASTEXITCODE" }

Write-Host 'Core build and tests passed.'
}
finally {
    foreach ($entry in [Environment]::GetEnvironmentVariables(
            [EnvironmentVariableTarget]::Process).GetEnumerator()) {
        $name = [string]$entry.Key
        if (-not $environmentSnapshot.ContainsKey($name)) {
            [Environment]::SetEnvironmentVariable(
                $name, $null, [EnvironmentVariableTarget]::Process)
        }
    }
    foreach ($name in $environmentSnapshot.Keys) {
        if ($environmentSnapshot[$name].Length -eq 0) {
            continue
        }
        [Environment]::SetEnvironmentVariable(
            $name, $environmentSnapshot[$name],
            [EnvironmentVariableTarget]::Process)
    }
}
