[CmdletBinding()]
param(
    [string]$CatalogPath,
    [string]$ActorPositionXmlPath,
    [string]$MiniGameXmlPath,
    [switch]$VerifyOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CatalogPath)) {
    $CatalogPath = Join-Path $projectRoot 'src\data\generated\moles.lua'
}
if ([string]::IsNullOrWhiteSpace($ActorPositionXmlPath)) {
    $ActorPositionXmlPath = Join-Path $projectRoot `
        '..\DragonSwordWorldDataProbe\reference\assault-support\xml\017_ActorPositionData.xml'
}
if ([string]::IsNullOrWhiteSpace($MiniGameXmlPath)) {
    $MiniGameXmlPath = Join-Path $projectRoot `
        '..\DragonSwordWorldDataProbe\reference\assault-support\xml\001_MiniGameData.xml'
}

$catalog = (Resolve-Path -LiteralPath $CatalogPath).ProviderPath
$actorXml = (Resolve-Path -LiteralPath $ActorPositionXmlPath).ProviderPath
$miniGameXml = (Resolve-Path -LiteralPath $MiniGameXmlPath).ProviderPath

$expectedActorXmlSha256 =
    '11CA916050AFA25F856DF0AFF46CAE0D23F928E8490617FBD3B524DC183E3ACF'
$expectedMiniGameXmlSha256 =
    'EDA335750F30C31D1A3E003C96B54024F671F600E18FCC02759970F7236CE9E3'
$actualActorXmlSha256 =
    (Get-FileHash -LiteralPath $actorXml -Algorithm SHA256).Hash
$actualMiniGameXmlSha256 =
    (Get-FileHash -LiteralPath $miniGameXml -Algorithm SHA256).Hash
if ($actualActorXmlSha256 -ne $expectedActorXmlSha256) {
    throw "ActorPositionData source hash changed: $actualActorXmlSha256"
}
if ($actualMiniGameXmlSha256 -ne $expectedMiniGameXmlSha256) {
    throw "MiniGameData source hash changed: $actualMiniGameXmlSha256"
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

function Get-RequiredCapture {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Description
    )
    $matches = @([regex]::Matches($Text, $Pattern))
    if ($matches.Count -ne 1) {
        throw "$Description must occur exactly once."
    }
    return $matches[0].Groups['value'].Value
}

function Assert-ExactIdSet {
    param(
        [object[]]$Expected,
        [object[]]$Actual,
        [string]$Description
    )
    $difference = @(Compare-Object `
        -ReferenceObject @($Expected | Sort-Object) `
        -DifferenceObject @($Actual | Sort-Object))
    if ($difference.Count -ne 0) {
        throw "$Description ID set changed."
    }
}

$miniGameRows = @(Get-Content -LiteralPath $miniGameXml | Where-Object {
        $_ -match '<ns1:MiniGameData\s'
    })
if ($miniGameRows.Count -ne 176) {
    throw "MiniGameData row count changed: $($miniGameRows.Count)"
}
$expectedFlyIds = @(@(11001L..11023L) + @(11025L..11034L))
$expectedMoleIds = @(12001L..12040L)
$expectedWaveIds = @(13001L..13010L)
$expectedMiniGameIds = @(
    $expectedFlyIds + $expectedMoleIds + $expectedWaveIds)
$miniGameIds = [Collections.Generic.List[int64]]::new()
foreach ($row in $miniGameRows) {
    $idText = Get-RequiredCapture $row `
        'ns1:ID="(?<value>[0-9]+)"' 'MiniGameData ID'
    $dataLayer = Get-RequiredCapture $row `
        'ns1:DataLayer="(?<value>[^"]*)"' 'MiniGameData DataLayer'
    if ($dataLayer -notmatch `
            '^MiniGame_(?<kind>Fly|Mole|Wave)_(?<id>[0-9]+)$') {
        continue
    }
    $id = Parse-Int64Exact $idText 'MiniGameData mini-game ID'
    $layerId = Parse-Int64Exact `
        $Matches['id'] 'MiniGameData mini-game layer ID'
    if ($id -ne $layerId) {
        throw "MiniGameData ID/DataLayer mismatch: $id / $dataLayer"
    }
    if ($expectedMiniGameIds -contains $id) {
        $miniGameIds.Add($id)
    }
}
if ($miniGameIds.Count -ne 83 -or
    @($miniGameIds | Sort-Object -Unique).Count -ne 83) {
    throw 'MiniGameData must contain exactly 83 unique supported mini-game rows.'
}
Assert-ExactIdSet $expectedMiniGameIds $miniGameIds.ToArray() `
    'MiniGameData supported mini-game'

[xml]$actorDocument = Get-Content -LiteralPath $actorXml -Raw
$actorNamespace = 'http://leigient.549n.com/schema/ActorPositionInfo'
$actorNodes = @(
    $actorDocument.SelectNodes('//*[local-name()="ActorPositionData"]'))
if ($actorNodes.Count -ne 7536) {
    throw "ActorPositionData row count changed: $($actorNodes.Count)"
}
$miniGameActors = @{}
$miniGameActorIds = [Collections.Generic.List[int64]]::new()
foreach ($node in $actorNodes) {
    $name = $node.GetAttribute('Name', $actorNamespace)
    if ($name -notmatch `
            '^MiniGame_(?<kind>Fly|Mole|Wave)_(?<id>[0-9]+)_NPC_Start$') {
        continue
    }
    $id = Parse-Int64Exact $Matches['id'] 'Mini-game NPC_Start ID'
    if ($expectedMiniGameIds -notcontains $id) {
        continue
    }
    $mapId = Parse-Int64Exact `
        ($node.GetAttribute('MapGroupID', $actorNamespace)) `
        "Mini-game $id map ID"
    if ($mapId -ne 100) {
        throw "Mini-game $id NPC_Start must be in map 100, not $mapId."
    }
    if ($miniGameActors.ContainsKey($id)) {
        throw "Mini-game $id has more than one NPC_Start row."
    }
    $miniGameActors[$id] = [pscustomobject]@{
        Id = $id
        MapId = $mapId
        Name = $name
        X = Parse-DoubleExact `
            ($node.GetAttribute('PosX', $actorNamespace)) "Mini-game $id X"
        Y = Parse-DoubleExact `
            ($node.GetAttribute('PosY', $actorNamespace)) "Mini-game $id Y"
        Z = Parse-DoubleExact `
            ($node.GetAttribute('PosZ', $actorNamespace)) "Mini-game $id Z"
    }
    $miniGameActorIds.Add($id)
}
if ($miniGameActors.Count -ne 83) {
    throw "ActorPositionData must contain exactly 83 supported mini-game NPC_Start rows, not $($miniGameActors.Count)."
}
Assert-ExactIdSet $expectedMiniGameIds $miniGameActorIds.ToArray() `
    'ActorPositionData supported mini-game NPC_Start'

$catalogLines = @(Get-Content -LiteralPath $catalog)
$entryLines = @($catalogLines | Where-Object {
        $_ -match '^\s*\{\s*mini_game_id\s*='
    })
if ($entryLines.Count -ne 83) {
    throw "The mini-game catalog must contain exactly 83 rows, not $($entryLines.Count)."
}

$catalogIds = [Collections.Generic.List[int64]]::new()
$catalogMiniGameIds = [Collections.Generic.List[int64]]::new()
$normalizedLines = [Collections.Generic.List[string]]::new()
foreach ($line in $catalogLines) {
    if ($line -notmatch '^\s*\{\s*mini_game_id\s*=') {
        $normalizedLines.Add($line)
        continue
    }

    $idText = Get-RequiredCapture $line `
        '(?:^|[,\s])mini_game_id\s*=\s*(?<value>[0-9]+)(?=\s*,)' `
        'mini_game_id'
    $id = Parse-Int64Exact $idText 'mini_game_id'
    $catalogIds.Add($id)
    $catalogMiniGameIds.Add($id)
    if (-not $miniGameActors.ContainsKey($id)) {
        throw "Mini-game $id is absent from the pinned ActorPositionData source."
    }
    $actor = $miniGameActors[$id]
    $mapIdText = Get-RequiredCapture $line `
        '(?:^|[,\s])map_id\s*=\s*(?<value>-?[0-9]+)(?=\s*,)' `
        "Mini-game $id map_id"
    $xText = Get-RequiredCapture $line `
        '(?:^|[,\s])x\s*=\s*(?<value>[-+0-9.eE]+)(?=\s*,)' `
        "Mini-game $id x"
    $yText = Get-RequiredCapture $line `
        '(?:^|[,\s])y\s*=\s*(?<value>[-+0-9.eE]+)(?=\s*,)' `
        "Mini-game $id y"
    $zText = Get-RequiredCapture $line `
        '(?:^|[,\s])z\s*=\s*(?<value>[-+0-9.eE]+)(?=\s*,)' `
        "Mini-game $id z"
    $uidName = Get-RequiredCapture $line `
        '(?:^|[,\s])uid_name\s*=\s*"(?<value>[^"]+)"(?=\s*,)' `
        "Mini-game $id uid_name"
    $positionRole = Get-RequiredCapture $line `
        '(?:^|[,\s])position_role\s*=\s*"(?<value>[^"]+)"(?=\s*,)' `
        "Mini-game $id position_role"

    if ((Parse-Int64Exact $mapIdText "Mini-game $id map_id") -ne 100 -or
        (Parse-DoubleExact $xText "Mini-game $id x") -ne $actor.X -or
        (Parse-DoubleExact $yText "Mini-game $id y") -ne $actor.Y -or
        (Parse-DoubleExact $zText "Mini-game $id z") -ne $actor.Z -or
        $uidName -cne $actor.Name -or
        $positionRole -cne 'NPC_Start' -or
        $line -notmatch '(?:^|[,\s])has_z\s*=\s*true(?=\s*,)') {
        throw "Mini-game $id does not exactly match its pinned map-100 NPC_Start row."
    }

    $normalized = [regex]::Replace(
        $line,
        ',\s*height_z\s*=\s*[-+0-9.eE]+',
        '')
    $normalized = [regex]::Replace(
        $normalized,
        ',\s*height_trusted\s*=\s*(?:true|false)',
        '')
    $hasZMatches = @([regex]::Matches(
            $normalized,
            '(?<field>has_z\s*=\s*true)'))
    if ($hasZMatches.Count -ne 1) {
        throw "Mini-game $id must contain exactly one has_z=true field."
    }
    $heightText = $actor.Z.ToString('0.################', $culture)
    $normalized = [regex]::Replace(
        $normalized,
        'has_z\s*=\s*true',
        "has_z = true, height_z = $heightText, height_trusted = true")
    $normalizedLines.Add($normalized)
}

if (@($catalogIds | Sort-Object -Unique).Count -ne 83) {
    throw 'The mini-game catalog IDs must be unique.'
}
Assert-ExactIdSet $expectedMiniGameIds $catalogMiniGameIds.ToArray() `
    'Generated supported mini-game catalog'
$flyCount = @($catalogIds | Where-Object { $_ -ge 11001 -and $_ -le 11034 }).Count
$moleCount = @($catalogIds | Where-Object { $_ -ge 12001 -and $_ -le 12040 }).Count
$waveCount = @($catalogIds | Where-Object { $_ -ge 13001 -and $_ -le 13010 }).Count
if ($flyCount -ne 33 -or $moleCount -ne 40 -or $waveCount -ne 10) {
    throw "Mini-game shape changed: Fly=$flyCount Mole=$moleCount Wave=$waveCount"
}

$expectedText = ($normalizedLines -join "`n").TrimEnd("`n")
$currentText = (Get-Content -LiteralPath $catalog -Raw) -replace "`r`n", "`n"
$currentText = $currentText.TrimEnd("`n")
if ($VerifyOnly) {
    if ($currentText -cne $expectedText) {
        throw 'The checked-in mini-game height fields are not reproducible from the pinned sources.'
    }
} else {
    [IO.File]::WriteAllText(
        $catalog,
        (($normalizedLines -join "`r`n") + "`r`n"),
        [Text.UTF8Encoding]::new($false))
}

[pscustomobject]@{
    catalog = $catalog
    actor_position_xml = $actorXml
    actor_position_xml_sha256 = $actualActorXmlSha256
    mini_game_xml = $miniGameXml
    mini_game_xml_sha256 = $actualMiniGameXmlSha256
    rows = $entryLines.Count
    fly_rows = $flyCount
    mole_rows = $moleCount
    wave_rows = $waveCount
    trusted_mini_game_height_rows = $entryLines.Count
    trusted_fly_height_rows = $flyCount
    trusted_mole_height_rows = $moleCount
    trusted_wave_height_rows = $waveCount
    mode = if ($VerifyOnly) { 'VERIFIED' } else { 'WRITTEN' }
}
