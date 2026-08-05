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

$createdNew = $false
$mutex = New-Object Threading.Mutex($true, 'Local\DragonSwordWorldRadar.TransientHost', [ref]$createdNew)
if (-not $createdNew) { Log "HOST_ALREADY_RUNNING stamp=$RequestStamp pid=$PID"; exit 0 }
try {
    Log "HOST_START version=0.4.0-dev7-stable6 stamp=$RequestStamp pid=$PID"
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
    & (Join-Path $PSScriptRoot 'DragonSwordWorldRadar.Host.ps1') -ModDir $ModDir
    Log "HOST_RETURN stamp=$RequestStamp pid=$PID"
} catch {
    Log ("HOST_FATAL " + ($_ | Out-String))
    exit 1
} finally {
    try { $mutex.ReleaseMutex() } catch {}
    try { $mutex.Dispose() } catch {}
}
