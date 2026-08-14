param([string]$ModDir = (Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference = 'Stop'
$ModDir = [IO.Path]::GetFullPath($ModDir)
$env:EVENTRADAR_MOD_DIR = $ModDir
$runtime = Join-Path $ModDir 'runtime'
$logDir = Join-Path $runtime 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$processWorkDir = Join-Path ([IO.Path]::GetTempPath()) 'DragonSwordWorldRadar'
New-Item -ItemType Directory -Force -Path $processWorkDir | Out-Null
[Environment]::CurrentDirectory = $processWorkDir
Set-Location -LiteralPath $processWorkDir
$log = Join-Path $logDir 'DragonSwordWorldRadar.Host.log'
function Log([string]$message) {
    Add-Content -LiteralPath $log -Encoding UTF8 -Value ("[{0:O}] {1}" -f [DateTime]::UtcNow,$message)
}

$mutex = $null
$ownsMutex = $false
try {
    $createdNew = $false
    $mutex = New-Object Threading.Mutex($true, 'Local\DragonSwordWorldRadar.OverlayHost', [ref]$createdNew)
    if (-not $createdNew) { Log 'ALREADY_RUNNING'; return }
    $ownsMutex = $true
    Log "START version=0.4.0-dev74-processdispatchguard1-localcapfix1 renderer=WinForms pid=$PID host=single_process_watcher"

    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    if (-not ('DragonSwordWorldRadar.NativeDllSearch' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace DragonSwordWorldRadar {
    public static class NativeDllSearch {
        [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        private static extern bool SetDllDirectory(string path);
        public static void Use(string path) {
            if (!SetDllDirectory(path)) {
                throw new InvalidOperationException("SetDllDirectory failed: " + Marshal.GetLastWin32Error());
            }
        }
    }
}
'@
    }
    [DragonSwordWorldRadar.NativeDllSearch]::Use((Join-Path $ModDir 'vendor\sqlcipher'))

    if (-not ('DragonSwordWorldRadar.Program' -as [type])) {
        $sourceRoot = Join-Path $ModDir 'src\overlay'
        $sources = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -Filter '*.cs' -File |
            Sort-Object FullName | Select-Object -ExpandProperty FullName)
        if ($sources.Count -lt 20) { throw "Incomplete overlay source set: $($sources.Count) files" }
        $refs = @(
            [System.Windows.Forms.Form].Assembly.Location,
            [System.Drawing.Graphics].Assembly.Location,
            [System.Linq.Enumerable].Assembly.Location,
            [System.Uri].Assembly.Location
        ) | Select-Object -Unique
        Log ("COMPILE sources={0}" -f $sources.Count)
        Add-Type -Path $sources -ReferencedAssemblies $refs -ErrorAction Stop
        Log 'COMPILE_OK'
    } else {
        Log 'COMPILE_SKIPPED types_already_loaded=true'
    }

    [IO.File]::WriteAllText(
        (Join-Path $runtime 'active-version.txt'),
        '0.4.0-dev74-processdispatchguard1-localcapfix1 WinForms',
        [Text.UTF8Encoding]::new($false))
    [DragonSwordWorldRadar.Program]::Run()
    Log 'RETURNED'
} catch {
    Log ("FATAL " + ($_ | Out-String))
    throw
} finally {
    try {
        Remove-Item -LiteralPath (Join-Path $runtime 'active-version.txt') -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath (Join-Path $runtime 'launch.request') -Force -ErrorAction SilentlyContinue
        Get-ChildItem -LiteralPath (Join-Path $runtime 'bridge') -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -like 'radar_state*.json' -or $_.Name -like 'radar_motion_*.dat' } |
            Remove-Item -Force -ErrorAction SilentlyContinue
        Log 'SESSION_CLEANUP_COMPLETE'
    } catch {}
    if ($ownsMutex -and $mutex -ne $null) {
        try { $mutex.ReleaseMutex() } catch {}
        try { $mutex.Dispose() } catch {}
    }
}
