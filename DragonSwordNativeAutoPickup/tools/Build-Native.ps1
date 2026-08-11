[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$UE4SSRoot,
    [string]$ImGuiColorTextEditRoot,
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$VsDevCmdPath,
    [string]$RustupHome,
    [string]$CargoHome,
    [ValidateSet('Game__Shipping__Win64')]
    [string]$Configuration = 'Game__Shipping__Win64'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$sdkRoot = Join-Path $projectRoot '.sdk'

$expectedCommits = @{
    UE4SS = '1c1a1497f942c707f47ba668db75b25e86f6c08a'
    UEPseudo = 'b2e876da82b17254c04304746341c8fde0ddb37c'
    PatternSleuth = 'da8bfe4c5a464be0ef225c2c9a6ccaa2d9284018'
    ImGuiColorTextEdit = '6d943aba9f7cef05da80b86dbb0253b63818f95c'
}

function Resolve-RequiredPath {
    param([string]$Path, [string]$Description)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Assert-GitCommit {
    param([string]$Repository, [string]$Expected, [string]$Description)
    $actual = (& git -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "$Description must be pinned to $Expected; found $actual"
    }
    $status = (& git -C $Repository status --porcelain --untracked-files=no)
    if ($LASTEXITCODE -ne 0 -or $status) {
        throw "$Description checkout is not clean. Third-party source edits are not accepted."
    }
}

$resolvedUE4SS = Resolve-RequiredPath $UE4SSRoot 'RE-UE4SS source tree'
if (-not $ImGuiColorTextEditRoot) {
    $ImGuiColorTextEditRoot = Join-Path $sdkRoot 'ImGuiColorTextEdit'
}
$resolvedImGui = Resolve-RequiredPath $ImGuiColorTextEditRoot 'ImGuiColorTextEdit source tree'

$unrealRoot = Resolve-RequiredPath (Join-Path $resolvedUE4SS 'deps\first\Unreal') 'UEPseudo submodule'
$patternSleuthRoot = Resolve-RequiredPath (Join-Path $resolvedUE4SS 'deps\first\patternsleuth') 'patternsleuth submodule'
Assert-GitCommit $resolvedUE4SS $expectedCommits.UE4SS 'RE-UE4SS'
Assert-GitCommit $unrealRoot $expectedCommits.UEPseudo 'UEPseudo'
Assert-GitCommit $patternSleuthRoot $expectedCommits.PatternSleuth 'patternsleuth'
Assert-GitCommit $resolvedImGui $expectedCommits.ImGuiColorTextEdit 'ImGuiColorTextEdit'

if (-not $CMakePath) {
    $CMakePath = Join-Path $sdkRoot 'cmake-3.29.6\cmake-3.29.6-windows-x86_64\bin\cmake.exe'
}
$cmake = Resolve-RequiredPath $CMakePath 'CMake 3.29.6'
$cmakeVersion = (& $cmake --version | Select-Object -First 1)
if ($cmakeVersion -notmatch '^cmake version 3\.29\.6$') {
    throw "CMake 3.29.6 is required for the pinned Corrosion revision; found $cmakeVersion"
}

if (-not $NinjaPath) {
    $NinjaPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
}
$ninja = Resolve-RequiredPath $NinjaPath 'Ninja'

if (-not $VsDevCmdPath) {
    $VsDevCmdPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
}
$vsDevCmd = Resolve-RequiredPath $VsDevCmdPath 'Visual Studio 2022 developer command script'

if (-not $RustupHome) { $RustupHome = Join-Path $sdkRoot 'rustup' }
if (-not $CargoHome) { $CargoHome = Join-Path $sdkRoot 'cargo' }
$resolvedRustupHome = Resolve-RequiredPath $RustupHome 'Project-local rustup home'
$resolvedCargoHome = Resolve-RequiredPath $CargoHome 'Project-local Cargo home'
$rustToolchainBin = Resolve-RequiredPath (Join-Path $resolvedRustupHome 'toolchains\stable-x86_64-pc-windows-msvc\bin') 'Rust 1.97.1 toolchain'
$rustCompiler = Resolve-RequiredPath (Join-Path $rustToolchainBin 'rustc.exe') 'Rust 1.97.1 compiler'
$rustCargo = Resolve-RequiredPath (Join-Path $rustToolchainBin 'cargo.exe') 'Cargo 1.97.1'

$devEnvironment = & $env:ComSpec /d /s /c "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.44 >nul && set"
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio 2022 environment setup failed: $LASTEXITCODE"
}
foreach ($line in $devEnvironment) {
    if ($line -match '^([^=][^=]*)=(.*)$') {
        Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
    }
}

$env:RUSTUP_HOME = $resolvedRustupHome
$env:CARGO_HOME = $resolvedCargoHome
$env:PATH = "$rustToolchainBin;$(Join-Path $resolvedCargoHome 'bin');$(Split-Path -Parent $ninja);$env:PATH"
$rustVersion = (& $rustCompiler --version).Trim()
if ($LASTEXITCODE -ne 0 -or $rustVersion -notmatch '^rustc 1\.97\.1 ') {
    throw "Rust 1.97.1 is required for the pinned UE4SS lockfile v4 build; found $rustVersion"
}
$compilerCommand = Get-Command cl.exe -ErrorAction Stop
$compilerVersion = (Get-Item -LiteralPath $compilerCommand.Source).VersionInfo.FileVersion
if ($compilerVersion -notmatch '^19\.44\.') {
    throw 'MSVC 19.44 selected through -vcvars_ver=14.44 is required.'
}

# Upstream fetch declarations use both SSH spellings. Keep translation local to
# this PowerShell process; no global or repository Git configuration is changed.
$env:GIT_CONFIG_COUNT = '2'
$env:GIT_CONFIG_KEY_0 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_0 = 'git@github.com:'
$env:GIT_CONFIG_KEY_1 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_1 = 'ssh://git@github.com/'

$buildDirectory = Join-Path $projectRoot 'build-native'
& $cmake --fresh -S $projectRoot -B $buildDirectory -G Ninja `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    '-DDSNAP_BUILD_TESTS=ON' `
    '-DDSNAP_BUILD_UE4SS=ON' `
    "-DUE4SS_ROOT=$resolvedUE4SS" `
    "-DRust_COMPILER=$rustCompiler" `
    "-DRust_CARGO=$rustCargo" `
    '-DRust_RESOLVE_RUSTUP_TOOLCHAINS=OFF' `
    "-DFETCHCONTENT_SOURCE_DIR_IMGUITEXTEDIT=$resolvedImGui"
if ($LASTEXITCODE -ne 0) { throw "Native configure failed: $LASTEXITCODE" }

& $cmake --build $buildDirectory --target DragonSwordNativeAutoPickup
if ($LASTEXITCODE -ne 0) { throw "Native build failed: $LASTEXITCODE" }

$dll = Get-ChildItem -LiteralPath $buildDirectory -Recurse -Filter main.dll -File |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $dll) { throw 'Native build completed without producing main.dll.' }
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $dll.FullName).Hash
Write-Host "Native adapter build passed: $($dll.FullName)"
Write-Host "SHA-256: $hash"
Write-Host 'This build is not deployment or in-game acceptance.'
