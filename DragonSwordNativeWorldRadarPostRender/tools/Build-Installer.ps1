[CmdletBinding()]
param(
    [string]$PluginDll,
    [string]$NativeBuildReceipt,
    [string]$ExperimentalUE4SSDll,
    [string]$ExperimentalDwmapiDll,
    [string]$ExperimentalUsmap,
    [string]$ExperimentalSettingsIni,
    [string]$ThirdPartyNoticesPath,
    [string]$OutputDirectory,
    [string]$CscPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$installerRoot = Join-Path $projectRoot 'installer'
$version = '2.2.1'
$runtimeLabel = 'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1'
$gameCompatibilityPolicy = 'pe32plus-x64-runtime-unique-owner-pointer-pattern'
$experimentalUE4SSHash = 'F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1'
$experimentalDwmapiHash = '30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B'
$experimentalUsmapHash = '0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47'
$experimentalSettingsHash = '4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74'

if (-not $PluginDll) {
    $PluginDll = Join-Path $projectRoot 'dist\work\build\native\main.dll'
}
if (-not $NativeBuildReceipt) {
    $NativeBuildReceipt = Join-Path `
        (Split-Path -Parent $PluginDll) 'native-build-receipt.json'
}
$defaultWin64 = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Binaries\Win64'
if (-not $ExperimentalUE4SSDll) {
    $ExperimentalUE4SSDll = Join-Path $defaultWin64 'ue4ss\UE4SS.dll'
}
if (-not $ExperimentalDwmapiDll) {
    $ExperimentalDwmapiDll = Join-Path $defaultWin64 'dwmapi.dll'
}
if (-not $ExperimentalUsmap) {
    $ExperimentalUsmap = Join-Path $defaultWin64 'ue4ss\DS-5.3.2-0+UE5-1c1a1497.usmap'
}
if (-not $ExperimentalSettingsIni) {
    $ExperimentalSettingsIni = Join-Path $defaultWin64 'ue4ss\UE4SS-settings.ini'
}
if (-not $ThirdPartyNoticesPath) {
    $ThirdPartyNoticesPath = Join-Path $projectRoot 'THIRD_PARTY_NOTICES.txt'
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot 'dist\work\build\installer'
}

. (Join-Path $PSScriptRoot 'ReleaseLayout.ps1')

function Resolve-RequiredFile {
    param([string]$Path, [string]$Description)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Assert-X64ReleaseDll {
    param([string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 256 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
        throw "Native Radar payload is not a PE image: $Path"
    }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    if ($peOffset -lt 0 -or $peOffset + 24 -gt $bytes.Length -or
        $bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
        $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
        throw "Native Radar payload has an invalid PE header: $Path"
    }
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    $characteristics = [BitConverter]::ToUInt16($bytes, $peOffset + 22)
    if ($machine -ne 0x8664 -or ($characteristics -band 0x2000) -eq 0) {
        throw "Native Radar payload must be an x64 DLL: $Path"
    }
    $ascii = [Text.Encoding]::ASCII.GetString($bytes)
    $unicode = [Text.Encoding]::Unicode.GetString($bytes)
    if ((-not $ascii.Contains($version) -and -not $unicode.Contains($version)) -or
        (-not $ascii.Contains($runtimeLabel) -and -not $unicode.Contains($runtimeLabel))) {
        throw 'Native Radar payload does not contain the 2.2.1 release identity.'
    }
}

$dll = Resolve-RequiredFile $PluginDll 'Experimental-nested Native Radar DLL'
$nativeReceipt = Resolve-RequiredFile `
    $NativeBuildReceipt 'Native build receipt'
$experimentalLoader = Resolve-RequiredFile $ExperimentalUE4SSDll 'Pinned Experimental UE4SS.dll'
$experimentalProxy = Resolve-RequiredFile $ExperimentalDwmapiDll 'Pinned Experimental dwmapi.dll'
$experimentalMap = Resolve-RequiredFile $ExperimentalUsmap 'Pinned DragonSword usmap'
$experimentalSettings = Resolve-RequiredFile $ExperimentalSettingsIni 'Tested Experimental UE4SS settings'
$notices = Resolve-RequiredFile $ThirdPartyNoticesPath 'Third-party notices'
Assert-X64ReleaseDll $dll
if ((Get-Sha256 $experimentalLoader) -ne $experimentalUE4SSHash -or
    (Get-Sha256 $experimentalProxy) -ne $experimentalDwmapiHash -or
    (Get-Sha256 $experimentalMap) -ne $experimentalUsmapHash -or
    (Get-Sha256 $experimentalSettings) -ne $experimentalSettingsHash) {
    throw 'Pinned Experimental UE4SS loader resources do not match the tested release identity.'
}

$release = Get-Content -LiteralPath (Join-Path $projectRoot 'metadata\release.json') -Raw |
    ConvertFrom-Json
if ([string]$release.version -ne $version -or
    [string]$release.runtime_label -ne $runtimeLabel) {
    throw 'Release metadata does not match the installer identity.'
}
if ([string]$release.public_release_clearance.status -ne 'blocked' -or
    [bool]$release.public_release_clearance.binary_publication_authorized) {
    throw 'Public release clearance must remain blocked until provenance review is complete.'
}

$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $outputRoot) {
    $outputItem = Get-Item -LiteralPath $outputRoot -Force
    if (-not $outputItem.PSIsContainer -or
        ($outputItem.Attributes -band
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Installer output root is not a plain directory.'
    }
} else {
    New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
}
$generatedFull = [System.IO.Path]::GetFullPath(
    (Join-Path $outputRoot '.generated'))
if (-not [string]::Equals(
        [System.IO.Path]::GetDirectoryName($generatedFull),
        $outputRoot.TrimEnd('\'),
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Generated installer path escaped its output root.'
}
if (Test-Path -LiteralPath $generatedFull) {
    $generatedItem = Get-Item -LiteralPath $generatedFull -Force
    if (-not $generatedItem.PSIsContainer) {
        throw 'Generated installer path is not a directory.'
    }
    Assert-DsnwrNoReparseTree -Root $generatedFull `
        -Description 'Generated installer tree'
    Remove-Item -LiteralPath $generatedFull -Recurse -Force
}
New-Item -ItemType Directory -Path $generatedFull -Force | Out-Null
$runtimeRoot = Join-Path $generatedFull 'runtime'
New-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $dll `
    -BuildReceiptPath $nativeReceipt `
    -DestinationRoot $runtimeRoot | Out-Null
Test-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $dll `
    -BuildReceiptPath $nativeReceipt `
    -PayloadRoot $runtimeRoot | Out-Null
foreach ($root in @($runtimeRoot)) {
    if (Get-ChildItem -LiteralPath $root -Recurse -Force -File |
        Where-Object {
            $_.Name -ieq 'enabled.txt' -or
            $_.FullName -match '(?i)\\runtime\\(logs|backups|diagnostics)\\'
        }) {
        throw "Forbidden local state entered a public runtime payload: $root"
    }
}

$runtimeFiles = @(Get-ChildItem -LiteralPath $runtimeRoot -Recurse -Force -File |
    Sort-Object { (Get-DsnwrRelativePath -Root $runtimeRoot -Path $_.FullName).ToLowerInvariant() })
if ($runtimeFiles.Count -le 0 -or $runtimeFiles.Count -gt 128) {
    throw "Runtime payload file count is outside the installer contract: $($runtimeFiles.Count)"
}

function New-InstallerRuntimeZip {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][object[]]$Files,
        [Parameter(Mandatory = $true)][string]$Destination
    )
    $zipStream = [System.IO.File]::Open(
        $Destination,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None)
    try {
        $archive = [System.IO.Compression.ZipArchive]::new(
            $zipStream,
            [System.IO.Compression.ZipArchiveMode]::Create,
            $true)
        try {
            foreach ($file in $Files) {
                $relative = Get-DsnwrRelativePath -Root $Root -Path $file.FullName
                $entry = $archive.CreateEntry(
                    $relative,
                    [System.IO.Compression.CompressionLevel]::Optimal)
                $entryStream = $entry.Open()
                try {
                    $input = [System.IO.File]::OpenRead($file.FullName)
                    try { $input.CopyTo($entryStream) }
                    finally { $input.Dispose() }
                } finally {
                    $entryStream.Dispose()
                }
            }
        } finally {
            $archive.Dispose()
        }
    } finally {
        $zipStream.Dispose()
    }
}

Add-Type -AssemblyName System.IO.Compression
$runtimeZip = Join-Path $generatedFull 'experimental-runtime.zip'
New-InstallerRuntimeZip -Root $runtimeRoot -Files $runtimeFiles `
    -Destination $runtimeZip

$manifestPath = Join-Path $generatedFull 'payload-manifest.ini'
$manifestLines = [System.Collections.Generic.List[string]]::new()
foreach ($line in @(
        "version=$version",
        "runtime_label=$runtimeLabel",
        "game_compatibility_policy=$gameCompatibilityPolicy",
        "experimental_ue4ss_sha256=$experimentalUE4SSHash",
        "experimental_dwmapi_sha256=$experimentalDwmapiHash",
        "experimental_runtime_zip_sha256=$(Get-Sha256 $runtimeZip)",
        "experimental_runtime_file_count=$($runtimeFiles.Count)",
        "third_party_notices_sha256=$(Get-Sha256 $notices)")) {
    $manifestLines.Add($line)
}
foreach ($variant in @(
        [pscustomobject]@{ Name = 'experimental'; Root = $runtimeRoot; Files = $runtimeFiles })) {
    for ($index = 0; $index -lt $variant.Files.Count; ++$index) {
        $file = $variant.Files[$index]
        $prefix = '{0}_file_{1:D3}_' -f $variant.Name, $index
        $relative = Get-DsnwrRelativePath -Root $variant.Root -Path $file.FullName
        $manifestLines.Add("${prefix}path=$relative")
        $manifestLines.Add("${prefix}size=$($file.Length)")
        $manifestLines.Add("${prefix}sha256=$(Get-Sha256 $file.FullName)")
    }
}
[System.IO.File]::WriteAllLines(
    $manifestPath,
    $manifestLines,
    [System.Text.UTF8Encoding]::new($false))

if (-not $CscPath) {
    $candidate = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\Roslyn\csc.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf) { $CscPath = $candidate }
}
$csc = Resolve-RequiredFile $CscPath 'Visual Studio Roslyn C# compiler'
$referenceRoot = 'C:\Program Files (x86)\Reference Assemblies\Microsoft\Framework\.NETFramework\v4.8'
if (-not (Test-Path -LiteralPath (Join-Path $referenceRoot 'mscorlib.dll') -PathType Leaf)) {
    throw '.NET Framework 4.8 reference assemblies were not found.'
}

$outputExe = Join-Path $outputRoot "DragonSwordNativeWorldRadarPostRender-Setup-$version.exe"
if (Test-Path -LiteralPath $outputExe) {
    $outputExeItem = Get-Item -LiteralPath $outputExe -Force
    if ($outputExeItem.PSIsContainer -or
        ($outputExeItem.Attributes -band
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Existing installer output is not a plain file.'
    }
    Remove-Item -LiteralPath $outputExe -Force
}
$sources = @(Get-ChildItem -LiteralPath $installerRoot -Filter '*.cs' -File | Sort-Object Name)
if ($sources.Count -ne 4) {
    throw "Installer C# source set must contain exactly four files; found $($sources.Count)."
}
$arguments = [System.Collections.Generic.List[string]]::new()
foreach ($argument in @(
        '/nologo', '/target:winexe', '/platform:x64', '/optimize+', '/debug-',
        '/deterministic+', '/highentropyva+', '/utf8output', '/nostdlib+',
        "/out:$outputExe",
        "/win32manifest:$(Join-Path $installerRoot 'app.manifest')")) {
    $arguments.Add($argument)
}
foreach ($reference in @(
        'mscorlib.dll', 'System.dll', 'System.Core.dll', 'System.Drawing.dll',
        'System.Windows.Forms.dll', 'System.Web.dll', 'System.Web.Extensions.dll',
        'System.IO.Compression.dll',
        'System.IO.Compression.FileSystem.dll')) {
    $arguments.Add("/reference:$(Join-Path $referenceRoot $reference)")
}
foreach ($resource in @(
        @($manifestPath, 'Payload.Manifest.ini'),
        @($runtimeZip, 'Payload.ExperimentalRuntime.zip'),
        @($experimentalLoader, 'Payload.ExperimentalUE4SS.dll'),
        @($experimentalProxy, 'Payload.ExperimentalDwmapi.dll'),
        @($experimentalMap, 'Payload.Experimental.usmap'),
        @($experimentalSettings, 'Payload.ExperimentalUE4SS-settings.ini'),
        @($notices, 'Payload.ThirdPartyNotices.txt'))) {
    $arguments.Add("/resource:$($resource[0]),$($resource[1])")
}
foreach ($source in $sources) { $arguments.Add($source.FullName) }

& $csc $arguments.ToArray()
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $outputExe -PathType Leaf)) {
    throw "Installer compilation failed: $LASTEXITCODE"
}
$signature = Get-AuthenticodeSignature -LiteralPath $outputExe
if ([string]$signature.Status -ne 'NotSigned') {
    throw "The technical release candidate must remain explicitly unsigned; found $($signature.Status)."
}
$installerHash = Get-Sha256 $outputExe
$checksumPath = "$outputExe.sha256"
[System.IO.File]::WriteAllText(
    $checksumPath,
    "$installerHash  $([System.IO.Path]::GetFileName($outputExe))`r`n",
    [System.Text.UTF8Encoding]::new($false))

[pscustomobject]@{
    version = $version
    installer = $outputExe
    installer_sha256 = $installerHash
    checksum = $checksumPath
    experimental_runtime_zip_sha256 = Get-Sha256 $runtimeZip
    experimental_runtime_file_count = $runtimeFiles.Count
    plugin_sha256 = Get-Sha256 $dll
    installer_unsigned = $true
}
