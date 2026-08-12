param([Parameter(Mandatory=$true)][string]$ContextPath)

$ErrorActionPreference = 'Stop'
$moduleDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir = (Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')
$ctx = Import-ModuleContext -ContextPath $ContextPath
$fingerprintInfo = Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)

try {
    $supported = '85d072028086faac03bd39f783126254e624405cdb2784da04395d829c17a25f'
    if ([string]$fingerprintInfo.build_fingerprint -ne $supported) {
        Write-ModuleResult -Context $ctx -Status blocked -Blockers @('build_mismatch') `
            -NextAction 'Reveal cycle extractor is build-pinned.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 0
    }

    Write-ModuleProgress -Context $ctx -Step 1 -Total 4 `
        -Stage 'start_reveal_cycle_extractor' -Detail 'Target: RevealCycleData.xml'

    $sharedScript = Join-Path $toolsDir 'modules\assault_respawn_cycle_fast\KindPlaceFastProbe.ps1'
    $probeOut = Join-Path $moduleDir 'reveal_cycle_fast_probe'
    Remove-Item -LiteralPath $probeOut -Recurse -Force -ErrorAction SilentlyContinue
    $stdout = Join-Path ([string]$ctx.output_dir) 'reveal-cycle.stdout.log'
    $stderr = Join-Path ([string]$ctx.output_dir) 'reveal-cycle.stderr.log'
    $arguments = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass ' +
        ('-File "{0}" -TargetFile "RevealCycleData.xml" -OutputRoot "{1}" ' +
        '-OutputStem "reveal_cycle_fast"' -f $sharedScript, $moduleDir)
    $child = Start-Process powershell.exe -ArgumentList $arguments -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr

    Write-ModuleProgress -Context $ctx -Step 2 -Total 4 `
        -Stage 'wait_reveal_cycle_extractor' -Detail 'Hard timeout 60s.'
    $completed = $child.WaitForExit(60000)
    if (-not $completed) {
        try { $child.Kill() } catch {}
        Write-ModuleResult -Context $ctx -Status timeout -Blockers @('RevealCycle extractor timeout') `
            -NextAction 'Review child logs.' | Out-Null
        exit 2
    }

    $destination = Join-Path ([string]$ctx.output_dir) 'xml'
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    $sourceXml = Join-Path $probeOut 'xml'
    $xmlPath = Join-Path $destination 'RevealCycleData.xml'
    $found = $false
    if (Test-Path -LiteralPath $sourceXml) {
        $candidate = Get-ChildItem -LiteralPath $sourceXml -File -Filter '*RevealCycleData.xml' | Select-Object -First 1
        if ($null -ne $candidate) {
            Copy-Item -LiteralPath $candidate.FullName -Destination $xmlPath -Force
            $found = $true
        }
    }

    foreach ($name in @('reveal_cycle_fast.log', 'reveal_cycle_fast_last_stage.txt')) {
        $path = Join-Path $moduleDir $name
        if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination ([string]$ctx.output_dir) -Force }
    }
    foreach ($name in @('extraction_manifest.txt', 'all_game_data_xml_entries.csv')) {
        $path = Join-Path $probeOut $name
        if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination ([string]$ctx.output_dir) -Force }
    }

    Write-ModuleProgress -Context $ctx -Step 3 -Total 4 `
        -Stage 'parse_reveal_cycle_xml' -Detail ('found=' + $found)

    $rows = @()
    if ($found) {
        $document = New-Object Xml.XmlDocument
        $document.PreserveWhitespace = $false
        $document.Load($xmlPath)
        foreach ($node in @($document.DocumentElement.SelectNodes('./*'))) {
            if ($null -eq $node) { continue }
            $row = [ordered]@{}
            foreach ($attribute in @($node.Attributes)) {
                if ($null -ne $attribute) { $row[[string]$attribute.LocalName] = [string]$attribute.Value }
            }
            if ($row.Count -gt 0) { $rows += [pscustomobject]$row }
        }
    }

    $allPath = Join-Path ([string]$ctx.output_dir) 'reveal-cycle-all.tsv'
    if ($rows.Count -gt 0) {
        $rows | Export-Csv -LiteralPath $allPath -Delimiter "`t" -NoTypeInformation -Encoding UTF8
    }

    $nightCandidates = @($rows | Where-Object {
        [uint64]$reveal = 0
        [uint64]::TryParse([string]$_.RevealIngametime, [ref]$reveal) -and
        $reveal -eq 23
    })
    $nightPath = Join-Path ([string]$ctx.output_dir) 'reveal-cycle-23h-candidates.json'
    Write-JsonUtf8NoBom -Path $nightPath -Value ([ordered]@{
        observed_target_cid = 143
        observed_first_presence_window = '22:51:13-23:01:46'
        candidate_count = $nightCandidates.Count
        candidates = $nightCandidates
    }) -Depth 10

    Write-ModuleProgress -Context $ctx -Step 4 -Total 4 `
        -Stage 'finalize_reveal_cycle' `
        -Detail ('rows=' + $rows.Count + '; near_23h=' + $nightCandidates.Count)

    $outputs = @($stdout, $stderr, $xmlPath, $nightPath)
    if (Test-Path -LiteralPath $allPath) { $outputs += $allPath }
    Write-ModuleResult -Context $ctx `
        -Status $(if ($found -and $rows.Count -gt 0) { 'success' } elseif ($found) { 'partial' } else { 'blocked' }) `
        -Metrics @{ xml=$found; rows=$rows.Count; reveal_near_23h=$nightCandidates.Count; child_exit=$child.ExitCode } `
        -Outputs $outputs `
        -Blockers $(if ($found -and $rows.Count -gt 0) { @() } elseif ($found) { @('RevealCycleData rows not parsed') } else { @('RevealCycleData.xml') }) `
        -NextAction 'Bind the CID 143 MonsterSpawnBase TableKey_RevealCycle to one extracted row without broad Actor enumeration.' `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch {
    $message = Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status error -Blockers @($message) `
        -NextAction 'Review RevealCycle fast logs.' | Out-Null
    exit 1
}
