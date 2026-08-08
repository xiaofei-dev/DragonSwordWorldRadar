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
  Write-ModuleResult -Context $ctx -Status blocked -Blockers @('build_mismatch') -NextAction 'Respawn cycle fast extractor is build-pinned.' -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
  exit 0
 }
 Write-ModuleProgress -Context $ctx -Step 1 -Total 4 -Stage 'start_respawn_cycle_extractor' -Detail 'Target: RespawnCycleData.xml'
 $probeOut=Join-Path $moduleDir 'respawn_cycle_fast_probe'
 Remove-Item -LiteralPath $probeOut -Recurse -Force -ErrorAction SilentlyContinue
 $script=Join-Path $moduleDir 'KindPlaceFastProbe.ps1'
 $stdout=Join-Path ([string]$ctx.output_dir) 'respawn-cycle.stdout.log'
 $stderr=Join-Path ([string]$ctx.output_dir) 'respawn-cycle.stderr.log'
 $args='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "{0}"' -f $script
 $child=Start-Process powershell.exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
 Write-ModuleProgress -Context $ctx -Step 2 -Total 4 -Stage 'wait_respawn_cycle_extractor' -Detail 'Hard timeout 60s.'
 $completed=$child.WaitForExit(60000)
 if(-not$completed){try{$child.Kill()}catch{};Write-ModuleResult -Context $ctx -Status timeout -Blockers @('RespawnCycle extractor timeout') -NextAction 'Review child logs.'|Out-Null;exit 2}
 $dest=Join-Path ([string]$ctx.output_dir) 'xml'
 New-Item -ItemType Directory -Force -Path $dest|Out-Null
 $src=Join-Path $probeOut 'xml'
 $found=$false
 if(Test-Path $src){
  foreach($f in Get-ChildItem $src -File -Filter '*.xml'){
   $logical=$f.Name -replace '^\d+_',''
   if($logical -eq 'RespawnCycleData.xml'){
    Copy-Item $f.FullName (Join-Path $dest $logical) -Force;$found=$true
   }
  }
 }
 foreach($name in @('respawn_cycle_fast.log','respawn_cycle_fast_last_stage.txt')){
  $p=Join-Path $moduleDir $name;if(Test-Path $p){Copy-Item $p ([string]$ctx.output_dir) -Force}
 }
 foreach($name in @('extraction_manifest.txt','all_game_data_xml_entries.csv')){
  $p=Join-Path $probeOut $name;if(Test-Path $p){Copy-Item $p ([string]$ctx.output_dir) -Force}
 }
 Write-ModuleProgress -Context $ctx -Step 3 -Total 4 -Stage 'validate_respawn_cycle_xml' -Detail ('found='+$found)
 $xml=Join-Path $dest 'RespawnCycleData.xml'
 $cycle105=$null
 $allRows=@()
 if($found){
  try{
   $doc=New-Object Xml.XmlDocument
   $doc.PreserveWhitespace=$false
   $doc.Load($xml)

   # Load() reads the original UTF-8 bytes according to the XML declaration,
   # avoiding Windows PowerShell's Get-Content text-decoding corruption.
   $nodes=@($doc.DocumentElement.SelectNodes('./*'))

   foreach($node in $nodes){
    if($null-eq$node){continue}

    $row=[ordered]@{}
    foreach($attribute in @($node.Attributes)){
     if($null-eq$attribute){continue}
     $row[[string]$attribute.LocalName]=[string]$attribute.Value
    }

    if($row.Count -eq 0){continue}

    $record=[pscustomobject]$row
    $allRows+=$record

    if([string]$row['ID'] -eq '105'){
     $cycle105=$record
    }
   }
  }catch{
   Write-ModuleLog -Context $ctx -Event 'RESPAWN_XML_PARSE_ERROR' `
    -Message (Get-ExceptionSummary -ErrorObject $_)
  }
 }

 $allPath=Join-Path ([string]$ctx.output_dir) 'respawn-cycle-all.tsv'
 if($allRows.Count -gt 0){
  $allRows|Export-Csv -LiteralPath $allPath -Delimiter "`t" -NoTypeInformation -Encoding UTF8
 }

 $evidence=Join-Path ([string]$ctx.output_dir) 'respawn-cycle-105.json'
 if($null-ne$cycle105){
  Write-JsonUtf8NoBom -Path $evidence -Value $cycle105 -Depth 8
 }else{
  Write-JsonUtf8NoBom -Path $evidence -Value ([ordered]@{
   found=$false
   id=105
   parsed_rows=$allRows.Count
  }) -Depth 8
 }

 $cycle105Found=$null-ne$cycle105
 $cycle105Type=if($cycle105Found){[string]$cycle105.RespawnType}else{''}
 $cycle105RealTime=if($cycle105Found){[string]$cycle105.RespawnRealTime}else{''}

 Write-ModuleProgress -Context $ctx -Step 4 -Total 4 `
  -Stage 'finalize_respawn_cycle' `
  -Detail ('cycle105_found='+$cycle105Found+
   '; type='+$cycle105Type+
   '; real_time='+$cycle105RealTime)

 $outputs=@($stdout,$stderr,$xml,$evidence)
 if(Test-Path -LiteralPath $allPath){$outputs+=$allPath}

 Write-ModuleResult -Context $ctx `
  -Status $(if($found -and $cycle105Found){'success'}elseif($found){'partial'}else{'blocked'}) `
  -Metrics @{
   xml=$found
   rows=$allRows.Count
   cycle105=$cycle105Found
   cycle105_type=$cycle105Type
   cycle105_real_time=$cycle105RealTime
   child_exit=$child.ExitCode
  } `
  -Outputs $outputs `
  -Blockers $(if($cycle105Found){@()}elseif($found){@('RespawnCycleID 105 not parsed')}else{@('RespawnCycleData.xml')}) `
  -NextAction 'Cycle 105 is now parsed namespace-safely; correlate DAILY reset timing with DaySwitch/save state.' `
  -Fingerprint ([string]$fingerprintInfo.build_fingerprint)|Out-Null
}catch{
 $m=Get-ExceptionSummary -ErrorObject $_
 Write-ModuleResult -Context $ctx -Status error -Blockers @($m) -NextAction 'Review RespawnCycle fast logs.'|Out-Null
 exit 1
}
