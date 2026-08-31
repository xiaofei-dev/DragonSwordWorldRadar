[CmdletBinding()]
param(
    [string]$ModsRoot = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Binaries\Win64\ue4ss\Mods',
    [ValidateSet('Preserve', 'Enable', 'Disable')]
    [string]$Diagnostics = 'Preserve'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectRootFull = [System.IO.Path]::GetFullPath($projectRoot).TrimEnd('\')
$nativeName = 'DragonSwordNativeWorldRadarPostRender'
$stableName = 'DragonSwordWorldRadarObjectState'
$target = Join-Path $ModsRoot $nativeName
$stableTarget = Join-Path $ModsRoot $stableName
$modsFile = Join-Path $ModsRoot 'mods.txt'
$sourceDll = Join-Path $projectRoot 'dist\work\build\native\main.dll'
$sourceBuildReceipt = Join-Path $projectRoot `
    'dist\work\build\native\native-build-receipt.json'
$sourceVisibilityConfig = Join-Path $projectRoot 'config\visibility.ini'
$sourceDiagnosticsConfig = Join-Path $projectRoot 'config\diagnostics.ini'
$sourceData = Join-Path $projectRoot 'src\data\generated'
$sourceOverrides = Join-Path $projectRoot 'src\data\defaults\treasure_overrides.txt'
$sourceSqlCipher = Join-Path $projectRoot 'assets\vendor\sqlcipher\e_sqlcipher.dll'
$sourceApacheLicense = Join-Path $projectRoot 'licenses\APACHE-2.0.txt'
$sourceSqlCipherLicense = Join-Path $projectRoot 'licenses\SQLCIPHER.txt'
$sourceFmtLicense = Join-Path $projectRoot 'licenses\FMT.txt'
$sourceUe4ssLicense = Join-Path $projectRoot 'licenses\UE4SS.txt'
$sourceLibTomCryptLicense = Join-Path $projectRoot 'licenses\LIBTOMCRYPT.txt'
$sourceThirdPartyNotices = Join-Path $projectRoot 'THIRD_PARTY_NOTICES.txt'
$sourceMetadata = Join-Path $projectRoot 'metadata\release.json'
$sourceMain = Join-Path $projectRoot 'src\native\main.cpp'

$modsRootFull = [System.IO.Path]::GetFullPath($ModsRoot).TrimEnd('\')
$targetFull = [System.IO.Path]::GetFullPath($target)
$stableTargetFull = [System.IO.Path]::GetFullPath($stableTarget)
foreach ($validatedTarget in @(
        [pscustomobject]@{ Path = $targetFull; Name = $nativeName },
        [pscustomobject]@{ Path = $stableTargetFull; Name = $stableName })) {
    if (-not [string]::Equals(
            [System.IO.Path]::GetDirectoryName($validatedTarget.Path),
            $modsRootFull,
            [System.StringComparison]::OrdinalIgnoreCase) `
        -or -not [string]::Equals(
            [System.IO.Path]::GetFileName($validatedTarget.Path),
            $validatedTarget.Name,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Deployment target escaped the exact Mods root: $($validatedTarget.Path)"
    }
}
# Use only the canonical paths that passed the exact direct-child validation
# for every installed-tree mutation and rollback operation below.
$target = $targetFull
$stableTarget = $stableTargetFull
$modsFile = Join-Path $modsRootFull 'mods.txt'
function Assert-NoReparseTree {
    param([string]$Root, [string]$Description)
    if (-not (Test-Path -LiteralPath $Root)) {
        return
    }

    $pending = [System.Collections.Generic.Stack[string]]::new()
    $pending.Push($Root)
    while ($pending.Count -ne 0) {
        $current = $pending.Pop()
        $item = Get-Item -LiteralPath $current -Force
        if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to use a reparse point in $Description`: $current"
        }
        if (-not $item.PSIsContainer) {
            continue
        }
        foreach ($child in @(Get-ChildItem -LiteralPath $current -Force)) {
            if (($child.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to use a reparse point in $Description`: $($child.FullName)"
            }
            if ($child.PSIsContainer) {
                $pending.Push($child.FullName)
            }
        }
    }
}

function Assert-InstalledDeploymentInputsSafe {
    if (-not (Test-Path -LiteralPath $modsRootFull -PathType Container)) {
        throw "Mods root is not a directory: $modsRootFull"
    }
    $modsRootItem = Get-Item -LiteralPath $modsRootFull -Force
    if (($modsRootItem.Attributes -band `
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing to mutate a reparse-point Mods root: $modsRootFull"
    }
    foreach ($installedTarget in @($target, $stableTarget)) {
        if (-not (Test-Path -LiteralPath $installedTarget)) {
            continue
        }
        $installedItem = Get-Item -LiteralPath $installedTarget -Force
        if (-not $installedItem.PSIsContainer) {
            throw "Installed mod target is not a directory: $installedTarget"
        }
        if (($installedItem.Attributes -band `
                [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to mutate a reparse-point mod target: $installedTarget"
        }
        Assert-NoReparseTree -Root $installedTarget `
            -Description 'an installed mod tree'
    }
    if (Test-Path -LiteralPath $modsFile) {
        $modsItem = Get-Item -LiteralPath $modsFile -Force
        if ($modsItem.PSIsContainer `
            -or ($modsItem.Attributes -band `
                [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "mods.txt must be a regular file: $modsFile"
        }
    }
}

function Assert-DragonSwordStopped {
    param([string]$Operation)

    if (Get-Process -Name 'DSClient-Win64-Shipping' `
            -ErrorAction SilentlyContinue) {
        throw "Refusing to $Operation while DragonSword is running."
    }
}

function Assert-PlainDirectoryIfPresent {
    param([string]$Path, [string]$Description)

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }
    $item = Get-Item -LiteralPath $Path -Force
    if (-not $item.PSIsContainer `
        -or ($item.Attributes -band `
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Description must be a plain directory: $Path"
    }
}

function Read-ModsTextDocument {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $offset = 0
    $preamble = [byte[]]@()
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF `
        -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $encoding = [System.Text.UTF8Encoding]::new($false, $true)
        $offset = 3
        $preamble = [byte[]]@(0xEF, 0xBB, 0xBF)
    } elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF `
        -and $bytes[1] -eq 0xFE) {
        $encoding = [System.Text.UnicodeEncoding]::new($false, $false, $true)
        $offset = 2
        $preamble = [byte[]]@(0xFF, 0xFE)
    } elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE `
        -and $bytes[1] -eq 0xFF) {
        $encoding = [System.Text.UnicodeEncoding]::new($true, $false, $true)
        $offset = 2
        $preamble = [byte[]]@(0xFE, 0xFF)
    } else {
        $encoding = [System.Text.UTF8Encoding]::new($false, $true)
    }
    try {
        $text = $encoding.GetString($bytes, $offset, $bytes.Length - $offset)
    } catch [System.Text.DecoderFallbackException] {
        # Legacy mods.txt files can be ANSI. Preserve that exact code page
        # rather than silently changing unrelated lines to UTF-8.
        $ansiCodePage = [System.Globalization.CultureInfo]::CurrentCulture.TextInfo.ANSICodePage
        try {
            $encoding = [System.Text.Encoding]::GetEncoding(
                $ansiCodePage,
                [System.Text.EncoderFallback]::ExceptionFallback,
                [System.Text.DecoderFallback]::ExceptionFallback)
            $text = $encoding.GetString($bytes)
        } catch {
            throw "mods.txt is neither strict UTF-8 nor valid current ANSI code page $ansiCodePage."
        }
        $preamble = [byte[]]@()
    }
    $lineBreaks = @([regex]::Matches($text, "`r`n|`n|`r") |
            ForEach-Object Value | Sort-Object -Unique)
    if ($lineBreaks.Count -gt 1) {
        throw 'mods.txt uses mixed line endings; refusing to normalize unrelated content.'
    }
    $newLine = if ($text.Contains("`r`n")) {
        "`r`n"
    } elseif ($text.Contains("`n")) {
        "`n"
    } elseif ($text.Contains("`r")) {
        "`r"
    } else {
        [Environment]::NewLine
    }
    $hasTrailingNewLine = $text.EndsWith("`r") -or $text.EndsWith("`n")
    $split = if ($text.Length -eq 0) {
        @()
    } else {
        @($text -split "`r`n|`n|`r", -1)
    }
    if ($hasTrailingNewLine -and $split.Count -ne 0 `
        -and $split[$split.Count - 1] -eq '') {
        $split = @($split | Select-Object -First ($split.Count - 1))
    }
    return [pscustomobject]@{
        Lines = $split
        Encoding = $encoding
        Preamble = $preamble
        NewLine = $newLine
        HasTrailingNewLine = $hasTrailingNewLine
    }
}

function Write-ModsTextDocumentAtomically {
    param(
        [string]$Path,
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Document
    )

    $text = $Lines -join [string]$Document.NewLine
    if ($Document.HasTrailingNewLine) {
        $text += [string]$Document.NewLine
    }
    $body = $Document.Encoding.GetBytes($text)
    $preamble = [byte[]]$Document.Preamble
    $output = [byte[]]::new($preamble.Length + $body.Length)
    if ($preamble.Length -ne 0) {
        [Array]::Copy($preamble, 0, $output, 0, $preamble.Length)
    }
    if ($body.Length -ne 0) {
        [Array]::Copy($body, 0, $output, $preamble.Length, $body.Length)
    }
    $temporary = Join-Path (Split-Path -Parent $Path) `
        ('.dsnwr-mods-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    $replacementBackup = Join-Path (Split-Path -Parent $Path) `
        ('.dsnwr-mods-backup-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        $stream = [System.IO.File]::Open(
            $temporary, [System.IO.FileMode]::CreateNew,
            [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
        try {
            $stream.Write($output, 0, $output.Length)
            $stream.Flush($true)
        } finally {
            $stream.Dispose()
        }
        [System.IO.File]::Replace(
            $temporary, $Path, $replacementBackup, $true)
    } finally {
        foreach ($cleanup in @($temporary, $replacementBackup)) {
            if (Test-Path -LiteralPath $cleanup) {
                Remove-Item -LiteralPath $cleanup -Force
            }
        }
    }
}

Assert-InstalledDeploymentInputsSafe
Assert-DragonSwordStopped -Operation 'deploy'
foreach ($required in @(
        $sourceDll, $sourceBuildReceipt, $sourceVisibilityConfig,
        $sourceDiagnosticsConfig,
        $sourceData, $sourceOverrides,
        $sourceSqlCipher, $sourceApacheLicense, $sourceSqlCipherLicense,
        $sourceFmtLicense, $sourceUe4ssLicense, $sourceLibTomCryptLicense,
        $sourceThirdPartyNotices,
        $sourceMetadata, $sourceMain, $modsFile)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required deployment input is missing: $required"
    }
}
. (Join-Path $PSScriptRoot 'NativeBuildReceipt.ps1')
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
    -DllPath $sourceDll -ReceiptPath $sourceBuildReceipt | Out-Null

$release = Get-Content -LiteralPath $sourceMetadata -Raw | ConvertFrom-Json
$expectedVersion = [string]$release.version
if (-not $expectedVersion) {
    throw 'Release metadata does not declare a version.'
}
$mainSource = Get-Content -LiteralPath $sourceMain -Raw
if ($mainSource -notmatch ('kVersion\s*=\s*STR\("' + [regex]::Escape($expectedVersion) + '"\)') `
    -or $mainSource -notmatch ('version=' + [regex]::Escape($expectedVersion))) {
    throw "Native source and release metadata version do not match: $expectedVersion"
}

$sourceInputs = @(
    Get-ChildItem -LiteralPath (Join-Path $projectRoot 'src\native') -File -Filter '*.cpp'
    Get-ChildItem -LiteralPath (Join-Path $projectRoot 'src\native') -File -Filter '*.hpp'
    Get-ChildItem -LiteralPath (Join-Path $projectRoot 'include') -Recurse -File
    Get-Item -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt')
)
$latestSourceWrite = ($sourceInputs | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
$dllWrite = (Get-Item -LiteralPath $sourceDll).LastWriteTimeUtc
if ($dllWrite -lt $latestSourceWrite) {
    throw "Native DLL is older than a compiled source input. Rebuild before deployment. DLL=$dllWrite Source=$latestSourceWrite"
}
$dllBytes = [System.IO.File]::ReadAllBytes($sourceDll)
$dllAscii = [System.Text.Encoding]::ASCII.GetString($dllBytes)
$dllUnicode = [System.Text.Encoding]::Unicode.GetString($dllBytes)
if (-not $dllAscii.Contains($expectedVersion) `
    -and -not $dllUnicode.Contains($expectedVersion)) {
    throw "Native DLL does not embed release version $expectedVersion. Rebuild before deployment."
}

$preDeploymentGates = @(
    'Verify-NativeCompactRenderer.ps1',
    'Verify-NativeWorldMapCanary.ps1',
    'Verify-PostRenderCanary.ps1',
    'Verify-ReleaseHygiene.ps1'
)
# Every static gate completes before the backup directory or either installed
# mod tree can be created, moved, or overwritten.
try {
    & (Join-Path $PSScriptRoot 'Build-Core.ps1')
} catch {
    throw "Pre-deployment core tests failed: $($_.Exception.Message)"
}
foreach ($gate in $preDeploymentGates) {
    try {
        & (Join-Path $PSScriptRoot $gate)
    } catch {
        throw "Pre-deployment static gate failed: $gate ($($_.Exception.Message))"
    }
}
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
    -DllPath $sourceDll -ReceiptPath $sourceBuildReceipt | Out-Null

$catalogRules = @(
    [pscustomobject]@{ Name = 'treasure-actors.tsv'; Header = "SaveId`tClassName`tX`tY`tZ"; Minimum = 1600; Maximum = 2500 },
    [pscustomobject]@{ Name = 'boss-actors.tsv'; Header = "Id`tClassName`tX`tY`tZ"; Minimum = 9; Maximum = 9 },
    [pscustomobject]@{ Name = 'assault-actors.tsv'; Header = "Id`tClassName`tX`tY`tZ"; Minimum = 40; Maximum = 40 },
    [pscustomobject]@{ Name = 'area-quests.tsv'; Header = "Id`tX`tY`tZ"; Minimum = 147; Maximum = 147 }
)
foreach ($rule in $catalogRules) {
    $catalogPath = Join-Path $sourceData $rule.Name
    if (-not (Test-Path -LiteralPath $catalogPath -PathType Leaf)) {
        throw "Required native catalog is missing: $catalogPath"
    }
    $catalogLines = @(Get-Content -LiteralPath $catalogPath)
    if ($catalogLines.Count -lt 2 -or $catalogLines[0] -ne $rule.Header) {
        throw "Native catalog header is invalid: $catalogPath"
    }
    $rowCount = $catalogLines.Count - 1
    if ($rowCount -lt $rule.Minimum -or $rowCount -gt $rule.Maximum) {
        throw "Native catalog row count is invalid: $catalogPath ($rowCount)"
    }
}

$renderCatalogPath = Join-Path $sourceData 'treasures.lua'
if (-not (Test-Path -LiteralPath $renderCatalogPath -PathType Leaf)) {
    throw "Required section-aware render catalog is missing: $renderCatalogPath"
}
$renderCatalogRows = @(Select-String -LiteralPath $renderCatalogPath -Pattern '\{ save_id = ')
if ($renderCatalogRows.Count -ne 1693) {
    throw "Section-aware render catalog row count is invalid: $renderCatalogPath ($($renderCatalogRows.Count))"
}
$worldMapRows = @($renderCatalogRows | Where-Object {
        $_.Line -match 'section = "[^"]*100"'
    })
if ($worldMapRows.Count -ne 1506) {
    throw "Section-aware map-100 row count is invalid: $renderCatalogPath ($($worldMapRows.Count))"
}
$renderCatalogIds = @($renderCatalogRows | ForEach-Object {
        if ($_.Line -notmatch '\{ save_id = ([0-9]+), section = "[0-9]+"') {
            throw "Section-aware render catalog row is malformed: $($_.Line)"
        }
        [int64]$Matches[1]
    })
if (@($renderCatalogIds | Sort-Object -Unique).Count -ne 1693) {
    throw "Section-aware render catalog contains duplicate SaveId values: $renderCatalogPath"
}

$generatedLuaRules = @(
    [pscustomobject]@{ Name = 'bosses.lua'; Pattern = '\{ boss_id = '; Count = 9 },
    [pscustomobject]@{ Name = 'assaults.lua'; Pattern = '\{ place_id = '; Count = 40 },
    [pscustomobject]@{ Name = 'moles.lua'; Pattern = '\{ mini_game_id = '; Count = 83 }
)
foreach ($rule in $generatedLuaRules) {
    $catalogPath = Join-Path $sourceData $rule.Name
    if (-not (Test-Path -LiteralPath $catalogPath -PathType Leaf)) {
        throw "Required render catalog is missing: $catalogPath"
    }
    $rows = @(Select-String -LiteralPath $catalogPath -Pattern $rule.Pattern)
    if ($rows.Count -ne $rule.Count) {
        throw "$($rule.Name) row count mismatch: expected $($rule.Count), found $($rows.Count)."
    }
}
$moleRows = @(Select-String -LiteralPath (Join-Path $sourceData 'moles.lua') -Pattern '\{ mini_game_id = ')
$flyRows = @($moleRows | Where-Object {
        $_.Line -match 'mini_game_id = 11\d{3}' -and $_.Line -notmatch 'mini_game_id = 11024'
    })
$hammerRows = @($moleRows | Where-Object { $_.Line -match 'mini_game_id = 12\d{3}' })
$waveRows = @($moleRows | Where-Object { $_.Line -match 'mini_game_id = 13\d{3}' })
if ($flyRows.Count -ne 33 -or $hammerRows.Count -ne 40 -or $waveRows.Count -ne 10) {
    throw 'Mini-game catalog shape mismatch: expected 33 Fly, 40 Mole, and 10 Wave rows.'
}
$assaultActorRows = @(Import-Csv -LiteralPath (Join-Path $sourceData 'assault-actors.tsv') -Delimiter "`t")
$assaultRenderRows = @(Select-String -LiteralPath (Join-Path $sourceData 'assaults.lua') -Pattern '\{ place_id = ')
for ($index = 0; $index -lt 40; $index++) {
    if ($assaultRenderRows[$index].Line -notmatch 'place_id = ([0-9]+).*cid = ([0-9]+).*map_id = 100') {
        throw "Assault render row is malformed at index $index."
    }
    if ([int64]$assaultActorRows[$index].Id -ne [int64]$Matches[1]) {
        throw "Assault actor and render catalogs are misaligned at index $index."
    }
}
$bossActorIds = @(Import-Csv -LiteralPath (Join-Path $sourceData 'boss-actors.tsv') -Delimiter "`t" | ForEach-Object { [int64]$_.Id })
$bossRenderIds = @(Select-String -LiteralPath (Join-Path $sourceData 'bosses.lua') -Pattern '\{ boss_id = ' | ForEach-Object {
        if ($_.Line -notmatch 'boss_id = ([0-9]+).*map_id = 100') {
            throw "Boss render row is malformed: $($_.Line)"
        }
        [int64]$Matches[1]
    })
if (@($bossActorIds | Where-Object { $_ -notin $bossRenderIds }).Count -ne 0) {
    throw 'Boss actor and render catalog identities do not match.'
}
Assert-DragonSwordStopped -Operation 'prepare a deployment backup'
Assert-InstalledDeploymentInputsSafe
$distRootFull = [System.IO.Path]::GetFullPath(
    (Join-Path $projectRootFull 'dist'))
$workRootFull = [System.IO.Path]::GetFullPath(
    (Join-Path $distRootFull 'work'))
$deploymentRootFull = [System.IO.Path]::GetFullPath(
    (Join-Path $workRootFull 'deployment'))
$deployBackupRootFull = [System.IO.Path]::GetFullPath(
    (Join-Path $deploymentRootFull 'deploy-backups'))
if (-not [string]::Equals(
        [System.IO.Path]::GetDirectoryName($distRootFull),
        $projectRootFull,
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetFileName($distRootFull),
        'dist',
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetDirectoryName($workRootFull),
        $distRootFull,
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetFileName($workRootFull),
        'work',
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetDirectoryName($deploymentRootFull),
        $workRootFull,
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetFileName($deploymentRootFull),
        'deployment',
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetDirectoryName($deployBackupRootFull),
        $deploymentRootFull,
        [System.StringComparison]::OrdinalIgnoreCase) `
    -or -not [string]::Equals(
        [System.IO.Path]::GetFileName($deployBackupRootFull),
        'deploy-backups',
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Deployment backup root escaped dist/work/deployment.'
}
$deploymentRoots = @(
    @{ Path = $distRootFull; Description = 'Project dist root' },
    @{ Path = $workRootFull; Description = 'Project work root' },
    @{ Path = $deploymentRootFull; Description = 'Project deployment work root' },
    @{ Path = $deployBackupRootFull; Description = 'Deployment backup root' }
)
foreach ($root in $deploymentRoots) {
    Assert-PlainDirectoryIfPresent -Path $root.Path `
        -Description $root.Description
    New-Item -ItemType Directory -Path $root.Path -Force | Out-Null
    Assert-PlainDirectoryIfPresent -Path $root.Path `
        -Description $root.Description
}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$backup = Join-Path $deployBackupRootFull "$stamp-native-only-deploy"
$targetExisted = Test-Path -LiteralPath $target -PathType Container
$backupInstalledTarget = Join-Path $backup 'installed-native'
$backupStableMarkers = Join-Path $backup 'stable-markers'
$installStage = Join-Path $backup 'install-stage'
$preservedVisibility = Join-Path $backup 'preserved\visibility.ini'
$preservedDiagnostics = Join-Path $backup 'preserved\diagnostics.ini'
if (Test-Path -LiteralPath $backup) {
    throw "Refusing to reuse an existing deployment backup: $backup"
}
New-Item -ItemType Directory -Path $backup | Out-Null
Copy-Item -LiteralPath $modsFile -Destination (Join-Path $backup 'mods.txt') -Force
if ($targetExisted) {
    New-Item -ItemType Directory -Path $backupInstalledTarget -Force | Out-Null
    Get-ChildItem -LiteralPath $target -Force | Copy-Item `
        -Destination $backupInstalledTarget -Recurse -Force
    $currentVisibility = Join-Path $target 'config\visibility.ini'
    if (Test-Path -LiteralPath $currentVisibility -PathType Leaf) {
        New-Item -ItemType Directory -Path `
            (Split-Path -Parent $preservedVisibility) -Force | Out-Null
        Copy-Item -LiteralPath $currentVisibility `
            -Destination $preservedVisibility -Force
    }
    $currentDiagnostics = Join-Path $target 'config\diagnostics.ini'
    if (Test-Path -LiteralPath $currentDiagnostics -PathType Leaf) {
        New-Item -ItemType Directory -Path `
            (Split-Path -Parent $preservedDiagnostics) -Force | Out-Null
        Copy-Item -LiteralPath $currentDiagnostics `
            -Destination $preservedDiagnostics -Force
    }
}
foreach ($markerName in @('enabled.txt', 'enabled.disabled.txt')) {
    $stableMarker = Join-Path $stableTarget $markerName
    if (Test-Path -LiteralPath $stableMarker -PathType Leaf) {
        New-Item -ItemType Directory -Path $backupStableMarkers -Force | Out-Null
        Copy-Item -LiteralPath $stableMarker `
            -Destination (Join-Path $backupStableMarkers $markerName) -Force
    }
}

. (Join-Path $PSScriptRoot 'ReleaseLayout.ps1')
New-Item -ItemType Directory -Path $installStage -Force | Out-Null
New-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $sourceDll `
    -DestinationRoot $installStage | Out-Null
Test-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $sourceDll `
    -PayloadRoot $installStage | Out-Null
$stagedDll = Join-Path $installStage 'dlls\main.dll'
$stagedReceipt = Join-Path $installStage `
    'metadata\native-build-receipt.json'
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
    -DllPath $stagedDll -ReceiptPath $stagedReceipt | Out-Null

Assert-InstalledDeploymentInputsSafe
$mutationStarted = $false
try {
$prohibited = @(
    'host', 'installer', 'scripts', 'src', 'tools', 'assets',
    'Install.cmd',
    'runtime\bridge', 'runtime\diagnostics', 'runtime\launch.request'
)
# This is the last fail-closed check before the short install transaction. Do
# not add a later process check after a mutation begins: that could force
# rollback writes while the game is already running.
Assert-DragonSwordStopped -Operation 'mutate the installed mods'
$mutationStarted = $true
$stableEnabledMarker = Join-Path $stableTarget 'enabled.txt'
if (Test-Path -LiteralPath $stableEnabledMarker -PathType Leaf) {
    $stableDisabledMarker = Join-Path $stableTarget 'enabled.disabled.txt'
    if (Test-Path -LiteralPath $stableDisabledMarker -PathType Leaf) {
        Move-Item -LiteralPath $stableDisabledMarker -Destination `
            (Join-Path $backup 'stable.enabled.disabled.txt') -Force
    }
    Move-Item -LiteralPath $stableEnabledMarker `
        -Destination $stableDisabledMarker -Force
}

# Replace the exact validated native target from a clean allowlisted stage.
# The full prior target and mods.txt were already backed up, so any failure can
# restore the complete previous installation rather than merging old files.
if (Test-Path -LiteralPath $target) {
    Remove-Item -LiteralPath $target -Recurse -Force
}
New-Item -ItemType Directory -Path $target -Force | Out-Null
Get-ChildItem -LiteralPath $installStage -Force | Copy-Item `
    -Destination $target -Recurse -Force
foreach ($installerOnlyDefault in @(
        'config\visibility.example.ini',
        'config\diagnostics.example.ini')) {
    $installedDefault = Join-Path $target $installerOnlyDefault
    if (Test-Path -LiteralPath $installedDefault -PathType Leaf) {
        Remove-Item -LiteralPath $installedDefault -Force
    }
}

$installedVisibilityConfig = Join-Path $target 'config\visibility.ini'
if (Test-Path -LiteralPath $preservedVisibility -PathType Leaf) {
    Copy-Item -LiteralPath $preservedVisibility `
        -Destination $installedVisibilityConfig -Force
} else {
    Copy-Item -LiteralPath $sourceVisibilityConfig `
        -Destination $installedVisibilityConfig -Force
}
$installedDiagnosticsConfig = Join-Path $target 'config\diagnostics.ini'
switch ($Diagnostics) {
    'Enable' {
        [System.IO.File]::WriteAllText(
            $installedDiagnosticsConfig,
            "[diagnostics]`n# Local gameplay-test override. Restart the game after changing this value.`ndebug_logging=true`n",
            [System.Text.UTF8Encoding]::new($false))
    }
    'Disable' {
        [System.IO.File]::WriteAllText(
            $installedDiagnosticsConfig,
            "[diagnostics]`n# Public-style diagnostics setting. This file is read once at Mod startup.`ndebug_logging=false`n",
            [System.Text.UTF8Encoding]::new($false))
    }
    default {
        if (Test-Path -LiteralPath $preservedDiagnostics -PathType Leaf) {
            Copy-Item -LiteralPath $preservedDiagnostics `
                -Destination $installedDiagnosticsConfig -Force
        } else {
            Copy-Item -LiteralPath $sourceDiagnosticsConfig `
                -Destination $installedDiagnosticsConfig -Force
        }
    }
}

$modsDocument = Read-ModsTextDocument -Path $modsFile
$lines = [System.Collections.Generic.List[string]]::new()
foreach ($line in @($modsDocument.Lines)) {
    $lines.Add([string]$line)
}
function Set-ModState([string]$Name, [int]$State) {
    $pattern = '^\s*' + [regex]::Escape($Name) + '\s*:'
    $matchingIndexes = [System.Collections.Generic.List[int]]::new()
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ($lines[$index] -match $pattern) {
            $matchingIndexes.Add($index)
        }
    }
    if ($matchingIndexes.Count -eq 0) {
        $lines.Add("$Name : $State")
        return
    }
    $lines[$matchingIndexes[0]] = "$Name : $State"
    for ($matchIndex = $matchingIndexes.Count - 1;
         $matchIndex -ge 1; --$matchIndex) {
        $lines.RemoveAt($matchingIndexes[$matchIndex])
    }
}
function Remove-ValidModEntries([string]$Name) {
    $pattern = '^\s*' + [regex]::Escape($Name) + '\s*:\s*[01]\s*$'
    for ($index = $lines.Count - 1; $index -ge 0; --$index) {
        if ($lines[$index] -match $pattern) {
            $lines.RemoveAt($index)
        }
    }
}
Remove-ValidModEntries $stableName
Set-ModState $nativeName 1
Write-ModsTextDocumentAtomically -Path $modsFile -Lines $lines `
    -Document $modsDocument
$installedModsDocument = Read-ModsTextDocument -Path $modsFile
$installedModLines = @($installedModsDocument.Lines)
$stableModLines = @($installedModLines | Where-Object {
        $_ -match ('^\s*' + [regex]::Escape($stableName) + '\s*:')
    })
$nativeModLines = @($installedModLines | Where-Object {
        $_ -match ('^\s*' + [regex]::Escape($nativeName) + '\s*:\s*1\s*$')
    })
$allRadarModPattern = '^\s*(?:' + [regex]::Escape($stableName) `
    + '|' + [regex]::Escape($nativeName) + ')\s*:'
$allRadarModLines = @($installedModLines | Where-Object {
        $_ -match $allRadarModPattern
    })
if ($stableModLines.Count -ne 0 -or $nativeModLines.Count -ne 1 `
    -or $allRadarModLines.Count -ne 1) {
    throw 'mods.txt does not contain exactly one enabled native entry and no predecessor entry.'
}

foreach ($name in $prohibited) {
    if (Test-Path -LiteralPath (Join-Path $target $name)) {
        throw "Native-only deployment contains prohibited content: $name"
    }
}
Test-DsnwrRuntimePayload -ProjectRoot $projectRoot -DllPath $sourceDll `
    -PayloadRoot $target -AllowUserVisibility -AllowUserDiagnostics `
    -InstalledConfiguration | Out-Null
$installedDll = Join-Path $target 'dlls\main.dll'
$installedReceipt = Join-Path $target `
    'metadata\native-build-receipt.json'
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
    -DllPath $installedDll -ReceiptPath $installedReceipt | Out-Null
Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
    -DllPath $sourceDll -ReceiptPath $sourceBuildReceipt | Out-Null
if (Test-Path -LiteralPath (Join-Path $stableTarget 'enabled.txt')) {
    throw 'The predecessor enabled.txt marker must not remain after its mods.txt authority is removed.'
}

$sourceHash = (Get-FileHash -LiteralPath $sourceDll -Algorithm SHA256).Hash
$installedHash = (Get-FileHash -LiteralPath (Join-Path $target 'dlls\main.dll') -Algorithm SHA256).Hash
if ($sourceHash -ne $installedHash) {
    throw 'Installed native DLL hash does not match the build output.'
}
$sourceSqlCipherHash = (Get-FileHash -LiteralPath $sourceSqlCipher -Algorithm SHA256).Hash
$installedSqlCipherHash = (Get-FileHash -LiteralPath (Join-Path $target 'vendor\sqlcipher\e_sqlcipher.dll') -Algorithm SHA256).Hash
if ($sourceSqlCipherHash -ne $installedSqlCipherHash) {
    throw 'Installed SQLCipher DLL hash does not match the source asset.'
}
$sourceOverrideHash = (Get-FileHash -LiteralPath $sourceOverrides -Algorithm SHA256).Hash
$installedOverrideHash = (Get-FileHash -LiteralPath (Join-Path $target 'data\defaults\treasure_overrides.txt') -Algorithm SHA256).Hash
if ($sourceOverrideHash -ne $installedOverrideHash) {
    throw 'Installed treasure override hash does not match the source default.'
}
$installedRenderCatalog = Join-Path $target 'data\generated\treasures.lua'
$sourceRenderCatalogHash = (Get-FileHash -LiteralPath $renderCatalogPath -Algorithm SHA256).Hash
$installedRenderCatalogHash = (Get-FileHash -LiteralPath $installedRenderCatalog -Algorithm SHA256).Hash
if ($sourceRenderCatalogHash -ne $installedRenderCatalogHash) {
    throw 'Installed section-aware render catalog hash does not match the source.'
}
$sourceAreaQuestCatalog = Join-Path $sourceData 'area-quests.tsv'
$installedAreaQuestCatalog = Join-Path $target 'data\generated\area-quests.tsv'
$sourceAreaQuestCatalogHash = (Get-FileHash -LiteralPath $sourceAreaQuestCatalog -Algorithm SHA256).Hash
$installedAreaQuestCatalogHash = (Get-FileHash -LiteralPath $installedAreaQuestCatalog -Algorithm SHA256).Hash
if ($sourceAreaQuestCatalogHash -ne $installedAreaQuestCatalogHash) {
    throw 'Installed area quest catalog hash does not match the source.'
}

[pscustomobject]@{
    Version = $expectedVersion
    InstalledDllSha256 = $installedHash
    InstalledSqlCipherSha256 = $installedSqlCipherHash
    InstalledRenderCatalogSha256 = $installedRenderCatalogHash
    InstalledAreaQuestCatalogSha256 = $installedAreaQuestCatalogHash
    PredecessorEntryPresent = $false
    NativeEnabled = 1
    Backup = $backup
}
} catch {
    $deploymentFailure = $_
    if (-not $mutationStarted) {
        throw $deploymentFailure
    }
    try {
        Copy-Item -LiteralPath (Join-Path $backup 'mods.txt') `
            -Destination $modsFile -Force
        if (Test-Path -LiteralPath $target) {
            Remove-Item -LiteralPath $target -Recurse -Force
        }
        if ($targetExisted) {
            New-Item -ItemType Directory -Path $target -Force | Out-Null
            Get-ChildItem -LiteralPath $backupInstalledTarget -Force | Copy-Item `
                -Destination $target -Recurse -Force
        }
        foreach ($markerName in @('enabled.txt', 'enabled.disabled.txt')) {
            $stableMarker = Join-Path $stableTarget $markerName
            if (Test-Path -LiteralPath $stableMarker) {
                Remove-Item -LiteralPath $stableMarker -Force
            }
            $backupMarker = Join-Path $backupStableMarkers $markerName
            if (Test-Path -LiteralPath $backupMarker -PathType Leaf) {
                New-Item -ItemType Directory -Path $stableTarget -Force | Out-Null
                Copy-Item -LiteralPath $backupMarker `
                    -Destination $stableMarker -Force
            }
        }
    } catch {
        throw "Deployment failed and automatic rollback also failed. Deployment=$($deploymentFailure.Exception.Message) Rollback=$($_.Exception.Message) Backup=$backup"
    }
    throw $deploymentFailure
}
