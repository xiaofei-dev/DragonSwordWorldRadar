param(
    [Parameter(Mandatory=$true)]
    [string]$Root
)

$ErrorActionPreference = 'Stop'
$toolsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $toolsDir 'Common.ps1')

$Root = Resolve-ProbeRoot -PathValue $Root
Assert-ProbeLayout -ProbeRoot $Root
$suite = Read-JsonUtf8Strict -Path (Join-Path $Root 'config\suite.json')

$createdNew = $false
$monitorMutex = New-Object Threading.Mutex(
    $true,
    'Local\DragonSwordWorldDataProbe_Modular_v101',
    [ref]$createdNew
)
if (-not $createdNew) { exit 0 }

$logPath = Join-Path $Root 'runtime\monitor\monitor.log'
$modsPath = Join-Path (Split-Path -Parent $Root) 'mods.txt'

function Test-ModsTxtEnabled {
    if (-not (Test-Path -LiteralPath $modsPath -PathType Leaf)) { return $false }
    try {
        foreach ($line in [IO.File]::ReadAllLines($modsPath)) {
            if ($line -match '^\s*DragonSwordWorldDataProbe\s*:\s*([01])\s*(?:;.*)?$') {
                return $Matches[1] -eq '1'
            }
        }
    }
    catch { return $false }
    return $false
}

function Write-MonitorLog {
    param([string]$Event,[string]$Text='')
    ('[{0}] [{1}] {2}' -f (Get-Date).ToUniversalTime().ToString('o'),$Event,$Text) |
        Add-Content -LiteralPath $logPath -Encoding UTF8
}

try {
    Write-MonitorLog 'MONITOR_START' 'version=1.0.45'

    if (-not (Test-ModsTxtEnabled)) {
        Write-MonitorLog 'MONITOR_DISABLED' 'mods_txt_not_enabled'
        exit 0
    }

    $pollSeconds = [Math]::Max(1,[int]$suite.monitor.poll_seconds)
    $deadline = (Get-Date).AddMinutes([Math]::Max(1,[int]$suite.monitor.wait_timeout_minutes))
    $gameProcess = $null

    while ($null -eq $gameProcess -and (Get-Date) -lt $deadline) {
        if (-not (Test-ModsTxtEnabled)) {
            Write-MonitorLog 'MONITOR_DISABLED' 'mods_txt_changed_while_waiting_for_game'
            exit 0
        }
        $gameProcess = Get-GameProcessesCompat | Select-Object -First 1
        if ($null -eq $gameProcess) {
            Start-Sleep -Seconds $pollSeconds
        }
    }

    if ($null -eq $gameProcess) {
        Write-MonitorLog 'MONITOR_TIMEOUT' 'game_not_found'
        exit 0
    }

    $gameProcessId = [int]$gameProcess.Id
    $runId = Get-Date -Format 'yyyyMMdd-HHmmss'

    Write-MonitorLog 'GAME_FOUND' ('pid={0};run={1}' -f $gameProcessId,$runId)
    Write-MonitorLog 'COLLECTION_DEFERRED' 'heavy external modules run after game exit'
    Write-MonitorLog 'WAIT_GAME_EXIT_BEGIN' ('pid={0}' -f $gameProcessId)

    while ($null -ne (Get-Process -Id $gameProcessId -ErrorAction SilentlyContinue)) {
        if (-not (Test-ModsTxtEnabled)) {
            Write-MonitorLog 'MONITOR_DISABLED' 'mods_txt_changed_while_game_running'
            exit 0
        }
        Start-Sleep -Seconds $pollSeconds
    }

    Write-MonitorLog 'GAME_EXIT_DETECTED' ('pid={0}' -f $gameProcessId)

    $exitGrace=[Math]::Max(0,[int]$suite.monitor.exit_grace_seconds)
    Write-MonitorLog 'POST_EXIT_GRACE_BEGIN' ('seconds={0}' -f $exitGrace)
    Start-Sleep -Seconds $exitGrace
    Write-MonitorLog 'POST_EXIT_GRACE_COMPLETE' ('seconds={0}' -f $exitGrace)

    if (-not (Test-ModsTxtEnabled)) {
        Write-MonitorLog 'MONITOR_DISABLED' 'mods_txt_not_enabled_before_collection'
        exit 0
    }

    Write-MonitorLog 'GAME_EXIT_COLLECTION_BEGIN' ('run=' + $runId)

    & (Join-Path $toolsDir 'ModuleHost.ps1') `
        -Root ([string]$Root) `
        -Trigger 'game_exit' `
        -RunId ([string]$runId)

    Write-MonitorLog 'GAME_EXIT_COLLECTION_COMPLETE' ('run=' + $runId)
    Write-MonitorLog 'MONITOR_COMPLETE' ('run=' + $runId)
}
catch {
    Write-MonitorLog 'MONITOR_ERROR' (Get-ExceptionSummary -ErrorObject $_)
}
finally {
    try { $monitorMutex.ReleaseMutex() } catch {}
    $monitorMutex.Dispose()
}
