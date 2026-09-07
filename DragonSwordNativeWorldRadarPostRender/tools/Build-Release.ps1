[CmdletBinding()]
param(
    [switch]$SkipNativeBuild,
    [string]$UE4SSRoot,
    [string]$SdkRoot,
    [string]$ImGuiColorTextEditRoot,
    [string]$IconFontCppHeadersRoot,
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$VsDevCmdPath,
    [string]$RustupHome,
    [string]$CargoHome,
    [string]$NativeBuildDirectory,
    [Parameter(Mandatory)][string]$InstallerTestGameExecutable,
    [Parameter(Mandatory)][string]$InstallerTestExperimentalUE4SSDll,
    [Parameter(Mandatory)][string]$InstallerTestExperimentalDwmapiDll,
    [Parameter(Mandatory)][string]$InstallerTestWorkingDirectory,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$version = '2.3.0'
if (-not $UE4SSRoot) { $UE4SSRoot = Join-Path $projectRoot '.sdk\RE-UE4SS' }
if (-not $SdkRoot) { $SdkRoot = Join-Path $projectRoot '.sdk' }
if (-not $ImGuiColorTextEditRoot) {
    $ImGuiColorTextEditRoot = Join-Path $SdkRoot 'ImGuiColorTextEditPinned'
}
if (-not $IconFontCppHeadersRoot) {
    $IconFontCppHeadersRoot = Join-Path $SdkRoot 'IconFontCppHeaders'
}
if (-not $NativeBuildDirectory) {
    $NativeBuildDirectory = Join-Path $projectRoot 'dist\work\build\native'
} elseif (-not [IO.Path]::IsPathRooted($NativeBuildDirectory)) {
    $NativeBuildDirectory = Join-Path $projectRoot $NativeBuildDirectory
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot "dist\final-$version"
}

if (Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue) {
    throw 'Refusing to build a release while DragonSword Awakening is running.'
}

if (-not $SkipNativeBuild) {
    $nativeParameters = @{
        UE4SSRoot = $UE4SSRoot
        SdkRoot = $SdkRoot
        ImGuiColorTextEditRoot = $ImGuiColorTextEditRoot
        IconFontCppHeadersRoot = $IconFontCppHeadersRoot
        BuildDirectory = $NativeBuildDirectory
    }
    foreach ($optional in @(
            'CMakePath', 'NinjaPath', 'VsDevCmdPath', 'RustupHome', 'CargoHome')) {
        $value = Get-Variable -Name $optional -ValueOnly
        if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
            $nativeParameters[$optional] = $value
        }
    }
    & (Join-Path $PSScriptRoot 'Build-Native.ps1') @nativeParameters | Out-Null
}

$nativeDll = Join-Path ([IO.Path]::GetFullPath($NativeBuildDirectory)) 'main.dll'
$nativeReceipt = Join-Path ([IO.Path]::GetFullPath($NativeBuildDirectory)) `
    'native-build-receipt.json'
. (Join-Path $PSScriptRoot 'NativeBuildReceipt.ps1')
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
    -DllPath $nativeDll -ReceiptPath $nativeReceipt | Out-Null

& (Join-Path $PSScriptRoot 'Build-Core.ps1') | Out-Null
& (Join-Path $PSScriptRoot 'Verify-NativeCompactRenderer.ps1') | Out-Null
& (Join-Path $PSScriptRoot 'Verify-NativeWorldMapCanary.ps1') | Out-Null
& (Join-Path $PSScriptRoot 'Verify-PostRenderCanary.ps1') | Out-Null
& (Join-Path $PSScriptRoot 'Verify-ReleaseHygiene.ps1') `
    -NativeBuildDirectory $NativeBuildDirectory | Out-Null

$installerResult = & (Join-Path $PSScriptRoot 'Build-Installer.ps1') `
    -PluginDll $nativeDll -NativeBuildReceipt $nativeReceipt
$installerResult = @($installerResult)[-1]
$installerPath = [string]$installerResult.installer

$installerTests = & (Join-Path $PSScriptRoot 'Test-Installer.ps1') `
    -InstallerExe $installerPath `
    -SupportedGameExecutable $InstallerTestGameExecutable `
    -ExperimentalUE4SSDll $InstallerTestExperimentalUE4SSDll `
    -ExperimentalDwmapiDll $InstallerTestExperimentalDwmapiDll `
    -WorkingDirectory $InstallerTestWorkingDirectory
if ([int]$installerTests.expected -ne 20 -or
    [int]$installerTests.passed -ne 20 -or
    [int]$installerTests.failed -ne 0 -or
    [int]$installerTests.skipped -ne 0 -or
    [string]$installerTests.release_gate -ne 'PASSED' -or
    -not [bool]$installerTests.sources_unchanged -or
    -not [bool]$installerTests.fixtures_cleaned) {
    throw 'Installer release gate did not return exactly 20 passed, 0 failed, and 0 skipped.'
}

$packageResult = & (Join-Path $PSScriptRoot 'Build-FinalPackages.ps1') `
    -InstallerExe $installerPath -PluginDll $nativeDll `
    -NativeBuildReceipt $nativeReceipt `
    -OutputDirectory $OutputDirectory
$packageResult = @($packageResult)[-1]

$manualTests = & (Join-Path $PSScriptRoot 'Test-ManualInstall.ps1') `
    -InstallerExe $installerPath `
    -PluginDll $nativeDll `
    -NativeBuildReceipt $nativeReceipt `
    -ManualNoUE4SSArchive ([string]$packageResult.manual_no_ue4ss_archive) `
    -ManualWithUE4SSArchive ([string]$packageResult.manual_with_ue4ss_archive) `
    -SupportedGameExecutable $InstallerTestGameExecutable `
    -WorkingDirectory $InstallerTestWorkingDirectory
if ([int]$manualTests.expected -ne 2 -or
    [int]$manualTests.passed -ne 2 -or
    [int]$manualTests.failed -ne 0 -or
    [int]$manualTests.skipped -ne 0 -or
    [string]$manualTests.release_gate -ne 'PASSED' -or
    [string]$manualTests.setup_manual_payload_equivalence -ne 'PASSED' -or
    [string]$manualTests.manual_copy_layout_validation -ne 'PASSED' -or
    [string]$manualTests.manual_clean_target_policy_validation -ne 'PASSED' -or
    -not [bool]$manualTests.sources_unchanged -or
    -not [bool]$manualTests.fixtures_cleaned) {
    throw 'Manual-copy release gate did not return exactly 2 passed, 0 failed, and 0 skipped with byte-equivalent payloads and exact clean-target layouts.'
}

$releaseManifestPath = [string]$packageResult.release_manifest
$releaseManifest = Get-Content -LiteralPath $releaseManifestPath -Raw |
    ConvertFrom-Json
$releaseManifest.runtime_payload.setup_manual_equivalence = 'PASSED'
$releaseManifest.source_validation = 'PASSED'
$releaseManifest.build_validation = 'PASSED'
$releaseManifest.installer_matrix = [ordered]@{
    expected = 20
    passed = [int]$installerTests.passed
    failed = [int]$installerTests.failed
    skipped = [int]$installerTests.skipped
    release_gate = [string]$installerTests.release_gate
}
$releaseManifest.manual_install_matrix = [ordered]@{
    expected = 2
    passed = [int]$manualTests.passed
    failed = [int]$manualTests.failed
    skipped = [int]$manualTests.skipped
    release_gate = [string]$manualTests.release_gate
    setup_manual_payload_equivalence =
        [string]$manualTests.setup_manual_payload_equivalence
    manual_copy_layout_validation =
        [string]$manualTests.manual_copy_layout_validation
    manual_clean_target_policy_validation =
        [string]$manualTests.manual_clean_target_policy_validation
}
[IO.File]::WriteAllText(
    $releaseManifestPath,
    (($releaseManifest | ConvertTo-Json -Depth 10) + "`r`n"),
    [Text.UTF8Encoding]::new($false))

[pscustomobject]@{
    version = $version
    native_dll = $nativeDll
    native_sha256 = (Get-FileHash -LiteralPath $nativeDll -Algorithm SHA256).Hash
    installer = $installerPath
    installer_sha256 = [string]$installerResult.installer_sha256
    installer_tests_passed = [int]$installerTests.passed
    installer_archive = [string]$packageResult.installer_archive
    installer_archive_sha256 = [string]$packageResult.installer_archive_sha256
    manual_no_ue4ss_archive = [string]$packageResult.manual_no_ue4ss_archive
    manual_no_ue4ss_sha256 = [string]$packageResult.manual_no_ue4ss_sha256
    manual_with_ue4ss_archive = [string]$packageResult.manual_with_ue4ss_archive
    manual_with_ue4ss_sha256 = [string]$packageResult.manual_with_ue4ss_sha256
    release_manifest = $releaseManifestPath
    sha256sums = [string]$packageResult.sha256sums
    manual_tests_passed = [int]$manualTests.passed
    setup_manual_payload_equivalence =
        [string]$manualTests.setup_manual_payload_equivalence
    manual_copy_layout_validation =
        [string]$manualTests.manual_copy_layout_validation
    manual_clean_target_policy_validation =
        [string]$manualTests.manual_clean_target_policy_validation
    gameplay_acceptance = 'NOT_VALIDATED'
    deployment = 'NOT_PERFORMED'
}
