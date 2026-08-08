param(
    [Parameter(Mandatory=$true)][string]$Root
)

$ErrorActionPreference='Stop'

$toolsDir=Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$Root=Resolve-ProbeRoot -PathValue $Root
Assert-ProbeLayout -ProbeRoot $Root

$suite=Read-JsonUtf8Strict -Path (Join-Path $Root 'config\suite.json')
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$runId='collect-'+$stamp
$collectionErrors=@()
$diagnosticStart=Get-Date

$progressPath=Join-Path $Root 'runtime\reports\diagnostics-progress.json'
$progressLog=Join-Path $Root 'runtime\reports\diagnostics-progress.log'
$stageCount=12

function Format-Elapsed {
    param([TimeSpan]$Elapsed)
    return ('{0:00}:{1:00}:{2:00}' -f
        [int][Math]::Floor($Elapsed.TotalHours),
        $Elapsed.Minutes,
        $Elapsed.Seconds)
}

function Write-DiagnosticsStage {
    param(
        [Parameter(Mandatory=$true)][int]$Index,
        [Parameter(Mandatory=$true)][string]$Stage,
        [string]$Detail='',
        [ValidateSet('running','waiting','complete','skipped','warning','error')]
        [string]$Status='running'
    )

    $elapsed=(Get-Date)-$diagnosticStart
    $progress=[ordered]@{
        schema_version=1
        probe_version='1.0.34'
        run_id=$runId
        stage_index=$Index
        stage_count=$stageCount
        stage=$Stage
        status=$Status
        detail=$Detail
        elapsed_seconds=[int][Math]::Floor($elapsed.TotalSeconds)
        updated_utc=(Get-Date).ToUniversalTime().ToString('o')
    }

    Write-JsonUtf8NoBom -Path $progressPath -Value $progress -Depth 8

    $line=('[{0}] [{1}/{2}] status={3} stage={4} elapsed={5} detail={6}' -f
        $progress.updated_utc,
        $Index,
        $stageCount,
        $Status,
        $Stage,
        (Format-Elapsed -Elapsed $elapsed),
        $Detail)

    $line|Add-Content -LiteralPath $progressLog -Encoding UTF8

    Write-Host ('[Diagnostics {0}/{1}] {2} [{3}]' -f
        $Index,
        $stageCount,
        $Stage,
        $Status)

    if(-not[string]::IsNullOrWhiteSpace($Detail)){
        Write-Host ('  {0}' -f $Detail)
    }
}

function Read-KeyValueFile {
    param([string]$Path)

    $result=@{}
    if(-not(Test-Path -LiteralPath $Path -PathType Leaf)){
        return $result
    }

    try{
        foreach($line in Get-Content -LiteralPath $Path -ErrorAction Stop){
            if($line -match '^([^=]+)=(.*)$'){
                $result[$Matches[1]]=$Matches[2]
            }
        }
    }
    catch{}

    return $result
}

function Get-RuntimeProbeDetail {
    $pending=Read-KeyValueFile -Path (
        Join-Path $Root 'runtime\state\pending-step.txt'
    )

    if($pending.Count -gt 0){
        return ('runtime probe={0}; step={1}; phase={2}; pass={3}; index={4}' -f
            [string]$pending.probe,
            [string]$pending.step_name,
            [string]$pending.phase,
            [string]$pending.pass,
            [string]$pending.index)
    }

    $statusPath=Join-Path $Root 'runtime\reports\probe-status.txt'
    if(Test-Path -LiteralPath $statusPath -PathType Leaf){
        try{
            $line=Get-Content -LiteralPath $statusPath |
                Where-Object {$_ -like 'current_probe=*'} |
                Select-Object -First 1

            if($line){
                return ('runtime '+$line)
            }
        }
        catch{}
    }

    return 'runtime step is between scheduled calls'
}

function Get-LastMonitorLine {
    $path=Join-Path $Root 'runtime\monitor\monitor.log'
    if(-not(Test-Path -LiteralPath $path -PathType Leaf)){
        return 'monitor log not available'
    }

    try{
        return [string](Get-Content -LiteralPath $path -Tail 1)
    }
    catch{
        return 'monitor log could not be read'
    }
}

function Wait-ForGameExit {
    $pollSeconds=2
    $statusSeconds=5

    if($null-ne$suite.diagnostics.game_exit_poll_seconds){
        $pollSeconds=[Math]::Max(
            1,
            [int]$suite.diagnostics.game_exit_poll_seconds
        )
    }

    if($null-ne$suite.diagnostics.wait_status_seconds){
        $statusSeconds=[Math]::Max(
            2,
            [int]$suite.diagnostics.wait_status_seconds
        )
    }

    $processes=@(Get-GameProcessesCompat)
    if($processes.Count -eq 0){
        Write-DiagnosticsStage `
            -Index 2 `
            -Stage 'wait_for_game_exit' `
            -Detail 'Game is already closed; no wait required.' `
            -Status 'skipped'

        return [pscustomobject]@{
            Waited=$false
            Seconds=0
            InitialPids=@()
        }
    }

    $initialPids=@($processes|ForEach-Object{[int]$_.Id})
    $waitStart=Get-Date
    $lastStatus=[DateTime]::MinValue

    Write-DiagnosticsStage `
        -Index 2 `
        -Stage 'wait_for_game_exit' `
        -Detail ('Game detected. pids='+($initialPids -join ',')+
            '. Close the game normally; diagnostics will continue automatically.') `
        -Status 'waiting'

    while($true){
        $processes=@(Get-GameProcessesCompat)

        if($processes.Count -eq 0){
            break
        }

        if(((Get-Date)-$lastStatus).TotalSeconds -ge $statusSeconds){
            $lastStatus=Get-Date
            $elapsed=(Get-Date)-$waitStart
            $pids=@($processes|ForEach-Object{[int]$_.Id})
            $runtime=Get-RuntimeProbeDetail

            Write-Host ('  [game wait {0}] pids={1}; {2}' -f
                (Format-Elapsed -Elapsed $elapsed),
                ($pids -join ','),
                $runtime)

            Write-DiagnosticsStage `
                -Index 2 `
                -Stage 'wait_for_game_exit' `
                -Detail (
                    'still running; elapsed='+
                    (Format-Elapsed -Elapsed $elapsed)+
                    '; pids='+($pids -join ',')+
                    '; '+$runtime
                ) `
                -Status 'waiting'
        }

        Start-Sleep -Seconds $pollSeconds
    }

    $seconds=[int][Math]::Floor(((Get-Date)-$waitStart).TotalSeconds)

    Write-DiagnosticsStage `
        -Index 2 `
        -Stage 'wait_for_game_exit' `
        -Detail ('Game exit detected after '+$seconds+' seconds.') `
        -Status 'complete'

    return [pscustomobject]@{
        Waited=$true
        Seconds=$seconds
        InitialPids=$initialPids
    }
}

function Wait-ForAutomaticMonitor {
    param([bool]$GameWasObserved)

    if(-not$GameWasObserved){
        Write-DiagnosticsStage `
            -Index 3 `
            -Stage 'wait_for_automatic_monitor' `
            -Detail 'No active game was observed by this diagnostic run.' `
            -Status 'skipped'

        return 0
    }

    if($suite.diagnostics.wait_for_automatic_monitor -ne $true){
        Write-DiagnosticsStage `
            -Index 3 `
            -Stage 'wait_for_automatic_monitor' `
            -Detail 'Automatic monitor wait is disabled by config.' `
            -Status 'skipped'

        return 0
    }

    $mutex=$null

    try{
        $mutex=[Threading.Mutex]::OpenExisting(
            'Local\DragonSwordWorldDataProbe_Modular_v101'
        )
    }
    catch [Threading.WaitHandleCannotBeOpenedException]{
        $mutex=$null
    }
    catch{
        $mutex=$null
    }

    if($null-eq$mutex){
        $fallbackSeconds=3
        if($null-ne$suite.diagnostics.post_exit_fallback_grace_seconds){
            $fallbackSeconds=[Math]::Max(
                0,
                [int]$suite.diagnostics.post_exit_fallback_grace_seconds
            )
        }

        Write-DiagnosticsStage `
            -Index 3 `
            -Stage 'wait_for_automatic_monitor' `
            -Detail (
                'No active monitor mutex found; waiting '+
                $fallbackSeconds+
                ' seconds for file flush before local collection.'
            ) `
            -Status 'waiting'

        if($fallbackSeconds -gt 0){
            Start-Sleep -Seconds $fallbackSeconds
        }

        Write-DiagnosticsStage `
            -Index 3 `
            -Stage 'wait_for_automatic_monitor' `
            -Detail 'Fallback grace completed.' `
            -Status 'complete'

        return $fallbackSeconds
    }

    $timeoutMinutes=30
    if($null-ne$suite.diagnostics.automatic_monitor_wait_timeout_minutes){
        $timeoutMinutes=[Math]::Max(
            1,
            [int]$suite.diagnostics.automatic_monitor_wait_timeout_minutes
        )
    }

    $waitStart=Get-Date
    $lastStatus=[DateTime]::MinValue
    $acquired=$false

    Write-DiagnosticsStage `
        -Index 3 `
        -Stage 'wait_for_automatic_monitor' `
        -Detail (
            'Automatic monitor is active. Waiting for its post-exit module collection.'
        ) `
        -Status 'waiting'

    try{
        while(-not$acquired){
            try{
                $acquired=$mutex.WaitOne(1000)
            }
            catch [Threading.AbandonedMutexException]{
                $acquired=$true
            }

            if($acquired){break}

            $elapsed=(Get-Date)-$waitStart

            if($elapsed.TotalMinutes -ge $timeoutMinutes){
                throw (
                    'Timed out waiting for automatic monitor after '+
                    $timeoutMinutes+
                    ' minutes.'
                )
            }

            if(((Get-Date)-$lastStatus).TotalSeconds -ge 5){
                $lastStatus=Get-Date
                $monitorLine=Get-LastMonitorLine

                Write-Host ('  [monitor wait {0}] {1}' -f
                    (Format-Elapsed -Elapsed $elapsed),
                    $monitorLine)

                Write-DiagnosticsStage `
                    -Index 3 `
                    -Stage 'wait_for_automatic_monitor' `
                    -Detail (
                        'elapsed='+
                        (Format-Elapsed -Elapsed $elapsed)+
                        '; last_monitor_line='+$monitorLine
                    ) `
                    -Status 'waiting'
            }
        }
    }
    finally{
        if($acquired){
            try{$mutex.ReleaseMutex()}catch{}
        }
        $mutex.Dispose()
    }

    $seconds=[int][Math]::Floor(((Get-Date)-$waitStart).TotalSeconds)

    Write-DiagnosticsStage `
        -Index 3 `
        -Stage 'wait_for_automatic_monitor' `
        -Detail (
            'Automatic monitor completed after '+
            $seconds+
            ' seconds. Local collect will reuse valid results.'
        ) `
        -Status 'complete'

    return $seconds
}

Write-DiagnosticsStage `
    -Index 1 `
    -Stage 'initialize' `
    -Detail ('version=1.0.34; run_id='+$runId) `
    -Status 'complete'

$gameRunningAtStart=@(Get-GameProcessesCompat).Count -gt 0
$waitResult=Wait-ForGameExit
$monitorWaitSeconds=Wait-ForAutomaticMonitor `
    -GameWasObserved ([bool]$waitResult.Waited)

Write-DiagnosticsStage `
    -Index 4 `
    -Stage 'collect_external_modules' `
    -Detail (
        'Starting ModuleHost collect. Valid game-exit results will be reused.'
    ) `
    -Status 'running'

try{
    & (Join-Path $toolsDir 'ModuleHost.ps1') `
        -Root ([string]$Root) `
        -Trigger 'collect' `
        -RunId ([string]$runId)
}
catch{
    $message='module_host: '+(Get-ExceptionSummary -ErrorObject $_)
    $collectionErrors+=$message

    Write-DiagnosticsStage `
        -Index 4 `
        -Stage 'collect_external_modules' `
        -Detail $message `
        -Status 'warning'
}

Write-DiagnosticsStage `
    -Index 4 `
    -Stage 'collect_external_modules' `
    -Detail 'Module collection/reuse phase finished.' `
    -Status 'complete'

$stage=Join-Path (
    [IO.Path]::GetTempPath()
) ('DSWDP-'+[Guid]::NewGuid().ToString('N'))

Write-DiagnosticsStage `
    -Index 5 `
    -Stage 'create_staging_directory' `
    -Detail $stage `
    -Status 'running'

New-Item -ItemType Directory -Force -Path $stage|Out-Null

Write-DiagnosticsStage `
    -Index 5 `
    -Stage 'create_staging_directory' `
    -Detail 'Temporary staging directory created.' `
    -Status 'complete'

$outputZip=Join-Path $Root (
    'runtime\reports\DragonSwordWorldDataProbe-Modular-Diagnostics-'+
    $stamp+
    '.zip'
)

$copiedBytes=[int64]0

try{
    $trees=@(
        @('config','config'),
        @('docs','docs'),
        @('metadata','metadata'),
        @('runtime\logs','runtime\logs'),
        @('runtime\monitor','runtime\monitor'),
        @('runtime\state','runtime\state'),
        @('runtime\reports','runtime\reports'),
        @('runtime\modules','runtime\modules'),
        @('runtime\runs','runtime\runs')
    )

    $treeIndex=0

    foreach($pair in $trees){
        $treeIndex++

        Write-DiagnosticsStage `
            -Index 6 `
            -Stage 'copy_probe_data' `
            -Detail (
                'tree '+
                $treeIndex+
                '/'+
                $trees.Count+
                ': '+
                [string]$pair[0]
            ) `
            -Status 'running'

        try{
            $copiedBytes+=Copy-TreeBounded `
                -Source (Join-Path $Root ([string]$pair[0])) `
                -Destination (Join-Path $stage ([string]$pair[1])) `
                -MaximumBytes (
                    [int64]$suite.diagnostics.maximum_zip_input_bytes
                )
        }
        catch{
            $collectionErrors+=(
                [string]$pair[0]+
                ': '+
                (Get-ExceptionSummary -ErrorObject $_)
            )
        }
    }

    Write-DiagnosticsStage `
        -Index 6 `
        -Stage 'copy_probe_data' `
        -Detail ('copied_bytes='+$copiedBytes) `
        -Status 'complete'

    $layout=$null

    Write-DiagnosticsStage `
        -Index 7 `
        -Stage 'resolve_game_layout' `
        -Detail 'Resolving Win64, Mods and Saved directories.' `
        -Status 'running'

    try{
        $layout=Resolve-GameLayout -ProbeRoot $Root -Suite $suite

        Write-DiagnosticsStage `
            -Index 7 `
            -Stage 'resolve_game_layout' `
            -Detail ('win64='+[string]$layout.win64) `
            -Status 'complete'
    }
    catch{
        $message='game_layout: '+(Get-ExceptionSummary -ErrorObject $_)
        $collectionErrors+=$message

        Write-DiagnosticsStage `
            -Index 7 `
            -Stage 'resolve_game_layout' `
            -Detail $message `
            -Status 'warning'
    }

    $ue4ssCopied=0

    Write-DiagnosticsStage `
        -Index 8 `
        -Stage 'copy_ue4ss_files' `
        -Detail 'Copying UE4SS.log, settings and mods.txt when present.' `
        -Status 'running'

    if($null-ne$layout){
        try{
            $ue4ssDir=Join-Path $stage 'ue4ss'
            New-Item -ItemType Directory -Force -Path $ue4ssDir|Out-Null

            foreach($name in @(
                'UE4SS.log',
                'UE4SS-settings.ini',
                'mods.txt'
            )){
                $source=Join-Path ([string]$layout.win64) $name

                if(Test-Path -LiteralPath $source){
                    Copy-Item `
                        -LiteralPath $source `
                        -Destination (Join-Path $ue4ssDir $name) `
                        -Force
                    $ue4ssCopied++
                }
            }
        }
        catch{
            $collectionErrors+=(
                'ue4ss_copy: '+
                (Get-ExceptionSummary -ErrorObject $_)
            )
        }
    }

    Write-DiagnosticsStage `
        -Index 8 `
        -Stage 'copy_ue4ss_files' `
        -Detail ('files_copied='+$ue4ssCopied) `
        -Status 'complete'

    $objectDumpLines=0

    Write-DiagnosticsStage `
        -Index 9 `
        -Stage 'extract_objectdump_evidence' `
        -Detail 'Filtering Assault/Treasure/Mole/Boss evidence from UE4SS_ObjectDump.' `
        -Status 'running'

    if($null-ne$layout){
        try{
            $analysisDir=Join-Path $stage 'analysis'
            New-Item -ItemType Directory -Force -Path $analysisDir|Out-Null

            $objectDump=Join-Path (
                [string]$layout.win64
            ) 'UE4SS_ObjectDump.txt'

            if(Test-Path -LiteralPath $objectDump){
                $selected=@(
                    Select-String `
                        -LiteralPath $objectDump `
                        -Pattern (
                            'UnexpectedMission|DETMonsterSpawn|SpawnCondition|'+
                            'Weather|Climate|RespawnCycle|SectionMonster|'+
                            'MiniGame_Mole_120|DsMiniGameNode_Mole|'+
                            'DsMiniGameMole|DMiniGameTable|TreasureBox|'+
                            'Treasure|Chest|InStandAlone|OPENED_BIT_FIELD|'+
                            'tb_treasure_box|DsAnimationProp|'+
                            'DInteractableComponent|ReturnContentsPropState'
                        ) |
                        Select-Object -First 60000 |
                        ForEach-Object{$_.Line}
                )

                $objectDumpLines=$selected.Count

                $selected|
                    Set-Content `
                        -LiteralPath (
                            Join-Path $analysisDir `
                                'objectdump-assault-treasure-candidates.txt'
                        ) `
                        -Encoding UTF8
            }
        }
        catch{
            $collectionErrors+=(
                'object_dump_extract: '+
                (Get-ExceptionSummary -ErrorObject $_)
            )
        }
    }

    Write-DiagnosticsStage `
        -Index 9 `
        -Stage 'extract_objectdump_evidence' `
        -Detail ('lines_written='+$objectDumpLines) `
        -Status 'complete'

    Write-DiagnosticsStage `
        -Index 10 `
        -Stage 'write_collection_summary' `
        -Detail 'Writing collection-summary.json and collection-errors.txt.' `
        -Status 'running'

    $finalGameRunning=@(Get-GameProcessesCompat).Count -gt 0

    $summary=[ordered]@{
        schema_version=1
        probe_version='1.0.34'
        collected_utc=(Get-Date).ToUniversalTime().ToString('o')
        game_running_at_start=$gameRunningAtStart
        waited_for_game_exit=[bool]$waitResult.Waited
        game_exit_wait_seconds=[int]$waitResult.Seconds
        automatic_monitor_wait_seconds=[int]$monitorWaitSeconds
        game_running=$finalGameRunning
        copied_bytes=$copiedBytes
        ue4ss_files_copied=$ue4ssCopied
        objectdump_extract_lines=$objectDumpLines
        error_count=$collectionErrors.Count
        boss_runtime_monitoring=$false
        assault_strategy='known read-only bool transition capture'
        mole_strategy='frozen; completion source confirmed 40/40'
        treasure_strategy='targeted DsAnimationProp snapshots plus observation hooks'
        diagnostics_behavior='wait_for_game_exit_then_collect_and_package'
    }

    Write-JsonUtf8NoBom `
        -Path (Join-Path $stage 'collection-summary.json') `
        -Value $summary `
        -Depth 8

    if($collectionErrors.Count -gt 0){
        $collectionErrors|
            Set-Content `
                -LiteralPath (Join-Path $stage 'collection-errors.txt') `
                -Encoding UTF8
    }

    # Include the latest progress state in the archive itself.
    Copy-Item `
        -LiteralPath $progressPath `
        -Destination (Join-Path $stage 'diagnostics-progress.json') `
        -Force `
        -ErrorAction SilentlyContinue

    Copy-Item `
        -LiteralPath $progressLog `
        -Destination (Join-Path $stage 'diagnostics-progress.log') `
        -Force `
        -ErrorAction SilentlyContinue

    Write-DiagnosticsStage `
        -Index 10 `
        -Stage 'write_collection_summary' `
        -Detail ('errors='+$collectionErrors.Count) `
        -Status 'complete'

    Write-DiagnosticsStage `
        -Index 11 `
        -Stage 'create_zip' `
        -Detail $outputZip `
        -Status 'running'

    New-ZipFromDirectory `
        -SourceDirectory $stage `
        -DestinationZip $outputZip

    $zipSize=(Get-Item -LiteralPath $outputZip -Force).Length

    Write-DiagnosticsStage `
        -Index 11 `
        -Stage 'create_zip' `
        -Detail ('zip_bytes='+$zipSize) `
        -Status 'complete'

    Write-DiagnosticsStage `
        -Index 12 `
        -Stage 'verify_and_complete' `
        -Detail 'Verifying ZIP structure and required summary entry.' `
        -Status 'running'

    $entryCount=Test-ZipArchive `
        -ZipPath $outputZip `
        -RequiredEntry 'collection-summary.json'

    Write-DiagnosticsStage `
        -Index 12 `
        -Stage 'verify_and_complete' `
        -Detail (
            'entries='+
            $entryCount+
            '; output='+
            $outputZip
        ) `
        -Status 'complete'

    Write-Host ''
    Write-Host ('Created: '+$outputZip) -ForegroundColor Green
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_

    Write-DiagnosticsStage `
        -Index 12 `
        -Stage 'verify_and_complete' `
        -Detail $message `
        -Status 'error'

    throw
}
finally{
    Remove-Item `
        -LiteralPath $stage `
        -Recurse `
        -Force `
        -ErrorAction SilentlyContinue
}
