#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
Add-Type -AssemblyName System.Xml
$installerSources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' } |
    Sort-Object FullName |
    Select-Object -ExpandProperty FullName)
$references = @(
    [System.Xml.XmlDocument].Assembly.Location,
    [System.Security.Cryptography.Aes].Assembly.Location,
    [System.Linq.Enumerable].Assembly.Location,
    [System.Uri].Assembly.Location
) | Select-Object -Unique
Add-Type -Path $installerSources -ReferencedAssemblies $references -ErrorAction Stop

$firstId = 11001
$lastId = 11034
$expectedCount = 33
$temp = Join-Path ([IO.Path]::GetTempPath()) (
    'DragonSwordWorldRadar-RealFlyCatalogTest-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    $miniGamePath = Join-Path $temp 'MiniGameData.xml'
    $positionPath = Join-Path $temp 'ActorPositionData.xml'
    $outputPath = Join-Path $temp 'moles.lua'
    $utf8 = New-Object Text.UTF8Encoding($false)

    $miniGameXml = New-Object Text.StringBuilder
    $null = $miniGameXml.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
    $null = $miniGameXml.AppendLine('<MiniGameDataMap>')
    for ($id = $firstId; $id -le $lastId; $id++) {
        $null = $miniGameXml.AppendLine((
            '  <MiniGameData ID="{0}" DataLayer="MiniGame_Fly_{0}" MissionGroupID="101" MiniGameNoticeTitle="109208" MiniGameNoticeDescription="109202" VehicleRelease="true" />' -f $id))
    }
    $null = $miniGameXml.AppendLine('</MiniGameDataMap>')
    [IO.File]::WriteAllText($miniGamePath,$miniGameXml.ToString(),$utf8)

    $positionXml = New-Object Text.StringBuilder
    $null = $positionXml.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
    $null = $positionXml.AppendLine('<ActorPositionDataMap>')
    for ($id = $firstId; $id -le $lastId; $id++) {
        $index = $id - $firstId
        $npcX = 100000 + ($index * 100)
        $npcY = 200000 + ($index * 100)
        $npcZ = 3000 + $index
        $teleportX = $npcX + 5000
        $teleportY = $npcY + 5000
        $teleportZ = $npcZ + 500
        $mapId = if ($index -lt 17) { 100 } else { 200 }

        # Emit Teleport_Start first so document order cannot beat NPC_Start priority.
        $null = $positionXml.AppendLine((
            '  <ActorPositionData MapGroupID="{4}" Name="MiniGame_Fly_{0}_Teleport_Start" PosX="{1}" PosY="{2}" PosZ="{3}" />' -f
            $id,$teleportX,$teleportY,$teleportZ,$mapId))

        # The captured game data currently has no NPC_Start for 11024, so this
        # synthetic case validates the required Teleport_Start fallback.
        if ($id -ne 11024) {
            $null = $positionXml.AppendLine((
                '  <ActorPositionData MapGroupID="{4}" Name="MiniGame_Fly_{0}_NPC_Start" PosX="{1}" PosY="{2}" PosZ="{3}" />' -f
                $id,$npcX,$npcY,$npcZ,$mapId))
        }
    }
    $null = $positionXml.AppendLine('</ActorPositionDataMap>')
    [IO.File]::WriteAllText($positionPath,$positionXml.ToString(),$utf8)

    $count = [DragonSwordWorldRadar.Installer.MoleLuaGenerator]::Generate(
        [string[]]@($miniGamePath,$positionPath),
        $outputPath)
    if ($count -ne $expectedCount) {
        throw "Generator returned $count records; expected $expectedCount."
    }
    $additionalPath = Join-Path $temp 'AdditionalMiniGames.xml'
    $additionalXml = New-Object Text.StringBuilder
    $null = $additionalXml.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
    $null = $additionalXml.AppendLine('<ActorPositionDataMap>')
    foreach ($typeRange in @(@('Mole',12001,12040),@('Wave',13001,13010))) {
        for ($id = [int]$typeRange[1]; $id -le [int]$typeRange[2]; $id++) {
            $null = $additionalXml.AppendLine(('  <ActorPositionData MapGroupID="100" Name="MiniGame_{0}_{1}_Teleport_Start" PosX="{2}" PosY="{3}" PosZ="3000" />' -f $typeRange[0],$id,(300000+$id),(400000+$id)))
        }
    }
    $null = $additionalXml.AppendLine('</ActorPositionDataMap>')
    [IO.File]::WriteAllText($additionalPath,$additionalXml.ToString(),$utf8)
    [DragonSwordWorldRadar.Installer.AdditionalMiniGameLuaGenerator]::Append(
        [string[]]@($additionalPath),$outputPath)

    $generated = [IO.File]::ReadAllText($outputPath)
    $records = [regex]::Matches(
        $generated,
        '(?m)^\s*\{[^\r\n]*\bmini_game_id\s*=\s*(?<id>\d+)\b[^\r\n]*\bmask_bit\s*=\s*(?<bit>-?\d+)\b[^\r\n]*\bmap_id\s*=\s*(?<map>\d+)\b[^\r\n]*\bx\s*=\s*(?<x>[-+0-9.eE]+)\b[^\r\n]*\by\s*=\s*(?<y>[-+0-9.eE]+)\b[^\r\n]*\bposition_role\s*=\s*"(?<role>[^"]+)"')
    if ($records.Count -ne 83) {
        throw "Generated Lua parsed $($records.Count) records; expected 83."
    }

    $seenIds = @{}
    $seenBits = @{}
    $mapCounts = @{}
    for ($index = 0; $index -lt $expectedCount; $index++) {
        $record = $records[$index]
        $id = [int]$record.Groups['id'].Value
        $bit = [int]$record.Groups['bit'].Value
        $map = [int]$record.Groups['map'].Value
        $x = [double]::Parse($record.Groups['x'].Value,[Globalization.CultureInfo]::InvariantCulture)
        $y = [double]::Parse($record.Groups['y'].Value,[Globalization.CultureInfo]::InvariantCulture)
        $role = $record.Groups['role'].Value
        $expectedId = if ($index -lt 23) { $firstId + $index } else { $firstId + $index + 1 }
        if ($id -ne $expectedId -or $bit -ne $index) {
            throw "Fly record ordering/mask mismatch at index ${index}: id=$id bit=$bit."
        }
        if ($seenIds.ContainsKey($id) -or $seenBits.ContainsKey($bit)) {
            throw "Fly identifiers are not unique at index $index."
        }
        $seenIds[$id] = $true
        $seenBits[$bit] = $true
        $mapCounts[$map] = 1 + [int]$mapCounts[$map]

        $sourceIndex = $id - $firstId
        $baseX = 100000 + ($sourceIndex * 100)
        $baseY = 200000 + ($sourceIndex * 100)
        if ($role -ne 'NPC_Start' -or $x -ne $baseX -or $y -ne $baseY) {
            throw "Fly ID $id did not select NPC_Start."
        }
    }
    if ($mapCounts.Count -ne 2 -or $mapCounts[100] -ne 17 -or $mapCounts[200] -ne 16) {
        throw "Fly map grouping changed: $($mapCounts | Out-String)"
    }

    if (-not (Test-Path -LiteralPath (Join-Path $temp 'mole-anchor-candidates.tsv') -PathType Leaf)) {
        throw 'Generator did not emit the candidate audit report.'
    }

    Write-Host 'MINIGAME_CATALOG_TEST_OK records=83; Fly=33; Mole=40; Wave=10; flyMaskBits=0-32.'
}
finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
