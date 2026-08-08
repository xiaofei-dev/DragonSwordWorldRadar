param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'

$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$ctx=Import-ModuleContext -ContextPath $ContextPath
$target=Read-JsonUtf8Strict -Path (Join-Path ([string]$ctx.root) 'config\static-targets\mole_game.json')
$fingerprintInfo=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)

function Read-XmlRows {
    param([string]$Path)

    $rows=@()
    if(-not(Test-Path -LiteralPath $Path -PathType Leaf)){return $rows}

    try{
        [xml]$doc=[IO.File]::ReadAllText($Path)

        foreach($node in @(
            $doc.SelectNodes('//*') |
                Where-Object {
                    $_.NodeType -eq [Xml.XmlNodeType]::Element -and
                    $_.Attributes.Count -gt 0
                }
        )){
            $row=[ordered]@{
                SourceFile=([IO.Path]::GetFileName($Path) -replace '^\d+_','')
                Element=$node.LocalName
            }

            foreach($attr in @($node.Attributes)){
                $row[$attr.LocalName]=[string]$attr.Value
            }

            foreach($child in @(
                $node.ChildNodes |
                    Where-Object {
                        $_.NodeType -eq [Xml.XmlNodeType]::Element -and
                        @($_.ChildNodes | Where-Object {
                            $_.NodeType -eq [Xml.XmlNodeType]::Element
                        }).Count -eq 0
                    }
            )){
                $row[$child.LocalName]=[string]$child.InnerText.Trim()
            }

            $rows+=[pscustomobject]$row
        }
    }
    catch{
    }

    return $rows
}

function Export-Rows {
    param($Rows,[string]$Path)

    $rowsArray=@($Rows)
    if($rowsArray.Count-eq0){return}

    $fields=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach($row in $rowsArray){
        foreach($property in $row.PSObject.Properties){
            [void]$fields.Add([string]$property.Name)
        }
    }

    $ordered=@('SourceFile','Element')+
        @($fields|Where-Object{$_-notin@('SourceFile','Element')}|Sort-Object)

    $normalized=foreach($row in $rowsArray){
        $out=[ordered]@{}
        foreach($name in $ordered){
            $property=$row.PSObject.Properties[$name]
            $out[$name]=if($null-ne$property){[string]$property.Value}else{''}
        }
        [pscustomobject]$out
    }

    $normalized|Export-Csv -LiteralPath $Path -NoTypeInformation -Encoding UTF8
}

try{
    $probeDir=Join-Path $moduleDir 'mole_static_probe'
    $xmlDir=Join-Path $probeDir 'xml'
    $outStatic=Join-Path ([string]$ctx.output_dir) 'static'

    Remove-Item -LiteralPath $outStatic -Recurse -Force -ErrorAction SilentlyContinue
    if(Test-Path -LiteralPath $probeDir -PathType Container){
        Copy-Item -LiteralPath $probeDir -Destination $outStatic -Recurse -Force
    }

    $moleIds=@()
    $moleIdSet=@{}
    for($id=12001;$id-le12040;$id++){
        $text=[string]$id
        $moleIds+=$text
        $moleIdSet[$text]=$true
    }

    $allRows=@()
    if(Test-Path -LiteralPath $xmlDir -PathType Container){
        foreach($file in Get-ChildItem -LiteralPath $xmlDir -File -Filter '*.xml'){
            $allRows+=@(Read-XmlRows -Path $file.FullName)
        }
    }

    # Preserve the full MiniGameData table, not just guessed Mole columns.
    $miniGameRows=@($allRows|Where-Object{$_.SourceFile-eq'MiniGameData.xml'})
    $miniGameCsv=Join-Path ([string]$ctx.output_dir) 'MiniGameData-full.csv'
    Export-Rows -Rows $miniGameRows -Path $miniGameCsv

    # Exact numeric cross-reference: any static field whose whole value is 12001..12040,
    # or whose text contains MiniGame_Mole_120xx.
    $crossRefs=@()
    foreach($row in $allRows){
        foreach($property in $row.PSObject.Properties){
            $value=[string]$property.Value
            if([string]::IsNullOrWhiteSpace($value)){continue}

            $matchedId=''
            if($moleIdSet.ContainsKey($value.Trim())){
                $matchedId=$value.Trim()
            }
            elseif($value-match'MiniGame_Mole_(120(?:0[1-9]|[1-3][0-9]|40))'){
                $matchedId=[string]$Matches[1]
            }

            if(-not[string]::IsNullOrWhiteSpace($matchedId)){
                $crossRefs+=[pscustomobject]@{
                    MoleID=$matchedId
                    SourceFile=[string]$row.SourceFile
                    Element=[string]$row.Element
                    MatchedField=[string]$property.Name
                    MatchedValue=$value
                    ID=[string]$row.ID
                    CID=[string]$row.CID
                    UID=[string]$row.UID
                    UIDName=[string]$row.UIDName
                    GroupID=[string]$row.GroupID
                    SectionUID=[string]$row.SectionUID
                    LevelCID=[string]$row.LevelCID
                    PosX=[string]$row.PosX
                    PosY=[string]$row.PosY
                    PosZ=[string]$row.PosZ
                    SpawnConditionID=[string]$row.SpawnConditionID
                    RespawnCycleID=[string]$row.RespawnCycleID
                    RandomSpawnID=[string]$row.RandomSpawnID
                }
            }
        }
    }

    $crossRefs=@($crossRefs|Sort-Object MoleID,SourceFile,Element,MatchedField -Unique)
    $crossRefCsv=Join-Path ([string]$ctx.output_dir) 'mole-12001-12040-static-crossrefs.csv'
    if($crossRefs.Count-gt0){
        $crossRefs|Export-Csv -LiteralPath $crossRefCsv -NoTypeInformation -Encoding UTF8
    }

    # Schema/ObjectDump evidence.
    $exactNodeHits=@()
    $classHits=@()
    $objectHits=@()

    try{
        $probeRoot=[IO.Path]::GetFullPath([string]$ctx.root).TrimEnd('\')
        $modsRoot=Split-Path -Parent $probeRoot
        $win64=Split-Path -Parent $modsRoot
        $dump=Join-Path $win64 'UE4SS_ObjectDump.txt'

        if(Test-Path -LiteralPath $dump -PathType Leaf){
            $regex=@($target.objectdump_patterns|ForEach-Object{[string]$_})-join'|'

            $objectHits=@(
                Select-String -LiteralPath $dump -Pattern $regex |
                    Select-Object -First 10000 |
                    ForEach-Object{[string]$_.Line}
            )

            $exactNodeHits=@(
                Select-String `
                    -LiteralPath $dump `
                    -Pattern 'MiniGame_Mole_120(0[1-9]|[1-3][0-9]|40)' |
                    Select-Object -First 10000 |
                    ForEach-Object{[string]$_.Line}
            )

            $classHits=@(
                Select-String `
                    -LiteralPath $dump `
                    -Pattern 'DsMiniGameNode_Mole|DsMiniGameMoleAsset|DsMiniGameMoleComponent|DsMiniGameMoleData|DMiniGameTable|EMoleType|RoundSetting' |
                    Select-Object -First 10000 |
                    ForEach-Object{[string]$_.Line}
            )
        }
    }
    catch{
    }

    $objectPath=Join-Path ([string]$ctx.output_dir) 'mole-objectdump-candidates.txt'
    $nodePath=Join-Path ([string]$ctx.output_dir) 'mole-12001-12040-objectdump.txt'
    $classPath=Join-Path ([string]$ctx.output_dir) 'mole-class-schema-objectdump.txt'

    if($objectHits.Count-gt0){$objectHits|Set-Content -LiteralPath $objectPath -Encoding UTF8}
    if($exactNodeHits.Count-gt0){$exactNodeHits|Set-Content -LiteralPath $nodePath -Encoding UTF8}
    if($classHits.Count-gt0){$classHits|Set-Content -LiteralPath $classPath -Encoding UTF8}

    $representedIds=@($crossRefs|ForEach-Object{[string]$_.MoleID}|Sort-Object -Unique)

    $summary=[ordered]@{
        schema_version=1
        known_mole_node_count=40
        known_mole_id_min=12001
        known_mole_id_max=12040
        confirmed_internal_class='DsMiniGameNode_Mole'
        confirmed_static_table='DMiniGameTable'
        extracted_minigame_rows=$miniGameRows.Count
        static_crossref_rows=$crossRefs.Count
        static_ids_represented=$representedIds.Count
        exact_mole_objectdump_lines=$exactNodeHits.Count
        mole_class_schema_lines=$classHits.Count
        build_fingerprint=[string]$fingerprintInfo.build_fingerprint
        production_ready=$false
        desired_rule='unfinished => show; completed => hide'
        generated_utc=(Get-Date).ToUniversalTime().ToString('o')
    }

    $summaryPath=Join-Path ([string]$ctx.output_dir) 'mole-discovery-summary.json'
    Write-JsonUtf8NoBom -Path $summaryPath -Value $summary -Depth 8

    $outputs=@($summaryPath)
    foreach($path in @($miniGameCsv,$crossRefCsv,$objectPath,$nodePath,$classPath,$outStatic)){
        if(Test-Path -LiteralPath $path){$outputs+=$path}
    }

    $status=
        if($representedIds.Count-eq40 -and $miniGameRows.Count-gt0){'partial'}
        elseif($crossRefs.Count-gt0 -or $exactNodeHits.Count-gt0){'partial'}
        else{'blocked'}

    $blockers=@()
    if($representedIds.Count-lt40){
        $blockers+=('Static mapping represents '+$representedIds.Count+'/40 Mole IDs.')
    }
    if($miniGameRows.Count-eq0){
        $blockers+='MiniGameData.xml was not extracted.'
    }
    $blockers+='Persistent completed/unfinished state is not yet confirmed.'

    Write-ModuleResult -Context $ctx -Status $status `
        -Metrics @{
            known_nodes=40
            minigame_rows=$miniGameRows.Count
            crossrefs=$crossRefs.Count
            ids_represented=$representedIds.Count
            objectdump_nodes=$exactNodeHits.Count
        } `
        -Outputs $outputs `
        -Blockers $blockers `
        -NextAction 'Use exact static crossrefs plus one persistent completion-state source; do not use streamed Actor existence.' `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error `
        -Blockers @($message) `
        -NextAction 'Review audited Mole discovery logs.' | Out-Null
    exit 1
}
