[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$UE4SSRoot,
    [string]$SdkRoot,
    [string]$ImGuiColorTextEditRoot,
    [string]$IconFontCppHeadersRoot,
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$VsDevCmdPath,
    [string]$RustupHome,
    [string]$CargoHome,
    [string]$BuildDirectory,
    [ValidateSet('Game__Shipping__Win64')]
    [string]$Configuration = 'Game__Shipping__Win64'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $SdkRoot) { $SdkRoot = Join-Path $projectRoot '.sdk' }
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $projectRoot 'dist\work\build\native-stable'
}

$expectedCommits = @{
    UE4SS = 'd935b5b23bac03b65c14ae38382b02007204cc2e'
    UEPseudo = 'd09b7218bfe7392adeffb500fdeee0b42ca1cd27'
    PatternSleuth = '33e731e99f2a6bb7f65a8e95e89fd1c06ce9d1d2'
    ImGuiColorTextEdit = 'af7821926251feca84e35f8fa83eee84dae90424'
    IconFontCppHeaders = '210b5a399a64270674560d633638952d1e8d804d'
}

function Resolve-RequiredPath {
    param([string]$Path, [string]$Description)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Assert-PinnedGitTree {
    param([string]$Repository, [string]$Expected, [string]$Description)
    $actual = (& git -c "safe.directory=$Repository" -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "$Description must be pinned to $Expected; found $actual"
    }
    & git -c "safe.directory=$Repository" -C $Repository diff --quiet --ignore-submodules=all --
    $worktree = $LASTEXITCODE
    & git -c "safe.directory=$Repository" -C $Repository diff --cached --quiet --ignore-submodules=all --
    $index = $LASTEXITCODE
    if ($worktree -ne 0 -or $index -ne 0) {
        throw "$Description checkout is not clean."
    }
}

$resolvedSdk = Resolve-RequiredPath $SdkRoot 'SDK root'
$resolvedUE4SS = Resolve-RequiredPath $UE4SSRoot 'official RE-UE4SS v3.0.1 source tree'
if (-not $ImGuiColorTextEditRoot) {
    $ImGuiColorTextEditRoot = Join-Path $resolvedSdk 'ImGuiColorTextEdit-v3.0.1'
}
if (-not $IconFontCppHeadersRoot) {
    $IconFontCppHeadersRoot = Join-Path $resolvedSdk 'IconFontCppHeaders-v3.0.1'
}
$resolvedImGui = Resolve-RequiredPath $ImGuiColorTextEditRoot 'pinned ImGuiColorTextEdit source tree'
$resolvedIconFont = Resolve-RequiredPath $IconFontCppHeadersRoot 'pinned IconFontCppHeaders source tree'
$unrealRoot = Resolve-RequiredPath (Join-Path $resolvedUE4SS 'deps\first\Unreal') 'UEPseudo submodule'
$patternSleuthRoot = Resolve-RequiredPath (Join-Path $resolvedUE4SS 'deps\first\patternsleuth') 'patternsleuth submodule'

Assert-PinnedGitTree $resolvedUE4SS $expectedCommits.UE4SS 'RE-UE4SS v3.0.1'
Assert-PinnedGitTree $unrealRoot $expectedCommits.UEPseudo 'UEPseudo'
Assert-PinnedGitTree $patternSleuthRoot $expectedCommits.PatternSleuth 'patternsleuth'
Assert-PinnedGitTree $resolvedImGui $expectedCommits.ImGuiColorTextEdit 'ImGuiColorTextEdit'
Assert-PinnedGitTree $resolvedIconFont $expectedCommits.IconFontCppHeaders 'IconFontCppHeaders'

if (-not $CMakePath) {
    $CMakePath = Join-Path $resolvedSdk 'cmake-3.29.6\cmake-3.29.6-windows-x86_64\bin\cmake.exe'
}
$cmake = Resolve-RequiredPath $CMakePath 'CMake 3.29.6'
$cmakeVersion = (& $cmake --version | Select-Object -First 1)
if ($cmakeVersion -notmatch '^cmake version 3\.29\.6$') {
    throw "CMake 3.29.6 is required; found $cmakeVersion"
}
if (-not $NinjaPath) {
    $NinjaPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
}
$ninja = Resolve-RequiredPath $NinjaPath 'Ninja'
$ninjaVersion = (& $ninja --version).Trim()
if (-not $VsDevCmdPath) {
    $VsDevCmdPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
}
$vsDevCmd = Resolve-RequiredPath $VsDevCmdPath 'Visual Studio 2022 developer command script'
if (-not $RustupHome) { $RustupHome = Join-Path $resolvedSdk 'rustup' }
if (-not $CargoHome) { $CargoHome = Join-Path $resolvedSdk 'cargo' }
$resolvedRustup = Resolve-RequiredPath $RustupHome 'Rustup home'
$resolvedCargo = Resolve-RequiredPath $CargoHome 'Cargo home'
$rustBin = Resolve-RequiredPath `
    (Join-Path $resolvedRustup 'toolchains\1.73.0-x86_64-pc-windows-msvc\bin') `
    'Rust 1.73.0 toolchain'
$rustCompiler = Resolve-RequiredPath (Join-Path $rustBin 'rustc.exe') 'Rust 1.73.0 compiler'
$rustCargo = Resolve-RequiredPath (Join-Path $rustBin 'cargo.exe') 'Cargo 1.73.0'

$devEnvironment = & $env:ComSpec /d /s /c `
    "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=14.38 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio 2022 environment setup failed.' }
$seen = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
foreach ($line in $devEnvironment) {
    if ($line -match '^([^=][^=]*)=(.*)$' -and $seen.Add($matches[1])) {
        Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
    }
}
$env:RUSTUP_HOME = $resolvedRustup
$env:CARGO_HOME = $resolvedCargo
$env:PATH = "$rustBin;$(Join-Path $resolvedCargo 'bin');$(Split-Path -Parent $ninja);$env:PATH"
$rustVersion = (& $rustCompiler --version).Trim()
if ($rustVersion -notmatch '^rustc 1\.73\.0 ') {
    throw "Rust 1.73.0 is required; found $rustVersion"
}
$compilerVersion = (Get-Item (Get-Command cl.exe -ErrorAction Stop).Source).VersionInfo.FileVersion
if ($compilerVersion -notmatch '^19\.38\.') {
    throw "MSVC 19.38 is required; found $compilerVersion"
}

$env:GIT_CONFIG_COUNT = '2'
$env:GIT_CONFIG_KEY_0 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_0 = 'git@github.com:'
$env:GIT_CONFIG_KEY_1 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_1 = 'ssh://git@github.com/'

$buildFull = [System.IO.Path]::GetFullPath($BuildDirectory)
$patternLock = Join-Path $resolvedUE4SS 'deps\first\patternsleuth_bind\Cargo.lock'
$patternLockBytes = [System.IO.File]::ReadAllBytes($patternLock)
$previousPreference = $ErrorActionPreference
try {
    # Pinned CMake dependencies emit non-fatal status text on stderr. Windows
    # PowerShell promotes that text to NativeCommandError under Stop, so native
    # tool success is determined by its exit code below.
    $ErrorActionPreference = 'Continue'
    & $cmake --fresh -S $projectRoot -B $buildFull -G Ninja `
        "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DCMAKE_MAKE_PROGRAM=$ninja" `
        '-DDSNWRPR_BUILD_UE4SS=ON' `
        '-DDSNWRPR_UE4SS_VARIANT=StableRoot' `
        "-DUE4SS_ROOT=$resolvedUE4SS" `
        "-DRust_COMPILER=$rustCompiler" `
        "-DRust_CARGO=$rustCargo" `
        '-DRust_RESOLVE_RUSTUP_TOOLCHAINS=OFF' `
        "-DFETCHCONTENT_SOURCE_DIR_IMGUITEXTEDIT=$resolvedImGui" `
        "-DFETCHCONTENT_SOURCE_DIR_ICONFONTCPPHEADERS=$resolvedIconFont"
    if ($LASTEXITCODE -ne 0) { throw "Stable native configure failed: $LASTEXITCODE" }
    & $cmake --build $buildFull --target DragonSwordNativeWorldRadarPostRender
    if ($LASTEXITCODE -ne 0) { throw "Stable native build failed: $LASTEXITCODE" }
} finally {
    $ErrorActionPreference = $previousPreference
    [System.IO.File]::WriteAllBytes($patternLock, $patternLockBytes)
}

Assert-PinnedGitTree $resolvedUE4SS $expectedCommits.UE4SS 'RE-UE4SS v3.0.1 after build'
Assert-PinnedGitTree $unrealRoot $expectedCommits.UEPseudo 'UEPseudo after build'
Assert-PinnedGitTree $patternSleuthRoot $expectedCommits.PatternSleuth 'patternsleuth after build'
Assert-PinnedGitTree $resolvedImGui $expectedCommits.ImGuiColorTextEdit 'ImGuiColorTextEdit after build'
Assert-PinnedGitTree $resolvedIconFont $expectedCommits.IconFontCppHeaders 'IconFontCppHeaders after build'

$dllPath = Join-Path $buildFull 'main.dll'
if (-not (Test-Path -LiteralPath $dllPath -PathType Leaf)) {
    throw "Stable native build completed without main.dll: $dllPath"
}
. (Join-Path $PSScriptRoot 'NativeBuildReceipt.ps1')
New-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot -DllPath $dllPath `
    -ReceiptPath (Join-Path $buildFull 'native-build-receipt.json') `
    -Configuration $Configuration -CMakeVersion $cmakeVersion `
    -NinjaVersion $ninjaVersion -CompilerVersion $compilerVersion `
    -RustVersion $rustVersion -UE4SSVariant StableRoot `
    -BuildScriptPath $PSCommandPath `
    -BuildLockPath (Join-Path $projectRoot 'metadata\native-build-stable-lock.json') | Out-Null

[pscustomobject]@{
    variant = 'StableRoot'
    dll = $dllPath
    sha256 = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash
    receipt = Join-Path $buildFull 'native-build-receipt.json'
    runtime_acceptance = 'NOT_VALIDATED'
}
