[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$UE4SSRoot,
    [Parameter(Mandatory)][string]$ImGuiColorTextEditRoot,
    [string]$CMakePath,
    [string]$NinjaPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe',
    [string]$VsDevCmdPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat',
    [Parameter(Mandatory)][string]$RustupHome,
    [Parameter(Mandatory)][string]$CargoHome
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$expected = @{
    UE4SS = 'd935b5b23bac03b65c14ae38382b02007204cc2e'
    UEPseudo = 'd09b7218bfe7392adeffb500fdeee0b42ca1cd27'
    PatternSleuth = '33e731e99f2a6bb7f65a8e95e89fd1c06ce9d1d2'
    ImGuiColorTextEdit = 'af7821926251feca84e35f8fa83eee84dae90424'
}

function Resolve-Required([string]$Path, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "$Label not found: $Path" }
    (Resolve-Path -LiteralPath $Path).Path
}
function Assert-Commit([string]$Path, [string]$Commit, [string]$Label) {
    $actual = (& git -C $Path rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Commit) { throw "$Label must be pinned to $Commit; found $actual" }
    if (& git -C $Path status --porcelain --untracked-files=no) { throw "$Label checkout is dirty" }
}

$ue4ss = Resolve-Required $UE4SSRoot 'RE-UE4SS'
$imgui = Resolve-Required $ImGuiColorTextEditRoot 'ImGuiColorTextEdit'
$unreal = Resolve-Required (Join-Path $ue4ss 'deps\first\Unreal') 'UEPseudo'
$patterns = Resolve-Required (Join-Path $ue4ss 'deps\first\patternsleuth') 'patternsleuth'
Assert-Commit $ue4ss $expected.UE4SS 'RE-UE4SS'
Assert-Commit $unreal $expected.UEPseudo 'UEPseudo'
Assert-Commit $patterns $expected.PatternSleuth 'patternsleuth'
Assert-Commit $imgui $expected.ImGuiColorTextEdit 'ImGuiColorTextEdit'

if (-not $CMakePath) {
    $CMakePath = Join-Path (Split-Path -Parent $RustupHome) 'cmake-3.29.6\cmake-3.29.6-windows-x86_64\bin\cmake.exe'
}
$cmake = Resolve-Required $CMakePath 'CMake'
$ninja = Resolve-Required $NinjaPath 'Ninja'
$vsDevCmd = Resolve-Required $VsDevCmdPath 'VsDevCmd'
$rustBin = Resolve-Required (Join-Path $RustupHome 'toolchains\1.73.0-x86_64-pc-windows-msvc\bin') 'Rust 1.73.0'
$rustc = Resolve-Required (Join-Path $rustBin 'rustc.exe') 'rustc'
$cargo = Resolve-Required (Join-Path $rustBin 'cargo.exe') 'cargo'

$devEnvironment = & $env:ComSpec /d /s /c "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.38 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio environment setup failed' }
foreach ($line in $devEnvironment) {
    if ($line -match '^([^=][^=]*)=(.*)$') { Set-Item -Path "Env:$($matches[1])" -Value $matches[2] }
}
$env:RUSTUP_HOME = (Resolve-Path -LiteralPath $RustupHome).Path
$env:CARGO_HOME = (Resolve-Path -LiteralPath $CargoHome).Path
$env:PATH = "$rustBin;$(Join-Path $env:CARGO_HOME 'bin');$(Split-Path -Parent $ninja);$env:PATH"
$env:GIT_CONFIG_COUNT = '2'
$env:GIT_CONFIG_KEY_0 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_0 = 'git@github.com:'
$env:GIT_CONFIG_KEY_1 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_1 = 'ssh://git@github.com/'

$build = Join-Path $projectRoot 'build-native'
& $cmake --fresh -S $projectRoot -B $build -G Ninja `
    '-DCMAKE_BUILD_TYPE=Game__Shipping__Win64' "-DCMAKE_MAKE_PROGRAM=$ninja" `
    '-DDSNWR_BUILD_UE4SS=ON' "-DUE4SS_ROOT=$ue4ss" `
    "-DRust_COMPILER=$rustc" "-DRust_CARGO=$cargo" '-DRust_RESOLVE_RUSTUP_TOOLCHAINS=OFF' `
    "-DFETCHCONTENT_SOURCE_DIR_IMGUITEXTEDIT=$imgui"
if ($LASTEXITCODE -ne 0) { throw "Native configure failed: $LASTEXITCODE" }
& $cmake --build $build --target DragonSwordNativeWorldRadar
if ($LASTEXITCODE -ne 0) { throw "Native build failed: $LASTEXITCODE" }
$dll = Get-ChildItem -LiteralPath $build -Recurse -Filter main.dll -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $dll) { throw 'Build produced no main.dll' }
Write-Host "NATIVE_BUILD_OK path=$($dll.FullName) sha256=$((Get-FileHash -Algorithm SHA256 -LiteralPath $dll.FullName).Hash)"
