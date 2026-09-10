[CmdletBinding()]
param(
    [string]$CatalogPath,
    [string]$ActorPositionXmlPath,
    [string]$OutputPath,
    [string]$MetadataPath,
    [switch]$VerifyOnly,
    [switch]$SelfTestOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$projectRoot = Split-Path -Parent $PSScriptRoot
$culture = [Globalization.CultureInfo]::InvariantCulture
$expectedActorXmlSha256 =
    '11CA916050AFA25F856DF0AFF46CAE0D23F928E8490617FBD3B524DC183E3ACF'

function Read-FiniteNumber {
    param([string]$Value)
    [double]$number = 0
    if (-not [double]::TryParse($Value,
            [Globalization.NumberStyles]::Float, $culture, [ref]$number) -or
        [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
        throw 'A scene-anchor coordinate is not finite.'
    }
    return $number
}

function Get-SceneSourcePriority {
    param([string]$Name)
    # A traversal/position proxy is not a physical task anchor. Explicit NPC
    # spawners remain eligible even when the spawn label ends in Empty.
    if ($Name -match '(?i)move[_ ]?check|StepPosition|StartPosition|PickUpPosition') {
        return -1
    }
    if ($Name -match '(?i)AcceptActor') { return 0 }
    if ($Name -match '(?i)NpcSpawn|(?:^|_)NPC(?:_|$)|dynamicquest_npc') {
        # Named/reading NPC spawns carry more specific target evidence than an
        # Empty spawn proxy at the same planar location (for example 1110083).
        if ($Name -match '(?i)(?:^|_)Empty[0-9]*$') { return 2 }
        return 1
    }
    if ($Name -match '(?i)Interact|Altar|Backpack') { return 1 }
    if ($Name -match '(?i)(?:^|_)Empty[0-9]*$|(?:^|_)Move(?:_|$)|(?:^|_)Start$') {
        return -1
    }
    if ($Name -match '(?i)StartActor') { return 2 }
    return 3
}

function Select-SceneSource {
    param([double]$MarkerZ, [object[]]$Bands, [object[]]$Candidates)
    if ([double]::IsNaN($MarkerZ) -or [double]::IsInfinity($MarkerZ) -or
        $Bands.Count -eq 0 -or $Bands.Count -gt 2) {
        return [pscustomobject]@{ Status = 'missing_height_profile'; Source = $null; Band = -1 }
    }
    $selectedBand = -1
    $nearestBandDistance = [double]::PositiveInfinity
    $bandTie = $false
    for ($index = 0; $index -lt $Bands.Count; ++$index) {
        $band = $Bands[$index]
        if ([double]::IsNaN($band.MinimumZ) -or [double]::IsInfinity($band.MinimumZ) -or
            [double]::IsNaN($band.MaximumZ) -or [double]::IsInfinity($band.MaximumZ) -or
            $band.MinimumZ -gt $band.MaximumZ -or
            ($index -gt 0 -and $Bands[$index - 1].MaximumZ -ge $band.MinimumZ)) {
            return [pscustomobject]@{ Status = 'invalid_height_profile'; Source = $null; Band = -1 }
        }
        $distance = if ($MarkerZ -lt $band.MinimumZ) { $band.MinimumZ - $MarkerZ }
            elseif ($MarkerZ -gt $band.MaximumZ) { $MarkerZ - $band.MaximumZ }
            else { 0.0 }
        if ($distance -lt $nearestBandDistance) {
            $selectedBand = $index
            $nearestBandDistance = $distance
            $bandTie = $false
        } elseif ($distance -eq $nearestBandDistance) { $bandTie = $true }
    }
    if ($bandTie) {
        return [pscustomobject]@{ Status = 'ambiguous_height_band'; Source = $null; Band = -1 }
    }
    $selected = $Bands[$selectedBand]
    $eligible = @($Candidates | Where-Object {
        $_.Priority -ge 0 -and $_.DistanceSquared -le 1000000.0 -and
        $_.Z -ge $selected.MinimumZ -and $_.Z -le $selected.MaximumZ -and
        -not [double]::IsNaN($_.X) -and -not [double]::IsInfinity($_.X) -and
        -not [double]::IsNaN($_.Y) -and -not [double]::IsInfinity($_.Y) -and
        -not [double]::IsNaN($_.Z) -and -not [double]::IsInfinity($_.Z)
    } | Sort-Object Priority, DistanceSquared, ActorId)
    if ($eligible.Count -eq 0) {
        return [pscustomobject]@{ Status = 'no_physical_source_in_selected_band'; Source = $null; Band = $selectedBand }
    }
    $best = $eligible[0]
    $ties = @($eligible | Where-Object {
        $_.Priority -eq $best.Priority -and $_.DistanceSquared -eq $best.DistanceSquared
    })
    foreach ($candidate in $ties) {
        # Multiple named actors at exactly the same full XYZ prove one anchor.
        # Equal-distance actors at distinct positions do not justify a choice.
        if ($candidate.X -ne $best.X -or $candidate.Y -ne $best.Y -or $candidate.Z -ne $best.Z) {
            return [pscustomobject]@{ Status = 'ambiguous_equally_ranked_sources'; Source = $null; Band = $selectedBand }
        }
    }
    return [pscustomobject]@{ Status = 'source_verified'; Source = $best; Band = $selectedBand }
}

function Select-ReviewedSceneSource {
    param([int64]$TaskId, [object[]]$Candidates)
    if ($TaskId -ne 1110038) { return $null }
    # This task's legacy band comes from an Empty proxy. The pinned XML has
    # exactly one named task entity, at 10.79 m from the raw marker. Keep its
    # entire authored XYZ; do not force its Z back into the proxy-derived band.
    $verified = @($Candidates | Where-Object {
        $_.ActorId -ceq '560707418635547564' -and $_.Priority -ge 0 -and
        $_.DistanceSquared -le 1210000.0 -and $_.DistanceSquared -ge 0.0 -and
        -not [double]::IsNaN($_.X) -and -not [double]::IsInfinity($_.X) -and
        -not [double]::IsNaN($_.Y) -and -not [double]::IsInfinity($_.Y) -and
        -not [double]::IsNaN($_.Z) -and -not [double]::IsInfinity($_.Z)
    })
    if ($verified.Count -ne 1) {
        throw 'The reviewed task 1110038 entity must retain its exact ActorID and 11 m planar bound.'
    }
    return [pscustomobject]@{
        Status = 'source_verified'; Source = $verified[0]; Band = -1
    }
}

function Test-SceneSourceSelection {
    function Assert-Selection { param([bool]$Condition) if (-not $Condition) { throw 'Scene source selection self-test failed.' } }
    $low = [pscustomobject]@{ MinimumZ = -687.0; MaximumZ = -687.0 }
    $high = [pscustomobject]@{ MinimumZ = 2000.0; MaximumZ = 2000.0 }
    $accept = [pscustomobject]@{ ActorId = '1'; Priority = 0; X = 50.0; Y = 60.0; Z = -687.0; DistanceSquared = 6100.0 }
    $npc = [pscustomobject]@{ ActorId = '2'; Priority = 1; X = 10.0; Y = 20.0; Z = -687.0; DistanceSquared = 500.0 }
    $move = [pscustomobject]@{ ActorId = '3'; Priority = -1; X = 0.0; Y = 0.0; Z = -687.0; DistanceSquared = 0.0 }
    $choice = Select-SceneSource 19502.0 @($low) @($npc, $move, $accept)
    Assert-Selection ($choice.Source.ActorId -eq '1' -and $choice.Source.X -eq 50.0 -and $choice.Source.Y -eq 60.0 -and $choice.Source.Z -eq -687.0)
    Assert-Selection ((Select-SceneSource 656.5 @($low, $high) @($accept)).Status -eq 'ambiguous_height_band')
    Assert-Selection ((Select-SceneSource 2000.0 @($low, $high) @($accept)).Status -eq 'no_physical_source_in_selected_band')
    Assert-Selection ((Select-SceneSource 0.0 @() @($accept)).Status -eq 'missing_height_profile')
    Assert-Selection ((Select-SceneSource -687.0 @($low) @($move)).Source -eq $null)
    $tie = [pscustomobject]@{ ActorId = '4'; Priority = 0; X = -50.0; Y = 60.0; Z = -687.0; DistanceSquared = 6100.0 }
    Assert-Selection ((Select-SceneSource -687.0 @($low) @($accept, $tie)).Status -eq 'ambiguous_equally_ranked_sources')
    $same = [pscustomobject]@{ ActorId = '5'; Priority = 0; X = 50.0; Y = 60.0; Z = -687.0; DistanceSquared = 6100.0 }
    Assert-Selection ((Select-SceneSource -687.0 @($low) @($accept, $same)).Status -eq 'source_verified')
    $nonfinite = [pscustomobject]@{ ActorId = '6'; Priority = 0; X = [double]::NaN; Y = 0.0; Z = -687.0; DistanceSquared = 0.0 }
    Assert-Selection ((Select-SceneSource -687.0 @($low) @($nonfinite)).Source -eq $null)
    Assert-Selection ((Get-SceneSourcePriority 'DQ1110033_1_AcceptActor') -eq 0)
    Assert-Selection ((Get-SceneSourcePriority 'NPCSpawnBase_DQ1110033_Empty') -eq 2)
    Assert-Selection ((Get-SceneSourcePriority 'NpcSpawnBase_DQ1110083_1_Welldone') -eq 1)
    Assert-Selection ((Get-SceneSourcePriority 'DQ1110033_Move_Check') -eq -1)
    Assert-Selection ((Get-SceneSourcePriority 'DQ1110033_StepPosition') -eq -1)
    $reviewed = [pscustomobject]@{ ActorId = '560707418635547564'; Priority = 3; X = 20.0; Y = 30.0; Z = 7864.0; DistanceSquared = 1210000.0 }
    $choice = Select-ReviewedSceneSource 1110038 @($reviewed)
    Assert-Selection ($choice.Source.X -eq 20.0 -and $choice.Source.Y -eq 30.0 -and $choice.Source.Z -eq 7864.0 -and $choice.Band -eq -1)
    Assert-Selection ($null -eq (Select-ReviewedSceneSource 1110039 @($reviewed)))
    foreach ($bad in @(
        [pscustomobject]@{ ActorId = 'other'; Priority = 3; X = 20.0; Y = 30.0; Z = 7864.0; DistanceSquared = 100.0 },
        [pscustomobject]@{ ActorId = '560707418635547564'; Priority = 3; X = 20.0; Y = 30.0; Z = 7864.0; DistanceSquared = 1210000.01 },
        [pscustomobject]@{ ActorId = '560707418635547564'; Priority = 3; X = [double]::NaN; Y = 30.0; Z = 7864.0; DistanceSquared = 100.0 }
    )) {
        $rejected = $false
        try { $null = Select-ReviewedSceneSource 1110038 @($bad) } catch { $rejected = $true }
        Assert-Selection $rejected
    }
}

Test-SceneSourceSelection
if ($SelfTestOnly) { 'Scene source selection self-tests passed.'; return }
if ([string]::IsNullOrWhiteSpace($CatalogPath)) {
    $CatalogPath = Join-Path $projectRoot 'src\data\generated\area-quests.tsv'
}
if ([string]::IsNullOrWhiteSpace($ActorPositionXmlPath)) {
    $ActorPositionXmlPath = Join-Path $projectRoot '..\DragonSwordWorldDataProbe\reference\assault-support\xml\017_ActorPositionData.xml'
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $projectRoot 'src\data\generated\area-quest-scene-anchors.tsv'
}
if ([string]::IsNullOrWhiteSpace($MetadataPath)) {
    $MetadataPath = Join-Path $projectRoot 'src\data\generated\area-quest-scene-anchors.metadata.json'
}
$sourceHash = (Get-FileHash -LiteralPath $ActorPositionXmlPath -Algorithm SHA256).Hash
if ($sourceHash -cne $expectedActorXmlSha256) { throw 'ActorPositionData source hash changed.' }
$baseHash = (Get-FileHash -LiteralPath $CatalogPath -Algorithm SHA256).Hash
$lines = @(Get-Content -LiteralPath $CatalogPath)
if ($lines.Count -ne 148 -or $lines[0] -cne "Id`tX`tY`tZ`tHeight1MinZ`tHeight1MaxZ`tHeight2MinZ`tHeight2MaxZ`tHeightBandCount") {
    throw 'The existing Area Quest catalog must retain its exact 147-row height-band schema.'
}
$baseRows = @(Import-Csv -LiteralPath $CatalogPath -Delimiter "`t")
$seen = [Collections.Generic.HashSet[int64]]::new()
[xml]$document = Get-Content -LiteralPath $ActorPositionXmlPath -Raw
$ns = 'http://leigient.549n.com/schema/ActorPositionInfo'
$actors = @($document.SelectNodes('//*[local-name()="ActorPositionData"]') | ForEach-Object {
    [pscustomobject]@{
        ActorId = $_.GetAttribute('ID', $ns)
        MapGroup = $_.GetAttribute('MapGroupID', $ns)
        Name = $_.GetAttribute('Name', $ns)
        X = Read-FiniteNumber ($_.GetAttribute('PosX', $ns))
        Y = Read-FiniteNumber ($_.GetAttribute('PosY', $ns))
        Z = Read-FiniteNumber ($_.GetAttribute('PosZ', $ns))
    }
})
$output = [Collections.Generic.List[string]]::new()
$output.Add("Id`tX`tY`tZ`tSourceAvailable")
$details = [Collections.Generic.List[object]]::new()
$available = 0
foreach ($row in $baseRows) {
    [int64]$id = 0
    if (-not [int64]::TryParse($row.Id, [ref]$id) -or $id -le 0 -or -not $seen.Add($id)) {
        throw 'Invalid or duplicate Area Quest ID.'
    }
    $x = Read-FiniteNumber $row.X
    $y = Read-FiniteNumber $row.Y
    $z = Read-FiniteNumber $row.Z
    if ($row.HeightBandCount -notin @('0', '1', '2')) { throw 'Invalid height-band count.' }
    $bands = @(for ($index = 1; $index -le [int]$row.HeightBandCount; ++$index) {
        [pscustomobject]@{
            MinimumZ = Read-FiniteNumber ($row.("Height${index}MinZ"))
            MaximumZ = Read-FiniteNumber ($row.("Height${index}MaxZ"))
        }
    })
    $pattern = '(?<![0-9])' + [regex]::Escape($row.Id) + '(?![0-9])'
    $allMatches = @($actors | Where-Object { $_.Name -match $pattern })
    $candidates = @($allMatches | Where-Object { $_.MapGroup -eq '100' } | ForEach-Object {
        [pscustomobject]@{
            ActorId = $_.ActorId
            Priority = Get-SceneSourcePriority $_.Name
            X = $_.X; Y = $_.Y; Z = $_.Z
            DistanceSquared = ($_.X - $x) * ($_.X - $x) + ($_.Y - $y) * ($_.Y - $y)
        }
    })
    $result = Select-ReviewedSceneSource $id $candidates
    $selectionMethod = 'height_band_and_ranked_entity'
    if ($null -eq $result) {
        $result = Select-SceneSource $z $bands $candidates
    } else {
        $selectionMethod = 'reviewed_named_entity_replaces_proxy_height'
    }
    $sourceActorId = ''
    $distanceMeters = $null
    if ($null -eq $result.Source) {
        $output.Add("$id`t0`t0`t0`t0")
    } else {
        ++$available
        $source = $result.Source
        $sourceActorId = $source.ActorId
        $distanceMeters = [Math]::Round([Math]::Sqrt($source.DistanceSquared) / 100.0, 6)
        $values = @($source.X, $source.Y, $source.Z | ForEach-Object { $_.ToString('0.################', $culture) })
        $output.Add("$id`t$($values[0])`t$($values[1])`t$($values[2])`t1")
    }
    $details.Add([ordered]@{
        id = $id; status = $result.Status; selected_height_band = $result.Band
        selection_method = $selectionMethod
        source_actor_id = $sourceActorId; source_planar_distance_meters = $distanceMeters
        exact_id_source_count_all_maps = $allMatches.Count
        near_source_count = @($candidates | Where-Object { $_.DistanceSquared -le 1000000.0 }).Count
    })
}
$expectedUnresolvedIds = @(1101301, 1103108, 1104104, 1104203)
$actualUnresolvedIds = @($details | Where-Object { $_.status -ne 'source_verified' } | ForEach-Object { $_.id })
if ($available -ne 143 -or ($actualUnresolvedIds -join ',') -cne ($expectedUnresolvedIds -join ',')) {
    throw 'Scene anchor coverage changed; review all changed source selections before updating the expected 143/4 split.'
}
$catalogText = ($output -join "`r`n") + "`r`n"
$sha = [Security.Cryptography.SHA256]::Create()
try {
    $catalogHash = ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($catalogText)))).Replace('-', '')
} finally { $sha.Dispose() }
$metadata = [ordered]@{
    schema_version = 1
    catalog = 'area-quest-scene-anchors.tsv'
    columns = @('Id', 'X', 'Y', 'Z', 'SourceAvailable')
    coordinate_units = 'unreal_centimeters'
    actor_position_source_sha256 = $sourceHash
    base_area_quest_catalog_sha256 = $baseHash
    catalog_crlf_sha256 = $catalogHash
    selection_policy = 'exact_task_id_map100_within10m_unique_height_band_then_accept_npc_interaction_then_nearest_full_xyz_except_explicit_reviewed_actor'
    reviewed_actor_overrides = @([ordered]@{
        id = 1110038; source_actor_id = '560707418635547564'; maximum_planar_distance_meters = 11
        reason = 'unique_named_task_entity_replaces_empty_proxy_derived_height_band_full_xyz'
    })
    unavailable_policy = 'all_zero_coordinates_source_available_0_no_raw_anchor_fallback'
    unresolved_reasons = @(
        [ordered]@{ id = 1101301; reason = 'only_move_check_and_step_positions_no_confirmed_task_start_or_interaction_entity' },
        [ordered]@{ id = 1103108; reason = 'no_exact_task_id_source_in_full_actor_position_data_all_maps' },
        [ordered]@{ id = 1104104; reason = 'no_exact_task_id_source_in_full_actor_position_data_all_maps' },
        [ordered]@{ id = 1104203; reason = 'no_exact_task_id_source_in_full_actor_position_data_all_maps' }
    )
    evidence_scope = 'authored_task_entity_origins_not_live_presence_ground_surface_or_gameplay_acceptance'
    raw_catalog_modified = $false
    rows = $baseRows.Count
    available_rows = $available
    unavailable_rows = $baseRows.Count - $available
    unresolved_ids = @($details | Where-Object { $_.status -ne 'source_verified' } | ForEach-Object { $_.id })
    records = $details.ToArray()
}
$metadataText = (($metadata | ConvertTo-Json -Depth 6) -replace '\r?\n', "`r`n") + "`r`n"
if ($VerifyOnly) {
    foreach ($expected in @(@($OutputPath, $catalogText), @($MetadataPath, $metadataText))) {
        if (-not (Test-Path -LiteralPath $expected[0] -PathType Leaf) -or
            (([IO.File]::ReadAllText($expected[0]) -replace "`r`n", "`n") -cne ($expected[1] -replace "`r`n", "`n"))) {
            throw 'Scene anchor catalog or verification metadata is not reproducible from the pinned source.'
        }
    }
} else {
    [IO.File]::WriteAllText($OutputPath, $catalogText, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($MetadataPath, $metadataText, [Text.UTF8Encoding]::new($false))
}
if ((Get-FileHash -LiteralPath $CatalogPath -Algorithm SHA256).Hash -cne $baseHash) {
    throw 'The original Area Quest catalog changed during generation.'
}
[pscustomobject]@{
    mode = if ($VerifyOnly) { 'VERIFIED' } else { 'WRITTEN' }
    rows = $baseRows.Count; available = $available; unavailable = $baseRows.Count - $available
    unresolved_ids = $metadata.unresolved_ids
    source_sha256 = $sourceHash; catalog_sha256 = $catalogHash
    raw_catalog_unchanged = $true
}
