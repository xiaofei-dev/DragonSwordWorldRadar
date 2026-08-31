[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$UE4SSRoot,
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
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectoryCandidate = if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    Join-Path $projectRoot 'dist\work\build\native'
} elseif ([System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory
} else {
    Join-Path $projectRoot $BuildDirectory
}
$resolvedBuildDirectory = [System.IO.Path]::GetFullPath(
    $buildDirectoryCandidate).TrimEnd('\')
$projectRootFull = [System.IO.Path]::GetFullPath($projectRoot).TrimEnd('\')
$projectWorkRootFull = [System.IO.Path]::GetFullPath(
    (Join-Path $projectRoot 'dist\work')).TrimEnd('\')
if ([string]::Equals(
        $resolvedBuildDirectory, $projectRootFull,
        [System.StringComparison]::OrdinalIgnoreCase) -or
    $projectRootFull.StartsWith(
        $resolvedBuildDirectory + '\',
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "BuildDirectory must not be the project root or one of its ancestors: $resolvedBuildDirectory"
}
if ($resolvedBuildDirectory.StartsWith(
        $projectRootFull + '\',
        [System.StringComparison]::OrdinalIgnoreCase) -and
    -not $resolvedBuildDirectory.StartsWith(
        $projectWorkRootFull + '\',
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "An in-project BuildDirectory must be inside dist\work: $resolvedBuildDirectory"
}
if (Test-Path -LiteralPath $resolvedBuildDirectory) {
    $buildDirectoryItem = Get-Item -LiteralPath $resolvedBuildDirectory -Force
    if (-not $buildDirectoryItem.PSIsContainer -or
        ($buildDirectoryItem.Attributes -band
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "BuildDirectory must be a plain directory: $resolvedBuildDirectory"
    }
}
$sdkRootCandidate = if ($SdkRoot) {
    $SdkRoot
} else {
    Join-Path $projectRoot '.sdk'
}

$expectedCommits = @{
    UE4SS = '1c1a1497f942c707f47ba668db75b25e86f6c08a'
    UEPseudo = 'b2e876da82b17254c04304746341c8fde0ddb37c'
    PatternSleuth = 'da8bfe4c5a464be0ef225c2c9a6ccaa2d9284018'
    ImGuiColorTextEdit = '6d943aba9f7cef05da80b86dbb0253b63818f95c'
    IconFontCppHeaders = '210b5a399a64270674560d633638952d1e8d804d'
}
$expectedFetchCommits = [ordered]@{
    'concurrentqueue-src' = 'c68072129c8a5b4025122ca5a0c82ab14b30cb03'
    'corrosion-src' = '52844733e14f095c947577627e367ee5f6458af7'
    'fmt-src' = '40626af88bd7df9a5fb80be7b25ac85b122d6c21'
    'glaze-src' = '3a850807501d98d23bab4bdc5af64d8d4e83e6bc'
    'glfw-src' = 'e2c92645460f680fd272fd2eed591efb2be7dc31'
    'imgui-src' = '5d4126876bc10396d4c6511853ff10964414c776'
    'polyhook2-src' = '298d56210b9d9e66cde8f96481d6053925c6ae15'
    'raw_pdb-src' = '8c6a7146393c83d27fa101e8bc8017f2a7f151df'
    'zydis-src' = 'a2278f1d254e492f6a6b39f6cb5d1f5d515659dc'
}
$expectedFmtPatchedRangesHash =
    '7146ED70122CCD548AF1BE97794ED069AC1A218B99ADFF4C83D84DFCF4B5276A'
$expectedPatternSleuthCargoLockBlobHash =
    '19292C3E0A74C851EB11AD09A3B3AC5E5D8E9B80EEBE34DD705DF10E09DC7E50'
$expectedPatternSleuthCargoLockWorktreeHash =
    '1E2A435879DAE44EF2CBB1E15F1AE4E6B487E6FF23A8C500D799E6EB23742156'
$knownCargoRewriteHash =
    '88C3718C03492CDC2650217A9D8BB2A8DBDECDBDE1B4EA79E3E529E838B49BBE'
$expectedFetchOrigins = @{
    'concurrentqueue-src' = 'https://github.com/cameron314/concurrentqueue.git'
    'corrosion-src' = 'https://github.com/UE4SS-RE/corrosion.git'
    'fmt-src' = 'https://github.com/fmtlib/fmt.git'
    'glaze-src' = 'https://github.com/stephenberry/glaze.git'
    'glfw-src' = 'https://github.com/glfw/glfw.git'
    'imgui-src' = 'https://github.com/ocornut/imgui.git'
    'polyhook2-src' = 'https://github.com/stevemk14ebr/PolyHook_2_0.git'
    'raw_pdb-src' = 'https://github.com/MolecularMatters/raw_pdb.git'
    'zydis-src' = 'https://github.com/zyantific/zydis.git'
}
$expectedSourceOrigins = @{
    UE4SS = 'https://github.com/UE4SS-RE/RE-UE4SS.git'
    UEPseudo = 'https://github.com/Re-UE4SS/UEPseudo.git'
    PatternSleuth = 'https://github.com/trumank/patternsleuth.git'
    ImGuiColorTextEdit = 'https://github.com/UE4SS-RE/ImGuiColorTextEdit.git'
    IconFontCppHeaders = 'https://github.com/juliettef/IconFontCppHeaders.git'
}
$buildLockSourceNames = @{
    UE4SS = 'RE-UE4SS'
    UEPseudo = 'UEPseudo'
    PatternSleuth = 'patternsleuth'
    ImGuiColorTextEdit = 'ImGuiColorTextEdit'
    IconFontCppHeaders = 'IconFontCppHeaders'
}
$expectedNestedCommits = [ordered]@{
    'polyhook2-src/zydis' = 'a2278f1d254e492f6a6b39f6cb5d1f5d515659dc'
    'polyhook2-src/zydis/dependencies/zycore' = '0b2432ced0884fd152b471d97ecf0258ff4d859f'
    'polyhook2-src/asmjit' = 'a3199e8857792cd10b7589ff5d58343d2c9008ea'
    'polyhook2-src/asmtk' = '3bce8a48aa895e6d639501d1f1105ab5fe007753'
    'zydis-src/dependencies/zycore' = '0b2432ced0884fd152b471d97ecf0258ff4d859f'
}
$expectedNestedOrigins = @{
    'polyhook2-src/zydis' = 'https://github.com/zyantific/zydis.git'
    'polyhook2-src/zydis/dependencies/zycore' = 'https://github.com/zyantific/zycore-c'
    'polyhook2-src/asmjit' = 'https://github.com/asmjit/asmjit.git'
    'polyhook2-src/asmtk' = 'https://github.com/asmjit/asmtk.git'
    'zydis-src/dependencies/zycore' = 'https://github.com/zyantific/zycore-c'
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
    # The build can run either in the desktop sandbox account or the user's
    # normal account. Trust only this exact pinned checkout for this command;
    # never mutate global or repository Git configuration.
    $safeDirectory = "safe.directory=$Repository"
    $actual = (& git -c $safeDirectory -C $Repository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "$Description must be pinned to $Expected; found $actual"
    }
    $status = @(Get-PinnedGitOutput -Repository $Repository `
            -GitArguments @(
                'status', '--short', '--untracked-files=all',
                '--ignore-submodules=all'))
    if ($status.Count -ne 0) {
        throw "$Description checkout is not clean: $($status -join ', ')"
    }
}

function Get-ByteArraySha256 {
    param([byte[]]$Bytes)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString(
                $sha.ComputeHash($Bytes))).Replace('-', '')
    } finally {
        $sha.Dispose()
    }
}

function Get-PinnedGitBlobBytes {
    param(
        [string]$Repository,
        [string]$RevisionPath
    )
    $git = (Get-Command git -ErrorAction Stop).Source
    $safe = $Repository.Replace('\', '/')
    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = $git
    $processInfo.Arguments =
        "-c `"safe.directory=$safe`" -C `"$Repository`" cat-file blob `"$RevisionPath`""
    $processInfo.UseShellExecute = $false
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::Start($processInfo)
    $memory = New-Object System.IO.MemoryStream
    try {
        $process.StandardOutput.BaseStream.CopyTo($memory)
        $errorText = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw "Pinned Git blob read failed: $errorText"
        }
        $bytes = $memory.ToArray()
        return ,$bytes
    } finally {
        $memory.Dispose()
        $process.Dispose()
    }
}

function Restore-PinnedCargoLockIfKnownMutation {
    param(
        [string]$Repository,
        [string]$Path
    )
    $lockItem = Get-Item -LiteralPath $Path -Force
    if ($lockItem.PSIsContainer `
        -or ($lockItem.Attributes -band `
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Pinned upstream Cargo.lock is not a regular file: $Path"
    }
    $committed = Get-PinnedGitBlobBytes -Repository $Repository `
        -RevisionPath 'HEAD:deps/first/patternsleuth_bind/Cargo.lock'
    if ((Get-ByteArraySha256 $committed) -ne `
        $expectedPatternSleuthCargoLockBlobHash) {
        throw 'Pinned upstream Cargo.lock blob does not match the build contract.'
    }
    $committedText = [System.Text.UTF8Encoding]::new(
        $false, $true).GetString($committed)
    if ($committedText.Contains("`r")) {
        throw 'Pinned upstream Cargo.lock blob has unexpected line endings.'
    }
    $autoCrlf = ((Get-PinnedGitOutput -Repository $Repository `
                -GitArguments @('config', '--bool', 'core.autocrlf')) `
            -join '').Trim()
    # Match the pinned checkout's own line-ending policy so the exact content
    # and Git's strict clean-worktree gate agree on every supported builder.
    $canonicalWorktree = if ($autoCrlf -eq 'true') {
        [System.Text.UTF8Encoding]::new($false).GetBytes(
            $committedText.Replace("`n", "`r`n"))
    } else {
        $committed
    }
    $canonicalHash = Get-ByteArraySha256 $canonicalWorktree
    if ($canonicalHash -notin @(
            $expectedPatternSleuthCargoLockBlobHash,
            $expectedPatternSleuthCargoLockWorktreeHash)) {
        throw 'Pinned upstream Cargo.lock worktree bytes do not match the build contract.'
    }
    $current = [System.IO.File]::ReadAllBytes($Path)
    $currentHash = Get-ByteArraySha256 $current
    if ($currentHash -eq $canonicalHash) {
        return ,$canonicalWorktree
    }
    if ($currentHash -ne $knownCargoRewriteHash `
        -and $currentHash -ne $expectedPatternSleuthCargoLockBlobHash) {
        throw "Refusing to overwrite an unknown upstream Cargo.lock change: $currentHash"
    }
    [System.IO.File]::WriteAllBytes($Path, $canonicalWorktree)
    Write-Host 'Restored the exact pinned Cargo.lock after a known prior Cargo rewrite.'
    return ,$canonicalWorktree
}

function Get-PinnedGitOutput {
    param(
        [string]$Repository,
        [string[]]$GitArguments
    )
    $safeDirectory = "safe.directory=$($Repository.Replace('\', '/'))"
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & git -c $safeDirectory -C $Repository @GitArguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    if ($exitCode -ne 0) {
        throw "Pinned dependency Git inspection failed: $Repository ($($output -join [Environment]::NewLine))"
    }
    return @($output | ForEach-Object { $_.ToString() })
}

function ConvertTo-CanonicalGitOrigin {
    param([string]$Origin)
    return ($Origin.Trim() -replace '^git@github\.com:', `
        'https://github.com/' -replace '^ssh://git@github\.com/', `
        'https://github.com/' -replace '\.git$', '').ToLowerInvariant()
}

function Assert-GitOrigin {
    param(
        [string]$Repository,
        [string]$Expected,
        [string]$Description
    )
    $actual = ((Get-PinnedGitOutput -Repository $Repository `
                -GitArguments @('remote', 'get-url', 'origin')) -join '').Trim()
    if ((ConvertTo-CanonicalGitOrigin $actual) -ne `
        (ConvertTo-CanonicalGitOrigin $Expected)) {
        throw "$Description origin must be $Expected; found $actual"
    }
}

function Assert-Gitlink {
    param(
        [string]$ParentRepository,
        [string]$RelativePath,
        [string]$Expected,
        [string]$Description
    )
    $entry = ((Get-PinnedGitOutput -Repository $ParentRepository `
                -GitArguments @('ls-files', '--stage', '--', $RelativePath)) `
            -join '').Trim()
    if ($entry -notmatch '^160000\s+([0-9a-fA-F]{40})\s+0\s+') {
        throw "$Description is not a pinned Git submodule entry."
    }
    if ($Matches[1].ToLowerInvariant() -ne $Expected.ToLowerInvariant()) {
        throw "$Description parent gitlink must be $Expected; found $($Matches[1])"
    }
}

function Assert-NestedFetchGitlinks {
    param([string]$BuildDirectory)

    $dependencyRoot = Join-Path $BuildDirectory '_deps'
    foreach ($relative in $expectedNestedCommits.Keys) {
        $repository = Join-Path $dependencyRoot ($relative.Replace('/', '\'))
        Assert-GitCommit $repository $expectedNestedCommits[$relative] `
            "Nested dependency $relative"
        Assert-GitOrigin $repository $expectedNestedOrigins[$relative] `
            "Nested dependency $relative"
    }
    $polyhook = Join-Path $dependencyRoot 'polyhook2-src'
    $polyhookZydis = Join-Path $polyhook 'zydis'
    $standaloneZydis = Join-Path $dependencyRoot 'zydis-src'
    Assert-Gitlink $polyhook 'zydis' `
        $expectedNestedCommits['polyhook2-src/zydis'] `
        'polyhook2-src/zydis'
    Assert-Gitlink $polyhook 'asmjit' `
        $expectedNestedCommits['polyhook2-src/asmjit'] `
        'polyhook2-src/asmjit'
    Assert-Gitlink $polyhook 'asmtk' `
        $expectedNestedCommits['polyhook2-src/asmtk'] `
        'polyhook2-src/asmtk'
    Assert-Gitlink $polyhookZydis 'dependencies/zycore' `
        $expectedNestedCommits['polyhook2-src/zydis/dependencies/zycore'] `
        'polyhook2-src/zydis/dependencies/zycore'
    Assert-Gitlink $standaloneZydis 'dependencies/zycore' `
        $expectedNestedCommits['zydis-src/dependencies/zycore'] `
        'zydis-src/dependencies/zycore'
}

function Assert-BuildLock {
    $lockPath = Join-Path $projectRoot 'metadata\native-build-lock.json'
    $lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
    if ([int]$lock.schema_version -ne 1 `
        -or [string]$lock.configuration -ne $Configuration `
        -or [string]$lock.toolchain.cmake -ne '3.29.6' `
        -or [string]$lock.toolchain.msvc -ne '19.44' `
        -or [string]$lock.toolchain.rust -ne '1.97.1') {
        throw 'Native build lock identity or configuration is invalid.'
    }
    $sourceByName = @{}
    foreach ($entry in @($lock.source_dependencies)) {
        if ($sourceByName.ContainsKey([string]$entry.name)) {
            throw "Native build lock repeats source dependency $($entry.name)."
        }
        $sourceByName[[string]$entry.name] = $entry
    }
    if ($sourceByName.Count -ne $expectedCommits.Count) {
        throw 'Native build lock source-dependency set is not exact.'
    }
    foreach ($name in $expectedCommits.Keys) {
        $lockName = $buildLockSourceNames[$name]
        if (-not $sourceByName.ContainsKey($lockName) `
            -or [string]$sourceByName[$lockName].commit -ne $expectedCommits[$name] `
            -or (ConvertTo-CanonicalGitOrigin `
                ([string]$sourceByName[$lockName].origin)) -ne `
                (ConvertTo-CanonicalGitOrigin $expectedSourceOrigins[$name])) {
            throw "Native build lock is stale for source dependency $name."
        }
    }
    $fetchByName = @{}
    foreach ($entry in @($lock.fetchcontent_dependencies)) {
        if ($fetchByName.ContainsKey([string]$entry.name)) {
            throw "Native build lock repeats FetchContent dependency $($entry.name)."
        }
        $fetchByName[[string]$entry.name] = $entry
    }
    if ($fetchByName.Count -ne $expectedFetchCommits.Count) {
        throw 'Native build lock FetchContent dependency set is not exact.'
    }
    foreach ($name in $expectedFetchCommits.Keys) {
        if (-not $fetchByName.ContainsKey($name) `
            -or [string]$fetchByName[$name].commit -ne `
                $expectedFetchCommits[$name] `
            -or (ConvertTo-CanonicalGitOrigin `
                ([string]$fetchByName[$name].origin)) -ne `
                (ConvertTo-CanonicalGitOrigin $expectedFetchOrigins[$name])) {
            throw "Native build lock is stale for FetchContent dependency $name."
        }
    }
    if ([string]$fetchByName['fmt-src'].allowed_patch_sha256 -ne `
        $expectedFmtPatchedRangesHash) {
        throw 'Native build lock does not bind the allowed fmt patch.'
    }
    $nestedByPath = @{}
    foreach ($entry in @($lock.nested_gitlinks)) {
        if ($nestedByPath.ContainsKey([string]$entry.path)) {
            throw "Native build lock repeats nested dependency $($entry.path)."
        }
        $nestedByPath[[string]$entry.path] = $entry
    }
    if ($nestedByPath.Count -ne $expectedNestedCommits.Count) {
        throw 'Native build lock nested-dependency set is not exact.'
    }
    foreach ($relative in $expectedNestedCommits.Keys) {
        if (-not $nestedByPath.ContainsKey($relative) `
            -or [string]$nestedByPath[$relative].commit -ne `
                $expectedNestedCommits[$relative] `
            -or (ConvertTo-CanonicalGitOrigin `
                ([string]$nestedByPath[$relative].origin)) -ne `
                (ConvertTo-CanonicalGitOrigin $expectedNestedOrigins[$relative])) {
            throw "Native build lock is stale for nested dependency $relative."
        }
    }
}

function Assert-FetchContentCache {
    param([string]$BuildDirectory)

    $dependencyRoot = Join-Path $BuildDirectory '_deps'
    $allSourceDirectories = @(if (Test-Path -LiteralPath $dependencyRoot) {
            Get-ChildItem -LiteralPath $dependencyRoot -Directory -Filter '*-src'
        })
    $unexpected = @($allSourceDirectories | Where-Object {
            -not $expectedFetchCommits.Contains($_.Name) -and
            $_.Name -ne 'iconfontcppheaders-src'
        })
    if ($unexpected.Count -ne 0) {
        throw "FetchContent cache contains unexpected source trees: $($unexpected.Name -join ', ')"
    }
    $present = @($allSourceDirectories | Where-Object {
            $expectedFetchCommits.Contains($_.Name)
        })
    if ($present.Count -eq 0) {
        return $false
    }
    if ($present.Count -ne $expectedFetchCommits.Count) {
        throw 'FetchContent cache is partial. Remove dist/work/build/native or provide the complete pinned cache.'
    }

    foreach ($name in $expectedFetchCommits.Keys) {
        $repository = Join-Path $dependencyRoot $name
        if (-not (Test-Path -LiteralPath (Join-Path $repository '.git'))) {
            throw "FetchContent cache entry is not a Git checkout: $name"
        }
        $head = ((Get-PinnedGitOutput -Repository $repository `
                    -GitArguments @('rev-parse', 'HEAD')) -join '').Trim()
        if ($head -ne $expectedFetchCommits[$name]) {
            throw "FetchContent cache $name must be pinned to $($expectedFetchCommits[$name]); found $head"
        }
        Assert-GitOrigin -Repository $repository `
            -Expected $expectedFetchOrigins[$name] `
            -Description "FetchContent cache $name"
        $status = @(Get-PinnedGitOutput -Repository $repository `
                -GitArguments @(
                    'status', '--short', '--untracked-files=all',
                    '--ignore-submodules=all'))
        if ($name -eq 'fmt-src') {
            if (($status -join "`n") -ne ' M include/fmt/ranges.h') {
                throw 'The fmt cache must contain only the pinned UE4SS check-macro patch.'
            }
            $ranges = Join-Path $repository 'include\fmt\ranges.h'
            $rangesHash = (Get-FileHash -LiteralPath $ranges `
                    -Algorithm SHA256).Hash
            if ($rangesHash -ne $expectedFmtPatchedRangesHash) {
                throw 'The fmt check-macro patch content does not match the pinned UE4SS result.'
            }
        } elseif ($status.Count -ne 0) {
            throw "FetchContent cache $name is not clean: $($status -join ', ')"
        }
    }
    Assert-NestedFetchGitlinks -BuildDirectory $BuildDirectory
    return $true
}

$resolvedSdkRoot = Resolve-RequiredPath $sdkRootCandidate 'Pinned native build SDK root'
$resolvedUE4SS = Resolve-RequiredPath $UE4SSRoot 'RE-UE4SS source tree'
if (-not $ImGuiColorTextEditRoot) {
    $ImGuiColorTextEditRoot = Join-Path $resolvedSdkRoot 'ImGuiColorTextEdit'
}
$resolvedImGui = Resolve-RequiredPath $ImGuiColorTextEditRoot 'ImGuiColorTextEdit source tree'
if (-not $IconFontCppHeadersRoot) {
    $iconFontCandidates = @(
        (Join-Path $resolvedSdkRoot 'IconFontCppHeaders'),
        (Join-Path $resolvedBuildDirectory '_deps\iconfontcppheaders-src')
    )
    $IconFontCppHeadersRoot = $iconFontCandidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Container } |
        Select-Object -First 1
}
$resolvedIconFont = Resolve-RequiredPath $IconFontCppHeadersRoot `
    'Pinned IconFontCppHeaders source tree'

$unrealRoot = Resolve-RequiredPath (Join-Path $resolvedUE4SS 'deps\first\Unreal') 'UEPseudo submodule'
$patternSleuthRoot = Resolve-RequiredPath (Join-Path $resolvedUE4SS 'deps\first\patternsleuth') 'patternsleuth submodule'
$patternSleuthBindLock = Join-Path $resolvedUE4SS `
    'deps\first\patternsleuth_bind\Cargo.lock'
$preRestoreHead = ((Get-PinnedGitOutput -Repository $resolvedUE4SS `
            -GitArguments @('rev-parse', 'HEAD')) -join '').Trim()
if ($preRestoreHead -ne $expectedCommits.UE4SS) {
    throw "RE-UE4SS must be pinned before Cargo.lock repair; found $preRestoreHead"
}
Assert-GitOrigin $resolvedUE4SS $expectedSourceOrigins.UE4SS `
    'RE-UE4SS before Cargo.lock repair'
$patternSleuthBindLockBytes = Restore-PinnedCargoLockIfKnownMutation `
    -Repository $resolvedUE4SS -Path $patternSleuthBindLock
Assert-GitCommit $resolvedUE4SS $expectedCommits.UE4SS 'RE-UE4SS'
Assert-GitCommit $unrealRoot $expectedCommits.UEPseudo 'UEPseudo'
Assert-GitCommit $patternSleuthRoot $expectedCommits.PatternSleuth 'patternsleuth'
Assert-GitCommit $resolvedImGui $expectedCommits.ImGuiColorTextEdit 'ImGuiColorTextEdit'
Assert-GitCommit $resolvedIconFont $expectedCommits.IconFontCppHeaders `
    'IconFontCppHeaders'
Assert-GitOrigin $resolvedUE4SS $expectedSourceOrigins.UE4SS 'RE-UE4SS'
Assert-GitOrigin $unrealRoot $expectedSourceOrigins.UEPseudo 'UEPseudo'
Assert-GitOrigin $patternSleuthRoot $expectedSourceOrigins.PatternSleuth `
    'patternsleuth'
Assert-GitOrigin $resolvedImGui $expectedSourceOrigins.ImGuiColorTextEdit `
    'ImGuiColorTextEdit'
Assert-GitOrigin $resolvedIconFont $expectedSourceOrigins.IconFontCppHeaders `
    'IconFontCppHeaders'
Assert-BuildLock

if (-not $CMakePath) {
    $CMakePath = Join-Path $resolvedSdkRoot 'cmake-3.29.6\cmake-3.29.6-windows-x86_64\bin\cmake.exe'
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
$ninjaVersion = (& $ninja --version).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($ninjaVersion)) {
    throw 'Ninja version inspection failed.'
}

if (-not $VsDevCmdPath) {
    $VsDevCmdPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
}
$vsDevCmd = Resolve-RequiredPath $VsDevCmdPath 'Visual Studio 2022 developer command script'

if (-not $RustupHome) { $RustupHome = Join-Path $resolvedSdkRoot 'rustup' }
if (-not $CargoHome) { $CargoHome = Join-Path $resolvedSdkRoot 'cargo' }
$resolvedRustupHome = Resolve-RequiredPath $RustupHome 'Project-local rustup home'
$resolvedCargoHome = Resolve-RequiredPath $CargoHome 'Project-local Cargo home'
$rustToolchainBin = Resolve-RequiredPath (Join-Path $resolvedRustupHome 'toolchains\stable-x86_64-pc-windows-msvc\bin') 'Rust 1.97.1 toolchain'
$rustCompiler = Resolve-RequiredPath (Join-Path $rustToolchainBin 'rustc.exe') 'Rust 1.97.1 compiler'
$rustCargo = Resolve-RequiredPath (Join-Path $rustToolchainBin 'cargo.exe') 'Cargo 1.97.1'

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

$previousErrorActionPreference = $ErrorActionPreference
function Invoke-NativeChecked {
    param(
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string[]]$ArgumentList
    )

    $nativeOutput = & $Executable @ArgumentList 2>&1
    $nativeExitCode = $LASTEXITCODE
    $nativeLines = @($nativeOutput | ForEach-Object { $_.ToString() })
    if ($nativeExitCode -ne 0) {
        $diagnosticLines = @($nativeLines | Where-Object {
            $_ -match '(?i)(^FAILED:|\berror\s+(?:C|LNK)\d+|fatal error|:\s*error:|ninja:\s*error|command failed)'
        } | Select-Object -Last 80)
        $tail = ($nativeLines | Select-Object -Last 120) -join [Environment]::NewLine
        $diagnostics = if ($diagnosticLines.Count -gt 0) {
            ($diagnosticLines -join [Environment]::NewLine) +
                [Environment]::NewLine + '--- output tail ---' +
                [Environment]::NewLine
        } else {
            ''
        }
        throw "$Label failed with exit code $nativeExitCode.`n$diagnostics$tail"
    }
    $nativeLines | Select-Object -Last 20 | ForEach-Object { Write-Host $_ }
}

try {
    # CMake and the pinned upstream projects emit non-fatal status text on
    # stderr. PowerShell 5 promotes that text to NativeCommandError when the
    # script-wide preference is Stop, so judge native tools by their exit code.
    $ErrorActionPreference = 'Continue'
    $configureArguments = @(
        '--fresh', '-S', $projectRoot, '-B', $resolvedBuildDirectory, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$Configuration",
        "-DCMAKE_MAKE_PROGRAM=$ninja",
        '-DDSNWRPR_BUILD_UE4SS=ON',
        "-DUE4SS_ROOT=$resolvedUE4SS",
        "-DRust_COMPILER=$rustCompiler",
        "-DRust_CARGO=$rustCargo",
        '-DRust_RESOLVE_RUSTUP_TOOLCHAINS=OFF',
        "-DFETCHCONTENT_SOURCE_DIR_IMGUITEXTEDIT=$resolvedImGui",
        "-DFETCHCONTENT_SOURCE_DIR_ICONFONTCPPHEADERS=$resolvedIconFont"
    )
    $hasCompleteFetchCache = Assert-FetchContentCache $resolvedBuildDirectory
    if ($hasCompleteFetchCache) {
        # A release rebuild must not contact or update upstream repositories
        # when the complete prior FetchContent source cache is available.
        $configureArguments += '-DFETCHCONTENT_FULLY_DISCONNECTED=ON'
    }
    Invoke-NativeChecked -Label 'Native configure' -Executable $cmake -ArgumentList $configureArguments

    # --fresh only regenerates the build graph. --clean-first additionally
    # removes every output known to that graph before compiling the release DLL.
    $buildArguments = @(
        '--build', $resolvedBuildDirectory, '--target',
        'DragonSwordNativeWorldRadarPostRender', '--clean-first')
    Invoke-NativeChecked -Label 'Native build' -Executable $cmake -ArgumentList $buildArguments
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
    # Cargo 1.97 may rewrite this pinned upstream lockfile while resolving the
    # selected crate. Restore the exact pre-build bytes even when the build
    # fails, then let the strict repository checks reject any other mutation.
    [System.IO.File]::WriteAllBytes($patternSleuthBindLock, $patternSleuthBindLockBytes)
}

Assert-GitCommit $resolvedUE4SS $expectedCommits.UE4SS 'RE-UE4SS after build'
Assert-GitCommit $unrealRoot $expectedCommits.UEPseudo 'UEPseudo after build'
Assert-GitCommit $patternSleuthRoot $expectedCommits.PatternSleuth 'patternsleuth after build'
Assert-GitCommit $resolvedImGui $expectedCommits.ImGuiColorTextEdit 'ImGuiColorTextEdit after build'
Assert-GitCommit $resolvedIconFont $expectedCommits.IconFontCppHeaders `
    'IconFontCppHeaders after build'
Assert-GitOrigin $resolvedUE4SS $expectedSourceOrigins.UE4SS `
    'RE-UE4SS after build'
Assert-GitOrigin $unrealRoot $expectedSourceOrigins.UEPseudo `
    'UEPseudo after build'
Assert-GitOrigin $patternSleuthRoot $expectedSourceOrigins.PatternSleuth `
    'patternsleuth after build'
Assert-GitOrigin $resolvedImGui $expectedSourceOrigins.ImGuiColorTextEdit `
    'ImGuiColorTextEdit after build'
Assert-GitOrigin $resolvedIconFont $expectedSourceOrigins.IconFontCppHeaders `
    'IconFontCppHeaders after build'
if (-not (Assert-FetchContentCache $resolvedBuildDirectory)) {
    throw 'Native build completed without the complete pinned FetchContent dependency set.'
}

$dllPath = Join-Path $resolvedBuildDirectory 'main.dll'
if (-not (Test-Path -LiteralPath $dllPath -PathType Leaf)) {
    throw "Native build completed without producing main.dll in BuildDirectory: $resolvedBuildDirectory"
}
$dll = Get-Item -LiteralPath $dllPath
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $dll.FullName).Hash
$receiptPath = Join-Path $resolvedBuildDirectory 'native-build-receipt.json'
. (Join-Path $PSScriptRoot 'NativeBuildReceipt.ps1')
New-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot -DllPath $dllPath `
    -ReceiptPath $receiptPath -Configuration $Configuration `
    -CMakeVersion $cmakeVersion -NinjaVersion $ninjaVersion `
    -CompilerVersion $compilerVersion -RustVersion $rustVersion | Out-Null
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot -DllPath $dllPath `
    -ReceiptPath $receiptPath | Out-Null
Write-Host "Native adapter build passed: $($dll.FullName)"
Write-Host "SHA-256: $hash"
Write-Host "Build receipt: $receiptPath"
Write-Host 'This build is not deployment or in-game acceptance.'
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
