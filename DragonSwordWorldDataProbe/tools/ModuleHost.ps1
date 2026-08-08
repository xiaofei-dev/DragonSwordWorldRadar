param(
    [Parameter(Mandatory=$true)][string]$Root,
    [Parameter(Mandatory=$true)]
    [ValidateSet('monitor_start','game_exit','collect','manual')]
    [string]$Trigger,
    [string]$RunId = ''
)

$ErrorActionPreference = 'Stop'
$toolsDir = Split-Path -Parent $MyInvocation.MyCommand.Path

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$Root = Resolve-ProbeRoot -PathValue $Root
Assert-ProbeLayout -ProbeRoot $Root

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $RunId = Get-Date -Format 'yyyyMMdd-HHmmss'
}

$suitePath = Join-Path $Root 'config\suite.json'
$suite = Read-JsonUtf8Strict -Path $suitePath
$frameworkVersion = [string]$suite.version

$profilePath = Join-Path $Root ('config\profiles\{0}.json' -f [string]$suite.active_profile)
$profile = Read-JsonUtf8Strict -Path $profilePath
$defsRaw = Read-JsonUtf8Strict -Path (Join-Path $Root 'config\modules.json')

$runRoot = Join-Path $Root ('runtime\runs\' + $RunId)
New-Item -ItemType Directory -Force -Path $runRoot | Out-Null

$hostProgressLog = Join-Path $runRoot 'host-progress.log'
$latestHostProgress = Join-Path $Root 'runtime\reports\latest-module-host-progress.json'

function Format-Elapsed {
    param([TimeSpan]$Elapsed)
    return ('{0:00}:{1:00}:{2:00}' -f
        [int][Math]::Floor($Elapsed.TotalHours),
        $Elapsed.Minutes,
        $Elapsed.Seconds)
}

function Write-HostProgress {
    param(
        [string]$Stage,
        [string]$Detail = '',
        [string]$Status = 'running',
        [int]$ModuleIndex = 0,
        [int]$ModuleCount = 0,
        [string]$ModuleId = ''
    )

    $progress = [ordered]@{
        schema_version = 1
        framework_version = $frameworkVersion
        run_id = [string]$RunId
        trigger = [string]$Trigger
        status = $Status
        stage = $Stage
        detail = $Detail
        module_index = $ModuleIndex
        module_count = $ModuleCount
        module_id = $ModuleId
        updated_utc = (Get-Date).ToUniversalTime().ToString('o')
    }

    Write-JsonUtf8NoBom -Path $latestHostProgress -Value $progress -Depth 8

    $line = ('[{0}] status={1} stage={2} module={3}/{4}:{5} detail={6}' -f
        $progress.updated_utc,
        $Status,
        $Stage,
        $ModuleIndex,
        $ModuleCount,
        $ModuleId,
        $Detail)

    $line | Add-Content -LiteralPath $hostProgressLog -Encoding UTF8
}

$moduleMutex = New-Object Threading.Mutex(
    $false,
    'Local\DragonSwordWorldDataProbe_ModuleHost_v106'
)
$mutexAcquired = $false

try {
    Write-HostProgress -Stage 'wait_collector_lock' `
        -Detail ('trigger='+$Trigger)

    Write-Host ('[ModuleHost 1/4] Waiting for collector lock. trigger={0}' -f $Trigger)

    $lockStart = Get-Date
    $lastLockMessage = Get-Date

    while (-not $mutexAcquired) {
        try {
            $mutexAcquired = $moduleMutex.WaitOne(1000)
        }
        catch [Threading.AbandonedMutexException] {
            $mutexAcquired = $true
        }

        if ($mutexAcquired) { break }

        $elapsed = (Get-Date) - $lockStart
        if ($elapsed.TotalSeconds -ge 1200) {
            throw 'Timed out waiting for another data-collection pipeline to finish.'
        }

        if (((Get-Date) - $lastLockMessage).TotalSeconds -ge 5) {
            $lastLockMessage = Get-Date
            $detail = ('another collector is active; elapsed={0}' -f
                (Format-Elapsed -Elapsed $elapsed))
            Write-Host ('  [lock wait] {0}' -f $detail)
            Write-HostProgress -Stage 'wait_collector_lock' -Detail $detail
        }
    }

    Write-Host ('[ModuleHost 2/4] Collector lock acquired after {0}.' -f
        (Format-Elapsed -Elapsed ((Get-Date) - $lockStart)))

    Write-HostProgress -Stage 'resolve_modules' `
        -Detail 'Reading profile/module definitions.'

    $definitions = @()
    foreach ($item in @($defsRaw)) {
        if ($null -ne $item) { $definitions += $item }
    }

    $allowed = @{}
    foreach ($moduleId in @($profile.external_modules)) {
        if ($null -ne $moduleId) {
            $allowed[[string]$moduleId] = $true
        }
    }

    $rows = @()
    $orderedDefinitions = @(
        $definitions |
            Sort-Object -Property `
                @{Expression={ if ($null -eq $_.order) { 9999 } else { [int]$_.order } }},
                @{Expression={ [string]$_.id }}
    )

    $eligible = @()
    foreach ($moduleDef in $orderedDefinitions) {
        $id = [string]$moduleDef.id
        if ([string]::IsNullOrWhiteSpace($id)) { continue }
        if ($moduleDef.enabled -ne $true) { continue }
        if (-not $allowed.ContainsKey($id)) { continue }

        $moduleTriggers = @()
        foreach ($triggerItem in @($moduleDef.triggers)) {
            if ($null -ne $triggerItem) {
                $moduleTriggers += [string]$triggerItem
            }
        }

        if ($moduleTriggers -notcontains $Trigger) { continue }
        $eligible += $moduleDef
    }

    $eligibleNames = @($eligible | ForEach-Object { [string]$_.id })
    Write-Host ('  eligible modules ({0}): {1}' -f
        $eligible.Count,
        (($eligibleNames -join ', ')))

    Write-HostProgress -Stage 'run_modules' `
        -Detail ('eligible='+($eligibleNames -join ',')) `
        -ModuleCount $eligible.Count

    $moduleIndex = 0

    foreach ($moduleDef in $eligible) {
        $moduleIndex++
        $id = [string]$moduleDef.id

        Write-Host ''
        Write-Host ('[Module {0}/{1}] {2}' -f
            $moduleIndex,
            $eligible.Count,
            $id)

        Write-Host ('  [1/5] Preparing module directories and context.')
        Write-HostProgress -Stage 'module_prepare' `
            -Detail 'Preparing current/run directories.' `
            -ModuleIndex $moduleIndex `
            -ModuleCount $eligible.Count `
            -ModuleId $id

        $entry = Join-Path $Root ([string]$moduleDef.entry)
        $current = Join-Path $Root ('runtime\modules\' + $id + '\current')
        $runOut = Join-Path $runRoot $id

        if ($Trigger -eq 'collect') {
            $existingResultPath = Join-Path $current 'result.json'

            if (Test-Path -LiteralPath $existingResultPath -PathType Leaf) {
                try {
                    $existingResult = Read-JsonUtf8Strict -Path $existingResultPath
                    $versionMatches = (
                        [string]$existingResult.framework_version -eq
                        $frameworkVersion
                    )
                    $fingerprintMatches = $true

                    if (-not [string]::IsNullOrWhiteSpace(
                        [string]$existingResult.fingerprint
                    )) {
                        $currentBuild = Get-StaticCacheFingerprint -ProbeRoot $Root
                        $fingerprintMatches = (
                            [string]$existingResult.fingerprint -eq
                            [string]$currentBuild.build_fingerprint
                        )
                    }

                    $statusText = [string]$existingResult.status
                    $statusReusable = @('success','partial') -contains $statusText

                    if ($versionMatches -and
                        $fingerprintMatches -and
                        $statusReusable) {
                        Write-Host ('  [2/5] Reusing current result: status={0}' -f
                            $statusText)
                        Write-Host ('  [3/5] Child process skipped.')
                        Write-Host ('  [4/5] Result fingerprint/version matched.')
                        Write-Host ('  [5/5] Module complete (reused).')

                        Write-HostProgress -Stage 'module_reused' `
                            -Detail ('status='+$statusText) `
                            -Status 'complete' `
                            -ModuleIndex $moduleIndex `
                            -ModuleCount $eligible.Count `
                            -ModuleId $id

                        $rows += [pscustomobject]@{
                            module = $id
                            status = $statusText
                            timed_out = $false
                            detail = 'reused current result'
                        }
                        continue
                    }

                    Write-Host ('  [2/5] Existing result not reusable: status={0}; version={1}; fingerprint={2}' -f
                        $statusText,
                        $versionMatches,
                        $fingerprintMatches)
                }
                catch {
                    Write-Host ('  [2/5] Existing result could not be read; module will rerun.')
                }
            }
            else {
                Write-Host ('  [2/5] No reusable current result; module will run.')
            }
        }
        else {
            Write-Host ('  [2/5] Trigger={0}; module will run.' -f $Trigger)
        }

        Remove-Item -LiteralPath $current -Recurse -Force -ErrorAction SilentlyContinue
        New-Item -ItemType Directory -Force -Path $current | Out-Null
        New-Item -ItemType Directory -Force -Path $runOut | Out-Null

        $resultPath = Join-Path $current 'result.json'
        $contextPath = Join-Path $runOut 'context.json'

        $context = [ordered]@{
            schema_version = 1
            framework_version = $frameworkVersion
            root = [string]$Root
            module_id = $id
            run_id = [string]$RunId
            trigger = [string]$Trigger
            output_dir = [string]$current
            run_output_dir = [string]$runOut
            result_path = [string]$resultPath
            suite_path = [string]$suitePath
            profile_path = [string]$profilePath
            target_profile_path = [string](
                Join-Path $Root 'config\static-targets\assault.json'
            )
        }

        Write-JsonUtf8NoBom -Path $contextPath -Value $context -Depth 8

        if (-not (Test-Path -LiteralPath $entry -PathType Leaf)) {
            Write-Host ('  [3/5] Entry missing: {0}' -f $entry)
            Write-Host ('  [4/5] No child process started.')
            Write-Host ('  [5/5] Module error.')

            $rows += [pscustomobject]@{
                module = $id
                status = 'error'
                timed_out = $false
                detail = 'entry_missing'
            }
            continue
        }

        $stdout = Join-Path $runOut 'stdout.log'
        $stderr = Join-Path $runOut 'stderr.log'
        $argumentString = (
            '-NoProfile -ExecutionPolicy Bypass -File "{0}" -ContextPath "{1}"' -f
            $entry,
            $contextPath
        )

        Write-Host ('  [3/5] Starting child process: {0}' -f
            ([IO.Path]::GetFileName($entry)))

        Write-HostProgress -Stage 'module_start_child' `
            -Detail ([IO.Path]::GetFileName($entry)) `
            -ModuleIndex $moduleIndex `
            -ModuleCount $eligible.Count `
            -ModuleId $id

        $process = Start-Process `
            -FilePath 'powershell.exe' `
            -ArgumentList $argumentString `
            -WindowStyle Hidden `
            -PassThru `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr

        $timeoutSeconds = 300
        if ($null -ne $moduleDef.timeout_seconds) {
            $timeoutSeconds = [Math]::Max(
                10,
                [int]$moduleDef.timeout_seconds
            )
        }

        $heartbeatSeconds = 5
        if ($null -ne $suite.diagnostics.module_heartbeat_seconds) {
            $heartbeatSeconds = [Math]::Max(
                2,
                [int]$suite.diagnostics.module_heartbeat_seconds
            )
        }

        Write-Host ('  [4/5] Waiting for module result. timeout={0}s' -f
            $timeoutSeconds)

        $started = Get-Date
        $lastHeartbeat = Get-Date
        $lastProgressWriteUtc = [DateTime]::MinValue
        $progressPath = Join-Path $current 'progress.json'
        $completed = $false

        while (-not $completed) {
            $completed = $process.WaitForExit(500)

            if (Test-Path -LiteralPath $progressPath -PathType Leaf) {
                try {
                    $progressInfo = Get-Item -LiteralPath $progressPath -Force

                    if ($progressInfo.LastWriteTimeUtc -gt $lastProgressWriteUtc) {
                        $lastProgressWriteUtc = $progressInfo.LastWriteTimeUtc
                        $progress = Read-JsonUtf8Strict -Path $progressPath

                        Write-Host ('    [module step {0}/{1} - {2}%] {3} :: {4}' -f
                            $progress.step,
                            $progress.total,
                            $progress.percent,
                            $progress.stage,
                            $progress.detail)

                        Write-HostProgress -Stage 'module_child_progress' `
                            -Detail (
                                'step='+$progress.step+'/'+$progress.total+
                                '; stage='+$progress.stage+
                                '; detail='+$progress.detail
                            ) `
                            -ModuleIndex $moduleIndex `
                            -ModuleCount $eligible.Count `
                            -ModuleId $id
                    }
                }
                catch {
                    # A partially replaced progress file is retried next poll.
                }
            }

            $elapsed = (Get-Date) - $started

            if (-not $completed -and
                $elapsed.TotalSeconds -ge $timeoutSeconds) {
                try { $process.Kill() } catch {}
                try { [void]$process.WaitForExit(5000) } catch {}
                break
            }

            if (-not $completed -and
                ((Get-Date) - $lastHeartbeat).TotalSeconds -ge
                    $heartbeatSeconds) {
                $lastHeartbeat = Get-Date
                $detail = ('running; elapsed={0}; timeout={1}s' -f
                    (Format-Elapsed -Elapsed $elapsed),
                    $timeoutSeconds)

                Write-Host ('    [heartbeat] {0}' -f $detail)
                Write-HostProgress -Stage 'module_child_running' `
                    -Detail $detail `
                    -ModuleIndex $moduleIndex `
                    -ModuleCount $eligible.Count `
                    -ModuleId $id
            }
        }

        if ($completed) {
            try { $process.WaitForExit() } catch {}
        }

        $status = if ($completed) { 'error' } else { 'timeout' }
        $detail = ''

        if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
            try {
                $moduleResult = Read-JsonUtf8Strict -Path $resultPath
                $status = [string]$moduleResult.status
                $detail = [string]$moduleResult.next_action
            }
            catch {
                $detail = Get-ExceptionSummary -ErrorObject $_
            }
        }
        elseif (Test-Path -LiteralPath $stderr -PathType Leaf) {
            $detail = (
                (
                    Get-Content -LiteralPath $stderr `
                        -ErrorAction SilentlyContinue |
                        Select-Object -First 20
                ) -join ' '
            )
        }

        if ([string]::IsNullOrWhiteSpace($detail)) {
            $detail = 'no additional detail'
        }

        Write-Host ('  [5/5] Module finished: status={0}; detail={1}' -f
            $status,
            $detail)

        Write-HostProgress -Stage 'module_complete' `
            -Detail ('status='+$status+'; '+$detail) `
            -Status 'complete' `
            -ModuleIndex $moduleIndex `
            -ModuleCount $eligible.Count `
            -ModuleId $id

        $rows += [pscustomobject]@{
            module = $id
            status = $status
            timed_out = (-not $completed)
            detail = $detail
        }
    }

    Write-Host ''
    Write-Host '[ModuleHost 3/4] Writing host summary.'

    $summary = [ordered]@{
        schema_version = 1
        framework_version = $frameworkVersion
        run_id = [string]$RunId
        trigger = [string]$Trigger
        generated_utc = (Get-Date).ToUniversalTime().ToString('o')
        modules = @($rows)
    }

    Write-JsonUtf8NoBom `
        -Path (Join-Path $runRoot 'host-summary.json') `
        -Value $summary `
        -Depth 10

    Write-JsonUtf8NoBom `
        -Path (Join-Path $Root 'runtime\reports\latest-module-host-summary.json') `
        -Value $summary `
        -Depth 10

    Write-HostProgress -Stage 'host_complete' `
        -Detail ('modules='+$rows.Count) `
        -Status 'complete' `
        -ModuleCount $eligible.Count

    Write-Host '[ModuleHost 4/4] Complete.'
}
finally {
    if ($mutexAcquired) {
        try { $moduleMutex.ReleaseMutex() } catch {}
    }
    $moduleMutex.Dispose()
}
