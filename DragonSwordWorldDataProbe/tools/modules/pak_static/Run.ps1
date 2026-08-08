param(
    [Parameter(Mandatory=$true)]
    [string]$ContextPath
)

$ErrorActionPreference='Stop'

$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$ctx=Import-ModuleContext -ContextPath $ContextPath

try{
    Write-Host '[PAK 1/4] Checking static cache...'

    $fingerprintInfo=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)
    $cacheRoot=Get-StaticCacheRoot -ProbeRoot ([string]$ctx.root) -Fingerprint ([string]$fingerprintInfo.fingerprint)
    $cacheStatic=Join-Path $cacheRoot 'assault-static'
    $cacheSummary=Join-Path $cacheRoot 'pak-static-summary.json'
    $cacheReady=Join-Path $cacheRoot 'ready.marker'

    $dest=Join-Path ([string]$ctx.output_dir) 'static'
    Remove-Item -LiteralPath $dest -Recurse -Force -ErrorAction SilentlyContinue

    if((Test-Path -LiteralPath $cacheReady -PathType Leaf) -and
       (Test-Path -LiteralPath $cacheStatic -PathType Container))
    {
        Write-Host '[PAK 2/4] Reusing cached PAK extraction.'
        Copy-Item -LiteralPath $cacheStatic -Destination $dest -Recurse -Force

        $cachedSummary=$null
        if(Test-Path -LiteralPath $cacheSummary){
            $cachedSummary=Read-JsonUtf8Strict -Path $cacheSummary
        }

        $cachedMissing=@()
        $cachedExtracted=0
        if($null-ne$cachedSummary){
            $cachedExtracted=[int]$cachedSummary.extracted_count
            foreach($name in @($cachedSummary.missing_critical_files)){
                if($null-ne$name){$cachedMissing+=[string]$name}
            }
        }

        $cachedStatus=
            if($cachedMissing.Count-eq0-and$cachedExtracted-gt0){'success'}
            elseif($cachedExtracted-gt0){'partial'}
            else{'blocked'}

        Write-ModuleResult -Context $ctx -Status $cachedStatus `
            -Metrics @{
                cache_hit=$true
                cache_schema=[string]$fingerprintInfo.cache_schema
                cache_fingerprint=[string]$fingerprintInfo.fingerprint
                extracted=$cachedExtracted
                missing_critical=$cachedMissing.Count
            } `
            -Outputs @($dest,$cacheSummary) `
            -Blockers $cachedMissing `
            -NextAction (
                if($cachedMissing.Count-eq0){
                    'Static cache reused; run assault and Mole analyzers.'
                }else{
                    'Partial static cache reused; decoder evidence remains available for missing critical XML.'
                }
            ) `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 0
    }

    Write-Host '[PAK 2/4] Cache miss. Running proven PakReaderCore + ooz extractor...'

    $legacyOut=Join-Path $moduleDir 'assault_static_probe'
    $legacyLog=Join-Path $moduleDir 'assault_static_probe.log'
    $legacyStage=Join-Path $moduleDir 'assault_static_probe_last_stage.txt'

    Remove-Item -LiteralPath $legacyOut -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $legacyLog -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $legacyStage -Force -ErrorAction SilentlyContinue

    $scriptPath=Join-Path $moduleDir 'AssaultStaticProbe.Proven.ps1'
    $childStdout=Join-Path ([string]$ctx.output_dir) 'pak-verification-child.stdout.log'
    $childStderr=Join-Path ([string]$ctx.output_dir) 'pak-verification-child.stderr.log'
    $args='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "{0}"' -f $scriptPath

    Write-Host '[PAK manual] Starting optional verifier (hard timeout: 180s)...'

    $child=Start-Process `
        -FilePath 'powershell.exe' `
        -ArgumentList $args `
        -WindowStyle Hidden `
        -PassThru `
        -RedirectStandardOutput $childStdout `
        -RedirectStandardError $childStderr

    $childCompleted=$child.WaitForExit(180000)

    if(-not$childCompleted){
        try{$child.Kill()}catch{}

        Write-ModuleResult -Context $ctx -Status 'timeout' `
            -Metrics @{cache_hit=$false;hard_timeout_seconds=180} `
            -Outputs @($childStdout,$childStderr) `
            -Blockers @('Optional PAK verifier exceeded 180 seconds and was terminated.') `
            -NextAction 'Do not use PAK verification as a production dependency. Runtime DUnexpectedMissionTable remains primary.' `
            -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
        exit 2
    }

    $childExit=$child.ExitCode

    if(Test-Path -LiteralPath $legacyOut -PathType Container){
        Copy-Item -LiteralPath $legacyOut -Destination $dest -Recurse -Force
    }

    foreach($name in @('assault_static_probe.log','assault_static_probe_last_stage.txt')){
        $source=Join-Path $moduleDir $name
        if(Test-Path -LiteralPath $source -PathType Leaf){
            Copy-Item -LiteralPath $source -Destination ([string]$ctx.output_dir) -Force
        }
    }

    Write-Host '[PAK 3/4] Validating extracted XML set...'

    $xmlDir=Join-Path $dest 'xml'
    $manifest=Join-Path $dest 'extraction_manifest.txt'
    $xmlFiles=@()

    if(Test-Path -LiteralPath $xmlDir -PathType Container){
        $xmlFiles=@(Get-ChildItem -LiteralPath $xmlDir -File -Filter '*.xml')
    }

    $critical=@(
        'UnexpectedMissionWorldData.xml',
        'UnexpectedMissionPlaceData.xml',
        'UnexpectedMissionKindData.xml',
        'MiniGameData.xml'
    )

    $successNames=@{}
    if(Test-Path -LiteralPath $manifest -PathType Leaf){
        foreach($line in Get-Content -LiteralPath $manifest){
            if($line -match '^OK_[^|]+\|\s*/([^|]+\.xml)\s*\|'){
                $successNames[$Matches[1].Trim()]=$true
            }
        }
    }

    $missing=@()
    foreach($name in $critical){
        if(-not$successNames.ContainsKey($name)){
            $missing+=$name
        }
    }

    $summary=[ordered]@{
        schema_version=1
        cache_schema=[string]$fingerprintInfo.cache_schema
        build_fingerprint=[string]$fingerprintInfo.build_fingerprint
        cache_fingerprint=[string]$fingerprintInfo.fingerprint
        backend='proven_PakReaderCore_ooz'
        child_exit_code=$childExit
        extracted_count=$xmlFiles.Count
        critical_files=$critical
        missing_critical_files=$missing
        generated_utc=(Get-Date).ToUniversalTime().ToString('o')
    }

    $summaryPath=Join-Path ([string]$ctx.output_dir) 'extraction-summary.json'
    Write-JsonUtf8NoBom -Path $summaryPath -Value $summary -Depth 8

    Write-Host '[PAK 4/4] Updating shared static cache...'

    New-Item -ItemType Directory -Force -Path $cacheRoot | Out-Null
    Remove-Item -LiteralPath $cacheStatic -Recurse -Force -ErrorAction SilentlyContinue

    if(Test-Path -LiteralPath $dest -PathType Container){
        Copy-Item -LiteralPath $dest -Destination $cacheStatic -Recurse -Force
    }

    Write-JsonUtf8NoBom -Path $cacheSummary -Value $summary -Depth 8

    # Cache support-table extraction even when critical Kind/Place are still missing.
    # Missing critical entries are tracked separately and can be refined later.
    [IO.File]::WriteAllText(
        $cacheReady,
        ([string]$fingerprintInfo.fingerprint),
        (New-Object Text.UTF8Encoding -ArgumentList $false)
    )

    $status=
        if($missing.Count-eq0){'success'}
        elseif($xmlFiles.Count-gt0){'partial'}
        else{'blocked'}

    $next=
        if($missing.Count-eq0){
            'Run assault catalog and mole discovery.'
        }else{
            'Support-table cache is ready. Continue exact Kind/Place decoder work without rescanning the PAK.'
        }

    Write-ModuleResult -Context $ctx -Status $status `
        -Metrics @{
            cache_hit=$false
            build_fingerprint=[string]$fingerprintInfo.build_fingerprint
            cache_fingerprint=[string]$fingerprintInfo.fingerprint
            extracted=$xmlFiles.Count
            missing_critical=$missing.Count
            child_exit=$childExit
        } `
        -Outputs @($dest,$summaryPath,$cacheStatic) `
        -Blockers $missing `
        -NextAction $next `
        -Fingerprint ([string]$fingerprintInfo.build_fingerprint) | Out-Null
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $message
    Write-ModuleResult -Context $ctx -Status 'error' `
        -Blockers @($message) `
        -NextAction 'Review pak_static module log.' | Out-Null
    exit 1
}
