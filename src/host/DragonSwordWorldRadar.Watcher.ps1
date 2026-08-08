param(
    [string]$ModDir = (Split-Path -Parent $PSScriptRoot),
    [string]$RequestStamp = ''
)
$ErrorActionPreference = 'Stop'
$ModDir = [IO.Path]::GetFullPath($ModDir)
. (Join-Path $PSScriptRoot 'DragonSwordWorldRadar.Common.ps1')
$runtime = Join-Path $ModDir 'runtime'
$logDir = Join-Path $runtime 'logs'
New-Item -ItemType Directory -Force -Path $logDir,(Join-Path $runtime 'bridge') | Out-Null
$processWorkDir = Join-Path ([IO.Path]::GetTempPath()) 'DragonSwordWorldRadar'
New-Item -ItemType Directory -Force -Path $processWorkDir | Out-Null
[Environment]::CurrentDirectory = $processWorkDir
Set-Location -LiteralPath $processWorkDir
$log = Join-Path $logDir 'DragonSwordWorldRadar.WatcherHost.log'
function Log([string]$message) {
    try { Add-Content -LiteralPath $log -Encoding UTF8 -Value ("[{0:O}] {1}" -f [DateTime]::UtcNow,$message) } catch {}
}

function Test-RadarEnabled {
    $modsPath = Join-Path (Split-Path -Parent $ModDir) 'mods.txt'
    if (-not (Test-Path -LiteralPath $modsPath -PathType Leaf)) {
        return $true
    }

    try {
        $modsText = [IO.File]::ReadAllText($modsPath)
        $match = [regex]::Match(
            $modsText,
            '(?m)^[ \t]*DragonSwordWorldRadar[ \t]*:[ \t]*([01])')
        if ($match.Success) {
            return $match.Groups[1].Value -eq '1'
        }
    }
    catch {
        Log ("MOD_SWITCH_READ_FAILED " + ($_ | Out-String))
    }
    return $true
}

$createdNew = $false
$mutex = New-Object Threading.Mutex($true, 'Local\DragonSwordWorldRadar.TransientHost', [ref]$createdNew)
if (-not $createdNew) { Log "HOST_ALREADY_RUNNING stamp=$RequestStamp pid=$PID"; exit 0 }
try {
    Log "HOST_START version=0.4.0-dev9-performance1.8-singlebridge1 stamp=$RequestStamp pid=$PID"
    if (-not (Test-RadarEnabled)) {
        Log "MOD_DISABLED DragonSwordWorldRadar=0 stamp=$RequestStamp"
        exit 0
    }

    $layout = Resolve-DragonSwordWorldRadarGameLayout -ModDir $ModDir
    $compatibility = Test-DragonSwordWorldRadarInstalledGame -ModDir $ModDir -Layout $layout
    if (-not $compatibility.Compatible) {
        $requiredPath = Join-Path $runtime 'reinstall-required.json'
        Write-DragonSwordWorldRadarJson -Path $requiredPath -Value ([ordered]@{
            reason = $compatibility.Reason
            detected_at_utc = [DateTime]::UtcNow.ToString('O')
            installed_game = $compatibility.Installed
            current_game = $compatibility.Current
        })
        Log ("REINSTALL_REQUIRED reason={0} current={1}" -f $compatibility.Reason,$compatibility.Current.display_version)
        Add-Type -AssemblyName System.Windows.Forms
        $installedVersion = if ($compatibility.Installed) { [string]$compatibility.Installed.display_version } else { 'not installed' }
        [System.Windows.Forms.MessageBox]::Show(
            "DragonSwordWorldRadar data was generated for game version: $installedVersion`r`nCurrent game version: $($compatibility.Current.display_version)`r`n`r`nRun Install.cmd to regenerate local game data.",
            'DragonSwordWorldRadar - Reinstall required',
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Warning) | Out-Null
        exit 2
    }
    Remove-Item -LiteralPath (Join-Path $runtime 'reinstall-required.json') -Force -ErrorAction SilentlyContinue
    $gameProcess = Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue |
        Sort-Object StartTime -Descending |
        Select-Object -First 1
    if (-not $gameProcess) {
        Log "GAME_SESSION_NOT_FOUND stamp=$RequestStamp"
        exit 0
    }
    $env:EVENTRADAR_GAME_PID = [string]$gameProcess.Id
    Log ("GAME_SESSION_BOUND stamp={0} gamePid={1}" -f $RequestStamp,$gameProcess.Id)
    & (Join-Path $PSScriptRoot 'DragonSwordWorldRadar.Host.ps1') -ModDir $ModDir
    Log "HOST_RETURN stamp=$RequestStamp pid=$PID"
} catch {
    Log ("HOST_FATAL " + ($_ | Out-String))
    exit 1
} finally {
    try { $mutex.ReleaseMutex() } catch {}
    try { $mutex.Dispose() } catch {}
}
