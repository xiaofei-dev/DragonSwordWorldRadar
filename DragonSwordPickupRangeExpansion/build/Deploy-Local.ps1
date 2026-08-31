[CmdletBinding()]
param(
    [string]$ArtifactPath,
    [string]$PaksRoot = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Content\Paks',
    [Parameter(Mandatory)][string]$RepakPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $ArtifactPath) {
    $ArtifactPath = Join-Path $projectRoot 'dist\DS_PickupRangeX5_P.pak'
}
if (Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue) {
    throw 'Refusing to deploy while DragonSword is running.'
}

$artifact = (Resolve-Path -LiteralPath $ArtifactPath).Path
$repak = (Resolve-Path -LiteralPath $RepakPath).Path
$paksRootFull = [IO.Path]::GetFullPath($PaksRoot)
$modsRoot = [IO.Path]::GetFullPath((Join-Path $paksRootFull '~mods'))
$expectedModsRoot = $paksRootFull.TrimEnd('\') + '\~mods'
if (-not $modsRoot.Equals($expectedModsRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Resolved ~mods target is outside the configured Paks root.'
}
$artifactManifest = Get-Content -LiteralPath (Join-Path $projectRoot 'dist\DS_PickupRangeX5_P.build.json') -Raw |
    ConvertFrom-Json
$targetManifest = Get-Content -LiteralPath (Join-Path $projectRoot 'metadata\targets-x5.json') -Raw |
    ConvertFrom-Json
$dropTargetManifest = Get-Content -LiteralPath (Join-Path $projectRoot 'metadata\drop-targets-x5.json') -Raw |
    ConvertFrom-Json
$artifactHash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash
if ($artifactHash -ne [string]$artifactManifest.artifact_sha256 -or
    [int]$artifactManifest.target_count -ne 69 -or
    [int]$artifactManifest.drop_item_target_count -ne 19 -or
    [string]$artifactManifest.static_validation -ne 'PASSED') {
    throw 'The deployment artifact does not match the statically verified x5 build manifest.'
}

$expectedEntries = @($targetManifest.targets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
}) + @($dropTargetManifest.targets | ForEach-Object {
    'DS/Content/' + ([string]$_.path) + '.uasset'
    'DS/Content/' + ([string]$_.path) + '.uexp'
})
$expectedEntries = @($expectedEntries | Sort-Object)
$artifactEntries = @(& $repak list $artifact | ForEach-Object { $_.Trim().Replace('\', '/') } | Sort-Object)
if ($LASTEXITCODE -ne 0 -or
    (Compare-Object -ReferenceObject $expectedEntries -DifferenceObject $artifactEntries)) {
    throw 'The deployment artifact entry set does not match the reviewed 69-target manifests.'
}

New-Item -ItemType Directory -Path $modsRoot -Force | Out-Null
$x3Path = Join-Path $modsRoot 'DS_PickupRangeX3Canary_P.pak'
$installedPath = Join-Path $modsRoot 'DS_PickupRangeX5_P.pak'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$rollbackRoot = Join-Path $projectRoot ("runtime\rollback\pickup-range-" + $stamp)

$allowedConflicts = @(
    [IO.Path]::GetFullPath($x3Path),
    [IO.Path]::GetFullPath($installedPath)
)
$conflicts = [Collections.Generic.List[string]]::new()
foreach ($pak in @(Get-ChildItem -LiteralPath $modsRoot -Filter '*.pak' -File -ErrorAction SilentlyContinue)) {
    if ($allowedConflicts -contains $pak.FullName) { continue }
    $entries = @(& $repak list $pak.FullName 2>$null | ForEach-Object { $_.Trim().Replace('\', '/') })
    if ($LASTEXITCODE -ne 0) {
        throw "Could not inspect installed mod PAK for conflicts: $($pak.FullName)"
    }
    if (@(Compare-Object -ReferenceObject $expectedEntries -DifferenceObject $entries -IncludeEqual -ExcludeDifferent |
        Where-Object SideIndicator -eq '==').Count -gt 0) {
        $conflicts.Add($pak.FullName)
    }
}
if ($conflicts.Count -gt 0) {
    throw "Other installed PAKs override x5 targets: $($conflicts -join '; ')"
}

foreach ($oldPath in @($x3Path, $installedPath)) {
    if (Test-Path -LiteralPath $oldPath -PathType Leaf) {
        New-Item -ItemType Directory -Path $rollbackRoot -Force | Out-Null
        Copy-Item -LiteralPath $oldPath -Destination (Join-Path $rollbackRoot ([IO.Path]::GetFileName($oldPath))) -Force
        Remove-Item -LiteralPath $oldPath -Force
    }
}
Copy-Item -LiteralPath $artifact -Destination $installedPath -Force

$installedHash = (Get-FileHash -LiteralPath $installedPath -Algorithm SHA256).Hash
if ($installedHash -ne $artifactHash) {
    throw 'Installed x5 PAK hash differs from the verified artifact.'
}
$installedEntries = @(& $repak list $installedPath | ForEach-Object { $_.Trim().Replace('\', '/') } | Sort-Object)
if ($LASTEXITCODE -ne 0 -or
    (Compare-Object -ReferenceObject $expectedEntries -DifferenceObject $installedEntries)) {
    throw 'Installed x5 PAK entry set differs from the reviewed manifest.'
}
if (Test-Path -LiteralPath $x3Path) {
    throw 'The obsolete x3 canary remains installed.'
}

$record = [ordered]@{
    schema_version = 1
    deployed_at_utc = [DateTime]::UtcNow.ToString('O')
    installed_path = $installedPath
    installed_sha256 = $installedHash
    target_count = 69
    authored_target_count = 50
    drop_item_target_count = 19
    pak_entry_count = $installedEntries.Count
    x3_removed = $true
    rollback_path = $rollbackRoot
    static_validation = 'PASSED'
    gameplay_acceptance = 'NOT_VALIDATED'
}
$recordRoot = Join-Path $projectRoot 'runtime\deployments'
New-Item -ItemType Directory -Path $recordRoot -Force | Out-Null
$recordPath = Join-Path $recordRoot 'pickup-range-x5-latest.json'
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText($recordPath, ($record | ConvertTo-Json -Depth 5), $utf8)

[pscustomobject]$record
