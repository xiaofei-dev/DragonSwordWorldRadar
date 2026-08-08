param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference = 'Stop'
$moduleDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir = (Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
$ctx = Import-ModuleContext -ContextPath $ContextPath
$script:logPath = Join-Path $ctx.run_output_dir 'decoder-evidence.log'
$script:stagePath = Join-Path $ctx.run_output_dir 'last-stage.txt'
$script:utf8 = New-Object System.Text.UTF8Encoding -ArgumentList $false
. (Join-Path $toolsDir 'core\PakCore.ps1')
$staticResultPath = Join-Path $ctx.root 'runtime\modules\pak_static\current\result.json'
if (-not (Test-Path -LiteralPath $staticResultPath -PathType Leaf)) {
    Write-ModuleResult -Context $ctx -Status skipped -NextAction 'pak_static has not run.' | Out-Null; exit 0
}
$staticResult = Read-JsonUtf8Strict -Path $staticResultPath
if ([string]$staticResult.status -in @('success','skipped')) {
    Write-ModuleResult -Context $ctx -Status skipped -Metrics @{ reason='critical_static_entries_complete' } -NextAction 'No decoder evidence is needed.' | Out-Null; exit 0
}
$pakContext = $null
try {
    $suite = Read-JsonUtf8Strict -Path $ctx.suite_path
    $targets = Read-JsonUtf8Strict -Path $ctx.target_profile_path
    $pakContext = Open-DSPakContext -Root $ctx.root -Suite $suite
    $missing = @($staticResult.blockers | Where-Object { $_ -like '*.xml' })
    if ($missing.Count -eq 0) { $missing = @($targets.critical_files); if ($missing.Count -eq 0) { $missing = @($targets.targets | Where-Object { $_.required -eq $true } | ForEach-Object { [string]$_.file }) } }
    $entryMap = @{}; foreach ($entry in @($pakContext.Entries)) { $entryMap[[string]$entry.File] = $entry }
    $allCandidates = New-Object System.Collections.Generic.List[object]
    $evidenceRows = New-Object System.Collections.Generic.List[object]
    $rawDir = Join-Path $ctx.output_dir 'raw-encoded-windows'
    New-Item -ItemType Directory -Force -Path $rawDir | Out-Null
    foreach ($name in $missing) {
        if (-not $entryMap.ContainsKey([string]$name)) {
            $evidenceRows.Add([pscustomobject]@{ file=$name; status='missing_from_directory_index'; encoded_offset=''; raw_file=''; candidate_count=0 }); continue
        }
        $entry = $entryMap[[string]$name]
        [byte[]]$raw = Get-DSRawEncodedWindow -EncodedEntries $pakContext.EncodedEntries -Offset ([int]$entry.EncodedOffset) -Before 32 -After 128
        [byte[]]$record = if ([int]$entry.EncodedRecordLength -gt 0) { Get-DSRawEncodedWindow -EncodedEntries $pakContext.EncodedEntries -Offset ([int]$entry.EncodedOffset) -Before 0 -After ([int]$entry.EncodedRecordLength) } else { [byte[]]@() }
        $rawPath = Join-Path $rawDir ((Sanitize $name) + '.window.bin')
        [IO.File]::WriteAllBytes($rawPath,$raw)
        $recordPath = Join-Path $rawDir ((Sanitize $name) + '.record.bin')
        [IO.File]::WriteAllBytes($recordPath,$record)
        $hexPath = Join-Path $rawDir ((Sanitize $name) + '.hex.txt')
        $hex = New-Object System.Collections.Generic.List[string]
        for ($i=0; $i -lt $raw.Length; $i+=16) {
            $count=[Math]::Min(16,$raw.Length-$i); $slice=New-Object byte[] $count; [Array]::Copy($raw,$i,$slice,0,$count)
            $hex.Add(('{0:X8}: {1}' -f ([Math]::Max(0,[int]$entry.EncodedOffset-32)+$i),([BitConverter]::ToString($slice).Replace('-',' '))))
        }
        $hex | Set-Content -LiteralPath $hexPath -Encoding ASCII
        $candidates = @(Get-DSDecodeCandidates -Context $pakContext -Entry $entry)
        foreach ($candidate in $candidates) { $allCandidates.Add($candidate) }
        $evidenceRows.Add([pscustomobject]@{ file=$name; status='captured'; encoded_offset=$entry.EncodedOffset; next_offset=$entry.NextEncodedOffset; record_length=$entry.EncodedRecordLength; raw_window=$rawPath; raw_record=$recordPath; candidate_count=$candidates.Count })
    }
    $allCandidates | Export-Csv -LiteralPath (Join-Path $ctx.output_dir 'decode-candidates.csv') -NoTypeInformation -Encoding UTF8
    $evidenceRows | Export-Csv -LiteralPath (Join-Path $ctx.output_dir 'target-evidence.csv') -NoTypeInformation -Encoding UTF8

    # Dump method signatures and readable IL. This exposes the actual custom mask/search logic
    # so the next decoder patch is evidence-driven rather than another header guess.
    $ilSource = Join-Path $moduleDir 'ILInspector.cs'
    if (-not ('DSILInspector' -as [type])) { Add-Type -TypeDefinition ([IO.File]::ReadAllText($ilSource)) -Language CSharp }
    $methodsOut = New-Object Text.StringBuilder
    foreach ($spec in @(
        @($pakContext.ProgramType,'InferNumberMask'),@($pakContext.ProgramType,'FindEncodedEntry'),
        @($pakContext.ProgramType,'FindTargetEncodedOffset'),@($pakContext.ProgramType,'ReadEncodedEntry'),
        @($pakContext.ProgramType,'ReadDataEntry'),@($pakContext.ProgramType,'ValidateEntry'),
        @($pakContext.ReaderType,'ReadUInt32'),@($pakContext.ReaderType,'ReadUInt64'),
        @($pakContext.ReaderType,'ReadInt32')
    )) {
        $type=$spec[0]; $name=[string]$spec[1]
        foreach ($method in @($type.GetMethods([Reflection.BindingFlags]'Static,Instance,Public,NonPublic') | Where-Object { $_.Name -eq $name })) {
            [void]$methodsOut.AppendLine([DSILInspector]::Disassemble($method)); [void]$methodsOut.AppendLine('---')
        }
    }
    [IO.File]::WriteAllText((Join-Path $ctx.output_dir 'pakcore-decoder-il.txt'),$methodsOut.ToString(),$utf8)
    $meta = [ordered]@{
        schema_version=1; pak_length=$pakContext.PakLength; encoded_entries_length=$pakContext.EncodedEntries.Length;
        number_mask=$pakContext.NumberMask; string_mask=$pakContext.StringMask; directory_string_mask=$pakContext.DirectoryStringMask;
        missing_targets=$missing; exact_diagnostic_count=$allCandidates.Count;
        safety='AES key intentionally omitted; exact offsets only; no adjacent-record substitution';
        next_decoder_reference='UE V11 compact entry: bits, compression slot, encryption flag, block count/size, 32/64-bit offset and size flags, block sizes.';
        generated_utc=(Get-Date).ToUniversalTime().ToString('o')
    }
    Write-JsonUtf8NoBom -Path (Join-Path $ctx.output_dir 'decoder-evidence-summary.json') -Value $meta -Depth 8
    Write-ModuleResult -Context $ctx -Status partial -Metrics @{ target_count=$evidenceRows.Count; exact_diagnostics=$allCandidates.Count } -Outputs @((Join-Path $ctx.output_dir 'decoder-evidence-summary.json'),(Join-Path $ctx.output_dir 'pakcore-decoder-il.txt'),(Join-Path $ctx.output_dir 'decode-candidates.csv')) -Blockers @($missing) -NextAction 'Review the exact compact-record parser error and control entry. Never retry neighboring offsets or alternate masks.' -Fingerprint ((Get-BuildFingerprint -Layout $pakContext.Layout)+':decoder_evidence_v100') | Out-Null
}
catch {
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error -Blockers @($message) -NextAction 'Review decoder-evidence.log.' | Out-Null
    exit 1
}
finally { Close-DSPakContext -Context $pakContext }
