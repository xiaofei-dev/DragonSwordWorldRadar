[CmdletBinding()]
param(
    [string]$CatalogPath,
    [string]$ActorPositionXmlPath,
    [switch]$VerifyOnly,
    [switch]$EmitText
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CatalogPath)) {
    $CatalogPath = Join-Path $projectRoot 'src\data\generated\area-quests.tsv'
}
if ([string]::IsNullOrWhiteSpace($ActorPositionXmlPath)) {
    $ActorPositionXmlPath = Join-Path $projectRoot `
        '..\DragonSwordWorldDataProbe\reference\assault-support\xml\017_ActorPositionData.xml'
}

$catalog = (Resolve-Path -LiteralPath $CatalogPath).ProviderPath
$actorXml = (Resolve-Path -LiteralPath $ActorPositionXmlPath).ProviderPath
$expectedActorXmlSha256 =
    '11CA916050AFA25F856DF0AFF46CAE0D23F928E8490617FBD3B524DC183E3ACF'
$actualActorXmlSha256 = (Get-FileHash -LiteralPath $actorXml -Algorithm SHA256).Hash
if ($actualActorXmlSha256 -ne $expectedActorXmlSha256) {
    throw "ActorPositionData source hash changed: $actualActorXmlSha256"
}

$culture = [Globalization.CultureInfo]::InvariantCulture
function Parse-Int64Exact {
    param([string]$Value, [string]$Description)
    [int64]$parsed = 0
    if (-not [int64]::TryParse(
            $Value,
            [Globalization.NumberStyles]::Integer,
            $culture,
            [ref]$parsed)) {
        throw "$Description is not an Int64: $Value"
    }
    return $parsed
}

function Parse-DoubleExact {
    param([string]$Value, [string]$Description)
    [double]$parsed = 0.0
    if (-not [double]::TryParse(
            $Value,
            [Globalization.NumberStyles]::Float,
            $culture,
            [ref]$parsed) -or
        [double]::IsNaN($parsed) -or [double]::IsInfinity($parsed)) {
        throw "$Description is not a finite number: $Value"
    }
    return $parsed
}

$catalogLines = @(Get-Content -LiteralPath $catalog)
$legacyHeader = "Id`tX`tY`tZ`tHeightZ`tHeightTrusted"
$heightBandHeader =
    "Id`tX`tY`tZ`tHeight1MinZ`tHeight1MaxZ`tHeight2MinZ`tHeight2MaxZ`tHeightBandCount"
if ($catalogLines.Count -ne 148 -or
    ($catalogLines[0] -ne $legacyHeader -and
     $catalogLines[0] -ne $heightBandHeader)) {
    throw 'The Area Quest catalog must contain the exact 147-row height schema.'
}
$baseRows = [Collections.Generic.List[object]]::new()
$uniqueIds = [Collections.Generic.HashSet[int64]]::new()
foreach ($line in $catalogLines | Select-Object -Skip 1) {
    $fields = @($line -split "`t", -1)
    if ($fields.Count -ne 6 -and $fields.Count -ne 9) {
        throw "Malformed Area Quest row: $line"
    }
    $id = Parse-Int64Exact $fields[0] 'Area Quest ID'
    if ($id -le 0 -or -not $uniqueIds.Add($id)) {
        throw "Invalid or duplicate Area Quest ID: $id"
    }
    $baseRows.Add([pscustomobject]@{
        Id = $id
        IdText = $fields[0]
        X = Parse-DoubleExact $fields[1] "Area Quest $id X"
        XText = $fields[1]
        Y = Parse-DoubleExact $fields[2] "Area Quest $id Y"
        YText = $fields[2]
        ZText = $fields[3]
    })
}

[xml]$document = Get-Content -LiteralPath $actorXml -Raw
$namespaceUri = 'http://leigient.549n.com/schema/ActorPositionInfo'
$actorRows = [Collections.Generic.List[object]]::new()
foreach ($node in $document.SelectNodes('//*[local-name()="ActorPositionData"]')) {
    $mapGroup = $node.GetAttribute('MapGroupID', $namespaceUri)
    if ($mapGroup -ne '100') {
        continue
    }
    $name = $node.GetAttribute('Name', $namespaceUri)
    $actorRows.Add([pscustomobject]@{
        Name = $name
        X = Parse-DoubleExact ($node.GetAttribute('PosX', $namespaceUri)) `
            "Actor $name X"
        Y = Parse-DoubleExact ($node.GetAttribute('PosY', $namespaceUri)) `
            "Actor $name Y"
        Z = Parse-DoubleExact ($node.GetAttribute('PosZ', $namespaceUri)) `
            "Actor $name Z"
    })
}

$maximumPlanarDistanceSquared = 1000.0 * 1000.0
$maximumHeightBandGap = 250.0
$profileCount = 0
$multiBandCount = 0
$missingIds = [Collections.Generic.List[int64]]::new()
$multiBandIds = [Collections.Generic.List[int64]]::new()
$output = [Collections.Generic.List[string]]::new()
$output.Add($heightBandHeader)
foreach ($row in $baseRows) {
    $idPattern = '(?<![0-9])' + [regex]::Escape($row.IdText) + '(?![0-9])'
    $candidates = @($actorRows | ForEach-Object {
        if ($_.Name -notmatch $idPattern) {
            return
        }
        $dx = $_.X - $row.X
        $dy = $_.Y - $row.Y
        $distanceSquared = $dx * $dx + $dy * $dy
        if ($distanceSquared -le $maximumPlanarDistanceSquared) {
            [pscustomobject]@{
                Name = $_.Name
                X = $_.X
                Y = $_.Y
                Z = $_.Z
                DistanceSquared = $distanceSquared
            }
        }
    } | Sort-Object DistanceSquared, Name, X, Y, Z)

    $bands = [Collections.Generic.List[object]]::new()
    foreach ($candidate in ($candidates | Sort-Object Z, DistanceSquared, Name, X, Y)) {
        $isTaskActor = $candidate.Name -notmatch '(?i)move[_ ]?check'
        if ($bands.Count -eq 0 -or
            (([double]$candidate.Z -
              [double]$bands[$bands.Count - 1].MaximumZ) -gt
             $maximumHeightBandGap)) {
            $bands.Add([pscustomobject]@{
                MinimumZ = [double]$candidate.Z
                MaximumZ = [double]$candidate.Z
                HasTaskActor = $isTaskActor
            })
        } else {
            $bands[$bands.Count - 1].MaximumZ = [double]$candidate.Z
            $bands[$bands.Count - 1].HasTaskActor =
                $bands[$bands.Count - 1].HasTaskActor -or $isTaskActor
        }
    }
    # A Move_Check-only band is an activation/traversal trigger rather than
    # the physical task target. Drop such a band when any co-located band has
    # an actual task actor, but retain it as a bounded fallback when it is the
    # only exact-ID evidence available for the task.
    $taskActorBands = @($bands | Where-Object { $_.HasTaskActor })
    if ($taskActorBands.Count -gt 0) {
        $filteredBands = [Collections.Generic.List[object]]::new()
        foreach ($band in $taskActorBands) {
            $filteredBands.Add($band)
        }
        $bands = $filteredBands
    }
    if ($bands.Count -gt 2) {
        throw "Area Quest $($row.Id) exceeds the fixed two-band height capacity."
    }
    if ($bands.Count -gt 0) {
        ++$profileCount
    } else {
        $missingIds.Add($row.Id)
    }
    if ($bands.Count -eq 2) {
        ++$multiBandCount
        $multiBandIds.Add($row.Id)
    }
    $heightValues = [double[]](0.0, 0.0, 0.0, 0.0)
    for ($index = 0; $index -lt $bands.Count; ++$index) {
        $heightValues[$index * 2] = [double]$bands[$index].MinimumZ
        $heightValues[$index * 2 + 1] = [double]$bands[$index].MaximumZ
    }
    $heightTexts = @($heightValues | ForEach-Object {
        $_.ToString('0.################', $culture)
    })
    $output.Add(
        "$($row.IdText)`t$($row.XText)`t$($row.YText)`t$($row.ZText)" +
        "`t$($heightTexts[0])`t$($heightTexts[1])" +
        "`t$($heightTexts[2])`t$($heightTexts[3])`t$($bands.Count)")
}

$expectedMissing = @(1103108L, 1104104L, 1104203L)
$expectedMultiBand = @(1103061L)
if ($profileCount -ne 144 -or $multiBandCount -ne 1 -or
    ($missingIds.ToArray() -join ',') -ne ($expectedMissing -join ',') -or
    ($multiBandIds.ToArray() -join ',') -ne ($expectedMultiBand -join ',')) {
    throw "Area Quest height-band boundary changed: profiles=$profileCount multi=$multiBandCount missing=$($missingIds -join ',') multi_ids=$($multiBandIds -join ',')"
}

$expectedText = $output -join "`n"
$currentText = (Get-Content -LiteralPath $catalog -Raw) -replace "`r`n", "`n"
$currentText = $currentText.TrimEnd("`n")
if ($EmitText) {
    [Console]::Out.Write($expectedText)
} elseif ($VerifyOnly) {
    if ($currentText -cne $expectedText) {
        throw 'The checked-in Area Quest height catalog is not reproducible from the pinned source.'
    }
} else {
    [IO.File]::WriteAllText(
        $catalog,
        (($output -join "`r`n") + "`r`n"),
        [Text.UTF8Encoding]::new($false))
}

if (-not $EmitText) {
    [pscustomobject]@{
        catalog = $catalog
        actor_position_xml = $actorXml
        actor_position_xml_sha256 = $actualActorXmlSha256
        rows = $baseRows.Count
        height_profile_rows = $profileCount
        multi_band_rows = $multiBandCount
        missing_height_rows = $missingIds.Count
        mode = if ($VerifyOnly) { 'VERIFIED' } else { 'WRITTEN' }
    }
}
