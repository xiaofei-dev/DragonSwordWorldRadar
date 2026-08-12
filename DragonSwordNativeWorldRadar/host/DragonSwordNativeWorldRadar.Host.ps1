param(
    [string]$ModDir = (Split-Path -Parent $PSScriptRoot),
    [int]$GamePid = 0
)

$ErrorActionPreference = 'Stop'
$ModDir = [IO.Path]::GetFullPath($ModDir)
$env:NATIVEWORLDRADAR_MOD_DIR = $ModDir
if ($GamePid -gt 0) { $env:NATIVEWORLDRADAR_GAME_PID = [string]$GamePid }

$runtime = Join-Path $ModDir 'runtime'
$logDir = Join-Path $runtime 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$processWorkDir = Join-Path ([IO.Path]::GetTempPath()) 'DragonSwordNativeWorldRadar'
New-Item -ItemType Directory -Force -Path $processWorkDir | Out-Null
[Environment]::CurrentDirectory = $processWorkDir
Set-Location -LiteralPath $processWorkDir
$log = Join-Path $logDir 'DragonSwordNativeWorldRadar.Host.log'

function Write-HostLog([string]$Message) {
    Add-Content -LiteralPath $log -Encoding UTF8 -Value ("[{0:O}] {1}" -f [DateTime]::UtcNow, $Message)
}

$mutex = $null
$ownsMutex = $false
try {
    $createdNew = $false
    $mutex = New-Object Threading.Mutex($true, 'Local\DragonSwordNativeWorldRadar.OverlayHost', [ref]$createdNew)
    if (-not $createdNew) { Write-HostLog 'ALREADY_RUNNING'; return }
    $ownsMutex = $true
    Write-HostLog "START version=0.1.0-medium-poc pid=$PID gamePid=$GamePid"

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

    $sourceRoot = Join-Path $ModDir 'src\overlay'
    $sources = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -Filter '*.cs' -File |
        Sort-Object FullName | Select-Object -ExpandProperty FullName)
    if ($sources.Count -lt 40) { throw "Incomplete overlay snapshot: $($sources.Count) files" }
    $refs = @(
        [System.Windows.Forms.Form].Assembly.Location,
        [System.Drawing.Graphics].Assembly.Location,
        [System.Linq.Enumerable].Assembly.Location,
        [System.Uri].Assembly.Location
    ) | Select-Object -Unique
    Write-HostLog ("COMPILE sources={0}" -f $sources.Count)
    Add-Type -Path $sources -ReferencedAssemblies $refs -ErrorAction Stop
    Write-HostLog 'COMPILE_OK'

    [IO.File]::WriteAllText(
        (Join-Path $runtime 'active-version.txt'),
        '0.1.0-medium-poc native-provider + WinForms-overlay',
        [Text.UTF8Encoding]::new($false))
    [DragonSwordWorldRadar.Program]::Run()
    Write-HostLog 'RETURNED'
} catch {
    Write-HostLog ("FATAL " + ($_ | Out-String))
    throw
} finally {
    Remove-Item -LiteralPath (Join-Path $runtime 'active-version.txt') -Force -ErrorAction SilentlyContinue
    Get-ChildItem -LiteralPath (Join-Path $runtime 'bridge') -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like 'native_radar_motion_*.dat' } |
        Remove-Item -Force -ErrorAction SilentlyContinue
    if ($ownsMutex -and $null -ne $mutex) {
        try { $mutex.ReleaseMutex() } catch {}
        try { $mutex.Dispose() } catch {}
    }
}
