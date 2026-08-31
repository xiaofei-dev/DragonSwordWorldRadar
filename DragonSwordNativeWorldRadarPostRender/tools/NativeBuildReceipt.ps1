$ErrorActionPreference = 'Stop'

function Get-DsnwrProjectRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $prefix = $rootFull + '\'
    if (-not $pathFull.StartsWith(
            $prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Build input escaped the project root: $pathFull"
    }
    return $pathFull.Substring($prefix.Length).Replace('\', '/')
}

function Get-DsnwrCompiledSourceDigest {
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    $files = @(
        Get-Item -LiteralPath (Join-Path $ProjectRoot 'CMakeLists.txt')
        Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'include') `
            -Recurse -Force -File
        Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'src\native') `
            -Recurse -Force -File | Where-Object {
                $_.Extension -in @('.cpp', '.hpp', '.h', '.cxx', '.cc')
            }
    ) | Sort-Object FullName
    if ($files.Count -eq 0) {
        throw 'No compiled source inputs were found.'
    }
    $lines = @($files | ForEach-Object {
            $relative = Get-DsnwrProjectRelativePath `
                -Root $ProjectRoot -Path $_.FullName
            $hash = (Get-FileHash -LiteralPath $_.FullName `
                    -Algorithm SHA256).Hash
            "$relative|$($_.Length)|$hash"
        })
    $bytes = [System.Text.UTF8Encoding]::new($false).GetBytes(
        ($lines -join "`n"))
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString(
                $sha.ComputeHash($bytes))).Replace('-', '')
    } finally {
        $sha.Dispose()
    }
}

function Get-DsnwrReleaseToolDigest {
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    $files = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'tools') `
            -Force -File -Filter '*.ps1' | Sort-Object FullName)
    if ($files.Count -eq 0) {
        throw 'No release tool inputs were found.'
    }
    $lines = @($files | ForEach-Object {
            $relative = Get-DsnwrProjectRelativePath `
                -Root $ProjectRoot -Path $_.FullName
            $hash = (Get-FileHash -LiteralPath $_.FullName `
                    -Algorithm SHA256).Hash
            "$relative|$($_.Length)|$hash"
        })
    $bytes = [System.Text.UTF8Encoding]::new($false).GetBytes(
        ($lines -join "`n"))
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([System.BitConverter]::ToString(
                $sha.ComputeHash($bytes))).Replace('-', '')
    } finally {
        $sha.Dispose()
    }
}

function New-DsnwrNativeBuildReceipt {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [Parameter(Mandatory = $true)][string]$ReceiptPath,
        [Parameter(Mandatory = $true)][string]$Configuration,
        [Parameter(Mandatory = $true)][string]$CMakeVersion,
        [Parameter(Mandatory = $true)][string]$NinjaVersion,
        [Parameter(Mandatory = $true)][string]$CompilerVersion,
        [Parameter(Mandatory = $true)][string]$RustVersion,
        [ValidateSet('ExperimentalNested', 'StableRoot')]
        [string]$UE4SSVariant = 'ExperimentalNested',
        [string]$BuildScriptPath,
        [string]$BuildLockPath
    )

    $releasePath = Join-Path $ProjectRoot 'metadata\release.json'
    if ([string]::IsNullOrWhiteSpace($BuildLockPath)) {
        $BuildLockPath = Join-Path $ProjectRoot 'metadata\native-build-lock.json'
    }
    $lockPath = [System.IO.Path]::GetFullPath($BuildLockPath)
    if ([string]::IsNullOrWhiteSpace($BuildScriptPath)) {
        $BuildScriptPath = Join-Path $ProjectRoot 'tools\Build-Native.ps1'
    }
    $buildScript = [System.IO.Path]::GetFullPath($BuildScriptPath)
    $helperScript = Join-Path $ProjectRoot 'tools\NativeBuildReceipt.ps1'
    foreach ($required in @(
            $DllPath, $releasePath, $lockPath, $buildScript, $helperScript)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Build receipt input is missing: $required"
        }
    }
    $release = Get-Content -LiteralPath $releasePath -Raw |
        ConvertFrom-Json
    $dll = Get-Item -LiteralPath $DllPath
    $receipt = [ordered]@{
        schema_version = 1
        name = 'DragonSwordNativeWorldRadarPostRender'
        version = [string]$release.version
        configuration = $Configuration
        ue4ss_variant = $UE4SSVariant
        created_utc = [DateTime]::UtcNow.ToString('o')
        dll = [ordered]@{
            size = [int64]$dll.Length
            sha256 = (Get-FileHash -LiteralPath $dll.FullName `
                    -Algorithm SHA256).Hash
        }
        compiled_source_sha256 = Get-DsnwrCompiledSourceDigest `
            -ProjectRoot $ProjectRoot
        build_lock_sha256 = (Get-FileHash -LiteralPath $lockPath `
                -Algorithm SHA256).Hash
        build_script_sha256 = (Get-FileHash -LiteralPath $buildScript `
                -Algorithm SHA256).Hash
        receipt_helper_sha256 = (Get-FileHash -LiteralPath $helperScript `
                -Algorithm SHA256).Hash
        release_tools_sha256 = Get-DsnwrReleaseToolDigest `
            -ProjectRoot $ProjectRoot
        toolchain = [ordered]@{
            cmake = $CMakeVersion
            ninja = $NinjaVersion
            msvc = $CompilerVersion
            rust = $RustVersion
        }
    }
    $parent = Split-Path -Parent $ReceiptPath
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    $receipt | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $ReceiptPath -Encoding UTF8
    return $receipt
}

function Test-DsnwrNativeBuildReceipt {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [Parameter(Mandatory = $true)][string]$ReceiptPath,
        [ValidateSet('ExperimentalNested', 'StableRoot')]
        [string]$UE4SSVariant = 'ExperimentalNested',
        [string]$BuildScriptPath,
        [string]$BuildLockPath
    )

    foreach ($required in @($DllPath, $ReceiptPath)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Native build receipt input is missing: $required"
        }
    }
    $receipt = Get-Content -LiteralPath $ReceiptPath -Raw |
        ConvertFrom-Json
    $releasePath = Join-Path $ProjectRoot 'metadata\release.json'
    if ([string]::IsNullOrWhiteSpace($BuildLockPath)) {
        $BuildLockPath = if ($UE4SSVariant -eq 'StableRoot') {
            Join-Path $ProjectRoot 'metadata\native-build-stable-lock.json'
        } else {
            Join-Path $ProjectRoot 'metadata\native-build-lock.json'
        }
    }
    $lockPath = [System.IO.Path]::GetFullPath($BuildLockPath)
    if ([string]::IsNullOrWhiteSpace($BuildScriptPath)) {
        $BuildScriptPath = if ($UE4SSVariant -eq 'StableRoot') {
            Join-Path $ProjectRoot 'tools\Build-Native-Stable.ps1'
        } else {
            Join-Path $ProjectRoot 'tools\Build-Native.ps1'
        }
    }
    $buildScript = [System.IO.Path]::GetFullPath($BuildScriptPath)
    $helperScript = Join-Path $ProjectRoot 'tools\NativeBuildReceipt.ps1'
    $release = Get-Content -LiteralPath $releasePath -Raw |
        ConvertFrom-Json
    $dll = Get-Item -LiteralPath $DllPath
    $failures = [System.Collections.Generic.List[string]]::new()
    if ([int]$receipt.schema_version -ne 1) { $failures.Add('schema') }
    if ([string]$receipt.name -ne 'DragonSwordNativeWorldRadarPostRender') {
        $failures.Add('name')
    }
    if ([string]$receipt.version -ne [string]$release.version) {
        $failures.Add('version')
    }
    if ([string]$receipt.configuration -ne 'Game__Shipping__Win64') {
        $failures.Add('configuration')
    }
    if ([string]$receipt.ue4ss_variant -ne $UE4SSVariant) {
        $failures.Add('ue4ss-variant')
    }
    $created = [DateTime]::MinValue
    if (-not [DateTime]::TryParse(
            [string]$receipt.created_utc,
            [System.Globalization.CultureInfo]::InvariantCulture,
            [System.Globalization.DateTimeStyles]::RoundtripKind,
            [ref]$created)) {
        $failures.Add('created-utc')
    }
    $expectedMsvc = if ($UE4SSVariant -eq 'StableRoot') { '^19\.38\.' } else { '^19\.44\.' }
    $expectedRust = if ($UE4SSVariant -eq 'StableRoot') { '^rustc 1\.73\.0 ' } else { '^rustc 1\.97\.1 ' }
    if ([string]$receipt.toolchain.cmake -notmatch '^cmake version 3\.29\.6$' `
        -or [string]$receipt.toolchain.ninja -notmatch '^1\.' `
        -or [string]$receipt.toolchain.msvc -notmatch $expectedMsvc `
        -or [string]$receipt.toolchain.rust -notmatch $expectedRust) {
        $failures.Add('toolchain')
    }
    if ([int64]$receipt.dll.size -ne [int64]$dll.Length `
        -or [string]$receipt.dll.sha256 -ne `
            (Get-FileHash -LiteralPath $dll.FullName -Algorithm SHA256).Hash) {
        $failures.Add('dll')
    }
    if ([string]$receipt.compiled_source_sha256 -ne `
        (Get-DsnwrCompiledSourceDigest -ProjectRoot $ProjectRoot)) {
        $failures.Add('compiled-source')
    }
    if ([string]$receipt.build_lock_sha256 -ne `
        (Get-FileHash -LiteralPath $lockPath -Algorithm SHA256).Hash) {
        $failures.Add('build-lock')
    }
    if ([string]$receipt.build_script_sha256 -ne `
        (Get-FileHash -LiteralPath $buildScript -Algorithm SHA256).Hash) {
        $failures.Add('build-script')
    }
    if ([string]$receipt.receipt_helper_sha256 -ne `
        (Get-FileHash -LiteralPath $helperScript -Algorithm SHA256).Hash) {
        $failures.Add('receipt-helper')
    }
    if ([string]$receipt.release_tools_sha256 -ne `
        (Get-DsnwrReleaseToolDigest -ProjectRoot $ProjectRoot)) {
        $failures.Add('release-tools')
    }
    if ($failures.Count -ne 0) {
        throw "Native build receipt is stale or invalid: $($failures -join ', ')"
    }
    return $true
}
