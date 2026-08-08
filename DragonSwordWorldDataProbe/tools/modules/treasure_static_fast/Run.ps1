param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'

$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$ctx=Import-ModuleContext -ContextPath $ContextPath
$fingerprintInfo=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)

try{
    $supported='85d072028086faac03bd39f783126254e624405cdb2784da04395d829c17a25f'
    if([string]$fingerprintInfo.build_fingerprint -ne $supported){
        Write-ModuleResult -Context $ctx -Status blocked `
            -Blockers @('build_mismatch') `
            -NextAction 'Treasure static extractor is build-pinned.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
        exit 0
    }

    Write-ModuleProgress -Context $ctx -Step 1 -Total 5 `
        -Stage 'start_treasure_static_extractor' `
        -Detail 'Targets: PropTreasureBoxData.xml + SectionTreasureBoxData.xml.'

    $probeOut=Join-Path $moduleDir 'treasure_static_fast_probe'
    Remove-Item -LiteralPath $probeOut -Recurse -Force -ErrorAction SilentlyContinue

    $script=Join-Path $moduleDir 'KindPlaceFastProbe.ps1'
    $stdout=Join-Path ([string]$ctx.output_dir) 'treasure-static.stdout.log'
    $stderr=Join-Path ([string]$ctx.output_dir) 'treasure-static.stderr.log'
    $args='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "{0}"' -f $script

    $child=Start-Process powershell.exe `
        -ArgumentList $args `
        -WindowStyle Hidden `
        -PassThru `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr

    Write-ModuleProgress -Context $ctx -Step 2 -Total 5 `
        -Stage 'wait_treasure_static_extractor' `
        -Detail 'Hard timeout 60s.'

    $completed=$child.WaitForExit(60000)
    if(-not$completed){
        try{$child.Kill()}catch{}
        Write-ModuleResult -Context $ctx -Status timeout `
            -Blockers @('Treasure static extractor timeout') `
            -NextAction 'Review child logs.'|Out-Null
        exit 2
    }

    $dest=Join-Path ([string]$ctx.output_dir) 'xml'
    New-Item -ItemType Directory -Force -Path $dest|Out-Null

    $src=Join-Path $probeOut 'xml'
    $foundProp=$false
    $foundSection=$false

    if(Test-Path -LiteralPath $src -PathType Container){
        foreach($f in Get-ChildItem -LiteralPath $src -File -Filter '*.xml'){
            $logical=$f.Name -replace '^\d+_',''
            if($logical -eq 'PropTreasureBoxData.xml'){
                Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $dest $logical) -Force
                $foundProp=$true
            }
            elseif($logical -eq 'SectionTreasureBoxData.xml'){
                Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $dest $logical) -Force
                $foundSection=$true
            }
        }
    }

    Write-ModuleProgress -Context $ctx -Step 3 -Total 5 `
        -Stage 'copy_extractor_evidence' `
        -Detail ('prop='+$foundProp+'; section='+$foundSection)

    foreach($name in @(
        'treasure_static_fast.log',
        'treasure_static_fast_last_stage.txt'
    )){
        $p=Join-Path $moduleDir $name
        if(Test-Path -LiteralPath $p){
            Copy-Item -LiteralPath $p -Destination ([string]$ctx.output_dir) -Force
        }
    }

    foreach($name in @(
        'extraction_manifest.txt',
        'all_game_data_xml_entries.csv'
    )){
        $p=Join-Path $probeOut $name
        if(Test-Path -LiteralPath $p){
            Copy-Item -LiteralPath $p -Destination ([string]$ctx.output_dir) -Force
        }
    }

    foreach($dirName in @('decoder_evidence','xor16_debug')){
        $p=Join-Path $probeOut $dirName
        if(Test-Path -LiteralPath $p -PathType Container){
            Copy-Item -LiteralPath $p `
                -Destination (Join-Path ([string]$ctx.output_dir) $dirName) `
                -Recurse -Force
        }
    }

    Write-ModuleProgress -Context $ctx -Step 4 -Total 5 `
        -Stage 'validate_treasure_xml_identity' `
        -Detail 'Checking XML roots and record counts.'

    $propPath=Join-Path $dest 'PropTreasureBoxData.xml'
    $sectionPath=Join-Path $dest 'SectionTreasureBoxData.xml'

    $propRows=0
    $sectionRows=0
    $propRoot=''
    $sectionRoot=''

    if($foundProp){
        try{
            [xml]$doc=Get-Content -LiteralPath $propPath -Raw
            $propRoot=[string]$doc.DocumentElement.LocalName
            $propRows=@($doc.DocumentElement.SelectNodes('./*')).Count
        }catch{}
    }

    if($foundSection){
        try{
            [xml]$doc=Get-Content -LiteralPath $sectionPath -Raw
            $sectionRoot=[string]$doc.DocumentElement.LocalName
            $sectionRows=@($doc.DocumentElement.SelectNodes('./*')).Count
        }catch{}
    }

    $evidence=[ordered]@{
        prop_found=$foundProp
        prop_root=$propRoot
        prop_rows=$propRows
        section_found=$foundSection
        section_root=$sectionRoot
        section_rows=$sectionRows
    }

    $evidencePath=Join-Path ([string]$ctx.output_dir) 'treasure-static-validation.json'
    Write-JsonUtf8NoBom -Path $evidencePath -Value $evidence -Depth 8

    Write-ModuleProgress -Context $ctx -Step 5 -Total 5 `
        -Stage 'finalize_treasure_static' `
        -Detail ('prop_rows='+$propRows+'; section_rows='+$sectionRows)

    $success=$foundProp -and $foundSection -and $propRows -gt 0 -and $sectionRows -gt 0

    $outputs=@($stdout,$stderr,$evidencePath)
    foreach($p in @($propPath,$sectionPath)){
        if(Test-Path -LiteralPath $p){$outputs+=$p}
    }

    Write-ModuleResult -Context $ctx `
        -Status $(if($success){'success'}else{'partial'}) `
        -Metrics @{
            prop_found=$foundProp
            prop_rows=$propRows
            section_found=$foundSection
            section_rows=$sectionRows
        } `
        -Outputs $outputs `
        -Blockers $(if($success){@()}else{@('one or both Treasure static XML tables incomplete')}) `
        -NextAction 'Parse BlueprintPath and join PropTreasureBoxData to SectionTreasureBoxData.' `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error `
        -Blockers @($message) `
        -NextAction 'Review Treasure static extractor logs.'|Out-Null
    exit 1
}
