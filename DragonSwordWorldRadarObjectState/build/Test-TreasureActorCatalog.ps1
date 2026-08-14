#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Parent $PSScriptRoot
$providerPath = Join-Path $root 'src\installer\Core\Providers\TreasureDataProvider.cs'
$providerSource = Get-Content -LiteralPath $providerPath -Raw
if ($providerSource -notmatch '(?s)TreasurePakExtractor\.Extract\(\s*context\.ExecutablePath,\s*pakPath,\s*context\.OodleLibraryPath,\s*BlueprintSourceEntry,\s*blueprintXmlPath\s*\)') {
    throw 'TreasureDataProvider must explicitly extract PropTreasureBoxData.xml instead of reusing the section-data default overload.'
}
Add-Type -AssemblyName System.Xml
$sources = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File |
    Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' } |
    Sort-Object FullName | Select-Object -ExpandProperty FullName)
$references = @(
    [System.Xml.XmlDocument].Assembly.Location,
    [System.Security.Cryptography.Aes].Assembly.Location,
    [System.Linq.Enumerable].Assembly.Location,
    [System.Uri].Assembly.Location) | Select-Object -Unique
Add-Type -Path $sources -ReferencedAssemblies $references -ErrorAction Stop

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('DragonSwordWorldRadarObjectState-ActorCatalog-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($temp) | Out-Null
try {
    $sectionPath = Join-Path $temp 'SectionTreasureBoxData.xml'
    $propPath = Join-Path $temp 'PropTreasureBoxData.xml'
    $outputPath = Join-Path $temp 'treasure-actors.tsv'
    $classes = @(
        'TreasureBox01','TreasureBox02','TreasureBox02_Mount',
        'TreasureBox03','TreasureBox03_Mount','TreasureBox03_OnlyFront',
        'TreasureBox04Key','TreasureBox04','TreasureBox05',
        'TreasureBox05_Mount','TreasureBox06')
    $settings = New-Object Xml.XmlWriterSettings
    $settings.Encoding = New-Object Text.UTF8Encoding($false)
    $settings.Indent = $false

    $sectionWriter = [Xml.XmlWriter]::Create($sectionPath, $settings)
    try {
        $sectionWriter.WriteStartElement('SectionTreasureBoxDataMap')
        for ($index = 0; $index -lt 1600; $index++) {
            $id = 1100000 + $index
            $sectionWriter.WriteStartElement('SectionActorData')
            $sectionWriter.WriteAttributeString('CID', [string]$id)
            $sectionWriter.WriteAttributeString('PosX', [string](1000 + $index))
            $sectionWriter.WriteAttributeString('PosY', [string](-2000 - $index))
            $sectionWriter.WriteAttributeString('PosZ', [string](300 + $index))
            $sectionWriter.WriteEndElement()
        }
        $sectionWriter.WriteEndElement()
    }
    finally { $sectionWriter.Dispose() }

    $propWriter = [Xml.XmlWriter]::Create($propPath, $settings)
    try {
        $propWriter.WriteStartElement('PropTreasureBoxDataMap')
        for ($index = 0; $index -lt 1600; $index++) {
            $id = 1100000 + $index
            $class = $classes[$index % $classes.Count]
            $propWriter.WriteStartElement('PropTreasureBoxData')
            $propWriter.WriteAttributeString('ID', [string]$id)
            $propWriter.WriteAttributeString(
                'BluePrintPath',
                "Blueprint'/Game/Treasure/$class.$class'")
            $propWriter.WriteEndElement()
        }
        $propWriter.WriteEndElement()
    }
    finally { $propWriter.Dispose() }

    $count = [DragonSwordWorldRadar.Installer.TreasureActorCatalogGenerator]::Generate(
        $sectionPath,
        $propPath,
        $outputPath)
    $rows = @(Import-Csv -Delimiter "`t" -LiteralPath $outputPath)
    if ($count -ne 1600 -or $rows.Count -ne 1600) {
        throw "Synthetic actor catalog count mismatch: generated=$count parsed=$($rows.Count)."
    }
    $classCount = @($rows | Group-Object ClassName).Count
    if ($classCount -ne 11 -or
        $rows[0].SaveId -ne '1100000' -or
        $rows[0].ClassName -ne 'TreasureBox01_C') {
        throw 'Synthetic actor catalog identity/class join failed.'
    }
    Write-Host "TREASURE_ACTOR_CATALOG_TESTS_OK rows=$count classes=$classCount"
}
finally {
    if ([IO.Directory]::Exists($temp)) {
        [IO.Directory]::Delete($temp, $true)
    }
}
