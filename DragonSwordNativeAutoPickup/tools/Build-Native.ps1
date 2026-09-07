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
    [ValidateSet('ExperimentalNested')]
    [string]$UE4SSVariant = 'ExperimentalNested',
    [string]$BuildDirectory,
    [switch]$OfflineDependencies,
    [ValidateSet('Game__Shipping__Win64')]
    [string]$Configuration = 'Game__Shipping__Win64'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$sdkRoot = Join-Path $projectRoot '.sdk'

$expectedCommits = if ($UE4SSVariant -eq 'StableRoot') { @{
    UE4SS = 'd935b5b23bac03b65c14ae38382b02007204cc2e'
    UEPseudo = 'd09b7218bfe7392adeffb500fdeee0b42ca1cd27'
    PatternSleuth = '33e731e99f2a6bb7f65a8e95e89fd1c06ce9d1d2'
    ImGuiColorTextEdit = 'af7821926251feca84e35f8fa83eee84dae90424'
} } else { @{
    UE4SS = '1c1a1497f942c707f47ba668db75b25e86f6c08a'
    UEPseudo = 'b2e876da82b17254c04304746341c8fde0ddb37c'
    PatternSleuth = 'da8bfe4c5a464be0ef225c2c9a6ccaa2d9284018'
    ImGuiColorTextEdit = '6d943aba9f7cef05da80b86dbb0253b63818f95c'
} }

function Resolve-RequiredPath {
    param([string]$Path, [string]$Description)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Assert-GitCommit {
    param([string]$Repository, [string]$Expected, [string]$Description)
    $actual = (& git -c "safe.directory=$Repository" -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "$Description must be pinned to $Expected; found $actual"
    }
    & git -c "safe.directory=$Repository" -C $Repository diff --quiet --ignore-submodules=all --
    $worktreeDiff = $LASTEXITCODE
    & git -c "safe.directory=$Repository" -C $Repository diff --cached --quiet --ignore-submodules=all --
    $indexDiff = $LASTEXITCODE
    if ($worktreeDiff -ne 0 -or $indexDiff -ne 0) {
        throw "$Description checkout is not clean. Third-party source edits are not accepted."
    }
}

$resolvedUE4SS = Resolve-RequiredPath $UE4SSRoot 'RE-UE4SS source tree'
if (-not $ImGuiColorTextEditRoot) {
    $imguiLeaf = if ($UE4SSVariant -eq 'StableRoot') {
        'ImGuiColorTextEdit-v3.0.1'
    } else {
        'ImGuiColorTextEdit'
    }
    $ImGuiColorTextEditRoot = Join-Path $sdkRoot $imguiLeaf
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
$rustToolchainName = if ($UE4SSVariant -eq 'StableRoot') {
    '1.73.0-x86_64-pc-windows-msvc'
} else {
    'stable-x86_64-pc-windows-msvc'
}
$rustDescription = if ($UE4SSVariant -eq 'StableRoot') { 'Rust 1.73.0' } else { 'Rust 1.97.1' }
$rustToolchainBin = Resolve-RequiredPath `
    (Join-Path $resolvedRustupHome "toolchains\$rustToolchainName\bin") `
    "$rustDescription toolchain"
$rustCompiler = Resolve-RequiredPath (Join-Path $rustToolchainBin 'rustc.exe') "$rustDescription compiler"
$rustCargo = Resolve-RequiredPath (Join-Path $rustToolchainBin 'cargo.exe') "$rustDescription Cargo"

$vcVarsVersion = if ($UE4SSVariant -eq 'StableRoot') { '14.38' } else { '14.44' }
$devEnvironment = & $env:ComSpec /d /s /c "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 -vcvars_ver=$vcVarsVersion >nul && set"
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio 2022 environment setup failed: $LASTEXITCODE"
}
$seenEnvironmentNames = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
foreach ($line in $devEnvironment) {
    if ($line -match '^([^=][^=]*)=(.*)$') {
        $name = $matches[1]
        if ($seenEnvironmentNames.Add($name)) {
            Set-Item -Path "Env:$name" -Value $matches[2]
        }
    }
}

$env:RUSTUP_HOME = $resolvedRustupHome
$env:CARGO_HOME = $resolvedCargoHome
$gitUnixTools = 'C:\Program Files\Git\usr\bin'
if (-not (Test-Path -LiteralPath (Join-Path $gitUnixTools 'sed.exe') -PathType Leaf) -or
    -not (Test-Path -LiteralPath (Join-Path $gitUnixTools 'basename.exe') -PathType Leaf)) {
    throw "Git for Windows Unix tools were not found: $gitUnixTools"
}
# FetchContent invokes Git shell helpers while configuring UE4SS dependencies.
# Keep the matching Unix tool directory ahead of system Git entries so
# git-submodule can always resolve basename, sed, and git-sh-setup.
$env:PATH = "$gitUnixTools;$rustToolchainBin;$(Join-Path $resolvedCargoHome 'bin');$(Split-Path -Parent $ninja);$env:PATH"
$rustVersion = (& $rustCompiler --version).Trim()
$expectedRustPattern = if ($UE4SSVariant -eq 'StableRoot') { '^rustc 1\.73\.0 ' } else { '^rustc 1\.97\.1 ' }
if ($LASTEXITCODE -ne 0 -or $rustVersion -notmatch $expectedRustPattern) {
    throw "$rustDescription is required for $UE4SSVariant; found $rustVersion"
}
$compilerCommand = Get-Command cl.exe -ErrorAction Stop
$compilerVersion = (Get-Item -LiteralPath $compilerCommand.Source).VersionInfo.FileVersion
$expectedCompilerPattern = if ($UE4SSVariant -eq 'StableRoot') { '^19\.38\.' } else { '^19\.44\.' }
if ($compilerVersion -notmatch $expectedCompilerPattern) {
    throw "MSVC selected through -vcvars_ver=$vcVarsVersion is not the required compiler for $UE4SSVariant; found $compilerVersion"
}
$msvcLinker = Resolve-RequiredPath `
    (Join-Path (Split-Path -Parent $compilerCommand.Source) 'link.exe') `
    'MSVC linker selected beside cl.exe'
# Git's Unix helpers must precede other Git entries for FetchContent, but that
# directory also contains an unrelated Unix link.exe.  A populated Cargo cache
# can hide the collision; pin fresh Rust builds to the linker from the exact
# MSVC toolchain selected above.
$env:CARGO_TARGET_X86_64_PC_WINDOWS_MSVC_LINKER = $msvcLinker

# Upstream fetch declarations use both SSH spellings. Keep translation local to
# this PowerShell process; no global or repository Git configuration is changed.
$env:GIT_CONFIG_COUNT = '2'
$env:GIT_CONFIG_KEY_0 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_0 = 'git@github.com:'
$env:GIT_CONFIG_KEY_1 = 'url.https://github.com/.insteadOf'
$env:GIT_CONFIG_VALUE_1 = 'ssh://git@github.com/'

if (-not $BuildDirectory) {
    $buildLeaf = if ($UE4SSVariant -eq 'StableRoot') { 'StableRoot' } else { 'ExperimentalNested' }
    $BuildDirectory = Join-Path $projectRoot (Join-Path 'out\native' $buildLeaf)
}
$buildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$patternSleuthBindLock = Join-Path $resolvedUE4SS 'deps\first\patternsleuth_bind\Cargo.lock'
$patternSleuthBindLockBytes = [System.IO.File]::ReadAllBytes($patternSleuthBindLock)
$offlineVerifier = $null
$offlineVerification = $null
$offlineManifestSha256 = $null
try {
    $configureArguments = @(
        '--fresh', '-S', $projectRoot, '-B', $buildDirectory, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$Configuration",
        "-DCMAKE_MAKE_PROGRAM=$ninja",
        '-DDSNAP_BUILD_TESTS=ON',
        '-DDSNAP_BUILD_UE4SS=ON',
        "-DDSNAP_UE4SS_VARIANT=$UE4SSVariant",
        "-DUE4SS_ROOT=$resolvedUE4SS",
        "-DRust_COMPILER=$rustCompiler",
        "-DRust_CARGO=$rustCargo",
        '-DRust_RESOLVE_RUSTUP_TOOLCHAINS=OFF',
        "-DFETCHCONTENT_SOURCE_DIR_IMGUITEXTEDIT=$resolvedImGui"
    )
    if ($OfflineDependencies) {
        $offlineVerifier = Resolve-RequiredPath `
            (Join-Path $PSScriptRoot 'Verify-OfflineFetchContent.ps1') `
            'Offline FetchContent verifier'
        $offlineVerification = & $offlineVerifier -PassThru
        if ($null -eq $offlineVerification -or $offlineVerification.Status -cne 'PASSED') {
            throw 'Pinned offline FetchContent verification did not return PASSED.'
        }
        $offlineManifestSha256 =
            (Get-FileHash -LiteralPath $offlineVerification.ManifestPath -Algorithm SHA256).Hash
        foreach ($dependency in $offlineVerification.Sources) {
            $configureArguments +=
                "-DFETCHCONTENT_SOURCE_DIR_$($dependency.FetchContentKey)=$($dependency.AbsolutePath)"
        }
        $configureArguments += '-DFETCHCONTENT_FULLY_DISCONNECTED=ON'
    }
    & $cmake @configureArguments
    if ($LASTEXITCODE -ne 0) { throw "Native configure failed: $LASTEXITCODE" }

    if ($OfflineDependencies) {
        $cmakeCachePath = Resolve-RequiredPath `
            (Join-Path $buildDirectory 'CMakeCache.txt') `
            'Native CMake cache'
        $cmakeCache = @{}
        foreach ($line in Get-Content -LiteralPath $cmakeCachePath) {
            if ($line -match '^([^#/:][^:]*)\:[^=]*=(.*)$') {
                $cmakeCache[$matches[1]] = $matches[2]
            }
        }
        if (-not $cmakeCache.ContainsKey('FETCHCONTENT_FULLY_DISCONNECTED') -or
            $cmakeCache['FETCHCONTENT_FULLY_DISCONNECTED'] -cne 'ON') {
            throw 'CMake did not preserve FETCHCONTENT_FULLY_DISCONNECTED=ON.'
        }
        $expectedCMakeSources = [ordered]@{}
        foreach ($dependency in $offlineVerification.Sources) {
            $expectedCMakeSources["FETCHCONTENT_SOURCE_DIR_$($dependency.FetchContentKey)"] =
                $dependency.AbsolutePath
        }
        $expectedCMakeSources['FETCHCONTENT_SOURCE_DIR_IMGUITEXTEDIT'] = $resolvedImGui
        $actualCMakeSourceKeys = @(
            $cmakeCache.Keys |
                Where-Object { ([string]$_).StartsWith(
                        'FETCHCONTENT_SOURCE_DIR_',
                        [StringComparison]::Ordinal) })
        if ($actualCMakeSourceKeys.Count -ne $expectedCMakeSources.Count -or
            @($actualCMakeSourceKeys |
                Where-Object { -not $expectedCMakeSources.Contains([string]$_) }).Count -ne 0) {
            throw 'Unexpected FetchContent source override set in CMakeCache.txt; the exact eleven-source mapping is required.'
        }
        foreach ($entry in $expectedCMakeSources.GetEnumerator()) {
            if (-not $cmakeCache.ContainsKey($entry.Key)) {
                throw "CMake did not preserve the pinned source mapping $($entry.Key)."
            }
            $actualSource = [IO.Path]::GetFullPath(
                ([string]$cmakeCache[$entry.Key]).Replace('/', '\'))
            $expectedSource = [IO.Path]::GetFullPath([string]$entry.Value)
            if (-not $actualSource.Equals($expectedSource, [StringComparison]::OrdinalIgnoreCase)) {
                throw "CMake source mapping mismatch for $($entry.Key): expected $expectedSource; found $actualSource"
            }
        }
    }

    & $cmake --build $buildDirectory --target DragonSwordNativeAutoPickup
    if ($LASTEXITCODE -ne 0) { throw "Native build failed: $LASTEXITCODE" }
}
finally {
    # Cargo 1.97 may rewrite this pinned upstream lockfile while resolving the
    # selected crate. Restore the exact pre-build bytes even when the build
    # fails, then let the strict repository checks reject any other mutation.
    [System.IO.File]::WriteAllBytes($patternSleuthBindLock, $patternSleuthBindLockBytes)
    if ($OfflineDependencies -and $null -ne $offlineVerifier) {
        $postBuildVerification = & $offlineVerifier -PassThru
        $postBuildManifestSha256 =
            (Get-FileHash -LiteralPath $postBuildVerification.ManifestPath -Algorithm SHA256).Hash
        if ($postBuildVerification.Status -cne 'PASSED' -or
            $postBuildManifestSha256 -cne $offlineManifestSha256) {
            throw 'Pinned offline FetchContent inputs changed during the native build.'
        }
    }
}

Assert-GitCommit $resolvedUE4SS $expectedCommits.UE4SS 'RE-UE4SS after build'
Assert-GitCommit $unrealRoot $expectedCommits.UEPseudo 'UEPseudo after build'
Assert-GitCommit $patternSleuthRoot $expectedCommits.PatternSleuth 'patternsleuth after build'
Assert-GitCommit $resolvedImGui $expectedCommits.ImGuiColorTextEdit 'ImGuiColorTextEdit after build'

$dll = Get-ChildItem -LiteralPath $buildDirectory -Recurse -Filter main.dll -File |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $dll) { throw 'Native build completed without producing main.dll.' }
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $dll.FullName).Hash
Write-Host "Native adapter build passed: $($dll.FullName)"
Write-Host "UE4SS variant: $UE4SSVariant"
Write-Host "SHA-256: $hash"
Write-Host 'This build is not deployment or in-game acceptance.'
