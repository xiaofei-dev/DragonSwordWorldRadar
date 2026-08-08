# Windows PowerShell 5.1-compatible module API.

function Import-ModuleContext {
    param([Parameter(Mandatory=$true)][string]$ContextPath)
    if (-not (Test-Path -LiteralPath $ContextPath -PathType Leaf)) {
        throw "Module context does not exist: $ContextPath"
    }
    $context = Read-JsonUtf8Strict -Path $ContextPath
    foreach ($name in @('root','module_id','run_id','trigger','output_dir','run_output_dir','result_path','suite_path')) {
        if ([string]::IsNullOrWhiteSpace([string]$context.$name)) { throw "Module context is missing $name" }
    }
    New-Item -ItemType Directory -Force -Path $context.output_dir,$context.run_output_dir | Out-Null
    return $context
}

function Write-ModuleLog {
    param(
        [Parameter(Mandatory=$true)][object]$Context,
        [Parameter(Mandatory=$true)][string]$Event,
        [string]$Message = ''
    )
    $line = ('[{0}] [{1}] module={2} trigger={3} {4}' -f
        (Get-Date).ToUniversalTime().ToString('o'),$Event,$Context.module_id,$Context.trigger,$Message)
    $line | Add-Content -LiteralPath (Join-Path $Context.run_output_dir 'module.log') -Encoding UTF8
    $line | Add-Content -LiteralPath (Join-Path $Context.output_dir 'module.log') -Encoding UTF8
}


function Write-ModuleProgress {
    param(
        [Parameter(Mandatory=$true)]$Context,
        [Parameter(Mandatory=$true)][int]$Step,
        [Parameter(Mandatory=$true)][int]$Total,
        [Parameter(Mandatory=$true)][string]$Stage,
        [string]$Detail = '',
        [ValidateSet('running','complete','warning')]
        [string]$Status = 'running'
    )

    $safeTotal = [Math]::Max(1,$Total)
    $safeStep = [Math]::Max(0,[Math]::Min($Step,$safeTotal))
    $percent = [int][Math]::Floor(($safeStep * 100.0) / $safeTotal)

    $progress = [ordered]@{
        schema_version = 1
        module_id = [string]$Context.module_id
        run_id = [string]$Context.run_id
        trigger = [string]$Context.trigger
        framework_version = [string]$Context.framework_version
        status = $Status
        step = $safeStep
        total = $safeTotal
        percent = $percent
        stage = $Stage
        detail = $Detail
        updated_utc = (Get-Date).ToUniversalTime().ToString('o')
    }

    $currentPath = Join-Path ([string]$Context.output_dir) 'progress.json'
    $runPath = Join-Path ([string]$Context.run_output_dir) 'progress.json'

    Write-JsonUtf8NoBom -Path $currentPath -Value $progress -Depth 8
    Write-JsonUtf8NoBom -Path $runPath -Value $progress -Depth 8

    $line = ('[{0}] step={1}/{2} percent={3} status={4} stage={5} detail={6}' -f
        $progress.updated_utc,
        $safeStep,
        $safeTotal,
        $percent,
        $Status,
        $Stage,
        $Detail)

    $line | Add-Content `
        -LiteralPath (Join-Path ([string]$Context.output_dir) 'progress.log') `
        -Encoding UTF8
    $line | Add-Content `
        -LiteralPath (Join-Path ([string]$Context.run_output_dir) 'progress.log') `
        -Encoding UTF8
}

function Write-ModuleResult {
    param(
        [Parameter(Mandatory=$true)]$Context,
        [Parameter(Mandatory=$true)]
        [ValidateSet('success','partial','blocked','error','skipped')]
        [string]$Status,
        $Metrics = $null,
        $Outputs = $null,
        $Blockers = $null,
        [string]$NextAction = '',
        [string]$Fingerprint = ''
    )

    if ($null -eq $Metrics) { $Metrics = @{} }

    $normalizedOutputs = @()
    foreach ($item in @($Outputs)) {
        if ($null -ne $item) { $normalizedOutputs += [string]$item }
    }

    $normalizedBlockers = @()
    foreach ($item in @($Blockers)) {
        if ($null -ne $item) { $normalizedBlockers += [string]$item }
    }

    $result = [ordered]@{
        schema_version = 1
        module_id = [string]$Context.module_id
        run_id = [string]$Context.run_id
        trigger = [string]$Context.trigger
        framework_version = [string]$Context.framework_version
        status = $Status
        generated_utc = (Get-Date).ToUniversalTime().ToString('o')
        fingerprint = $Fingerprint
        metrics = $Metrics
        outputs = $normalizedOutputs
        blockers = $normalizedBlockers
        next_action = $NextAction
    }

    Write-JsonUtf8NoBom -Path ([string]$Context.result_path) -Value $result -Depth 12
    Write-JsonUtf8NoBom -Path (Join-Path ([string]$Context.run_output_dir) 'result.json') -Value $result -Depth 12

    Write-ModuleLog -Context $Context -Event 'MODULE_RESULT' `
        -Message ('status={0}; blockers={1}' -f $Status,$normalizedBlockers.Count)

    try {
        Write-ModuleProgress `
            -Context $Context `
            -Step 1 `
            -Total 1 `
            -Stage 'module_result_written' `
            -Detail ('status='+$Status+'; blockers='+$normalizedBlockers.Count) `
            -Status 'complete'
    }
    catch {
        # Result creation must not fail because progress reporting failed.
    }
}

function Resolve-GameLayout {
    param(
        [Parameter(Mandatory=$true)][string]$ProbeRoot,
        [Parameter(Mandatory=$true)][object]$Suite
    )
    $current = Get-Item -LiteralPath $ProbeRoot -Force
    $win64 = $null
    while ($null -ne $current) {
        $candidate = Join-Path $current.FullName ([string]$Suite.game.executable_name)
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { $win64 = $current.FullName; break }
        $current = $current.Parent
    }
    if ([string]::IsNullOrWhiteSpace($win64)) {
        throw ('Could not locate {0} above the probe directory.' -f $Suite.game.executable_name)
    }
    $dsRoot = (Get-Item -LiteralPath $win64).Parent.Parent.FullName
    $gameRoot = (Get-Item -LiteralPath $dsRoot).Parent.FullName
    $exePath = Join-Path $win64 ([string]$Suite.game.executable_name)
    $pakPath = Join-Path $gameRoot ([string]$Suite.game.pak_relative_path)
    return [pscustomobject]@{
        win64 = $win64
        ds_root = $dsRoot
        game_root = $gameRoot
        exe_path = $exePath
        pak_path = $pakPath
        mods_root = Join-Path $win64 'Mods'
        saved_root = Join-Path $dsRoot 'Saved'
    }
}

function Get-BuildFingerprint {
    param([Parameter(Mandatory=$true)][object]$Layout)
    $parts = New-Object System.Collections.Generic.List[string]
    foreach ($path in @($Layout.exe_path,$Layout.pak_path)) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $item = Get-Item -LiteralPath $path -Force
            $parts.Add(('{0}|{1}|{2}' -f $item.Name,$item.Length,$item.LastWriteTimeUtc.Ticks))
        } else { $parts.Add(('missing|{0}' -f $path)) }
    }
    $text = $parts -join '||'
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($text)
        return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
    } finally { $sha.Dispose() }
}

function Copy-TreeBounded {
    param(
        [Parameter(Mandatory=$true)][string]$Source,
        [Parameter(Mandatory=$true)][string]$Destination,
        [int64]$MaximumBytes = 536870912
    )
    if (-not (Test-Path -LiteralPath $Source)) { return 0L }
    $total = 0L
    $sourceItem = Get-Item -LiteralPath $Source -Force
    if (-not $sourceItem.PSIsContainer) {
        if ($sourceItem.Length -le $MaximumBytes) {
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
            Copy-Item -LiteralPath $Source -Destination $Destination -Force
            return [int64]$sourceItem.Length
        }
        return 0L
    }
    foreach ($file in Get-ChildItem -LiteralPath $Source -Recurse -File -Force -ErrorAction SilentlyContinue) {
        if ($total + $file.Length -gt $MaximumBytes) { break }
        $relative = $file.FullName.Substring($sourceItem.FullName.Length).TrimStart('\','/')
        $target = Join-Path $Destination $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target -Force
        $total += $file.Length
    }
    return $total
}
