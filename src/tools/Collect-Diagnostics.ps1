$ErrorActionPreference = 'Stop'

$modRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$runtime = Join-Path $modRoot 'runtime'
$diagnosticsRoot = Join-Path $runtime 'diagnostics'
$timestamp = [DateTime]::Now.ToString('yyyyMMdd-HHmmss')
$work = Join-Path $diagnosticsRoot ("work-$timestamp-$PID")
$output = Join-Path $diagnosticsRoot ("DragonSwordWorldRadar-Diagnostics-$timestamp.zip")
$collectorLog = Join-Path $diagnosticsRoot 'DragonSwordWorldRadar.Diagnostics.log'

New-Item -ItemType Directory -Force -Path $diagnosticsRoot | Out-Null
New-Item -ItemType Directory -Force -Path $work | Out-Null

function Write-CollectorLog([string]$Message) {
    $line = '[{0}] {1}' -f [DateTime]::UtcNow.ToString('O'), $Message
    Add-Content -LiteralPath $collectorLog -Value $line -Encoding UTF8
    Write-Host $line
}

function Copy-IfExists([string]$Source,[string]$RelativeDestination) {
    if (-not (Test-Path -LiteralPath $Source)) { return }
    $destination = Join-Path $work $RelativeDestination
    $parent = Split-Path -Parent $destination
    if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
    Copy-Item -LiteralPath $Source -Destination $destination -Force
}

$success = $false
try {
    Write-CollectorLog "COLLECT_START version=0.4.0-dev7-stable6; modRoot=$modRoot"

    $logsPath = Join-Path $runtime 'logs'
    if (Test-Path -LiteralPath $logsPath) {
        foreach ($file in @(Get-ChildItem -LiteralPath $logsPath -File -ErrorAction Stop)) {
            Copy-IfExists $file.FullName (Join-Path 'logs' $file.Name)
        }
    }

    foreach ($name in @('active-version.txt','reinstall-required.json','launch.request')) {
        Copy-IfExists (Join-Path $runtime $name) (Join-Path 'runtime' $name)
    }
    foreach ($name in @(
        'radar_state_a.json',
        'radar_state_b.json',
        'radar_motion_a.dat',
        'radar_motion_b.dat',
        'radar_state.json')) {
        Copy-IfExists (Join-Path $runtime ('bridge\' + $name)) (Join-Path 'bridge' $name)
    }
    foreach ($name in @('release.json','install-state.json','datasets.json','build-manifest.json')) {
        Copy-IfExists (Join-Path $modRoot ('metadata\' + $name)) (Join-Path 'metadata' $name)
    }
    Copy-IfExists (Join-Path $modRoot 'scripts\config.lua') 'config\config.lua'
    Copy-IfExists (Join-Path $modRoot 'data\treasure_overrides.txt') 'config\treasure_overrides.txt'
    Copy-IfExists (Join-Path $modRoot 'data\generated\treasures.lua') 'data\treasures.lua'
    Copy-IfExists (Join-Path $modRoot 'data\generated\bosses.lua') 'data\bosses.lua'

    $modsRoot = Split-Path -Parent $modRoot
    Copy-IfExists (Join-Path $modsRoot 'mods.txt') 'ue4ss\mods.txt'
    $ue4ssRoot = Split-Path -Parent $modsRoot
    foreach ($candidate in @(
        (Join-Path $ue4ssRoot 'UE4SS.log'),
        (Join-Path (Split-Path -Parent $ue4ssRoot) 'UE4SS.log'))) {
        if (Test-Path -LiteralPath $candidate) {
            $tailPath = Join-Path $work 'ue4ss\UE4SS.tail.log'
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $tailPath) | Out-Null
            Get-Content -LiteralPath $candidate -Tail 3000 | Set-Content -LiteralPath $tailPath -Encoding UTF8
            break
        }
    }

    $processRows = @()
    foreach ($name in @('powershell','wscript','DSClient-Win64-Shipping')) {
        $processRows += @(Get-Process -Name $name -ErrorAction SilentlyContinue |
            Select-Object ProcessName,Id,CPU,WorkingSet64,StartTime)
    }
    $system = [ordered]@{
        collected_at_utc = [DateTime]::UtcNow.ToString('O')
        os = [Environment]::OSVersion.VersionString
        powershell = $PSVersionTable.PSVersion.ToString()
        is_64_bit = [Environment]::Is64BitProcess
        processes = $processRows
    }
    $system | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'system.json') -Encoding UTF8

    $hashes = @()
    foreach ($relative in @(
        'scripts\main.lua',
        'scripts\world_map.lua',
        'scripts\treasures.lua',
        'scripts\bosses.lua',
        'src\overlay\Bridge\MotionBridgeReader.cs',
        'src\overlay\Bridge\StaticStateBridgeReader.cs',
        'src\overlay\Rendering\TreasureMarkerPalette.cs',
        'src\overlay\UI\RadarForm.cs',
        'src\overlay\SaveData\SaveDatabaseFingerprint.cs',
        'src\overlay\SaveData\BossRespawnRuleResolver.cs',
        'src\overlay\SaveData\TreasureSaveState.cs',
        'host\DragonSwordWorldRadar.Watcher.vbs',
        'host\DragonSwordWorldRadar.Host.ps1',
        'metadata\release.json',
        'data\generated\treasures.lua',
        'data\generated\bosses.lua')) {
        $path = Join-Path $modRoot $relative
        if (Test-Path -LiteralPath $path) {
            $hashes += [ordered]@{
                path = $relative
                sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
    }
    $hashes | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $work 'hashes.json') -Encoding UTF8

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output -Force }
    [IO.Compression.ZipFile]::CreateFromDirectory(
        $work,
        $output,
        [IO.Compression.CompressionLevel]::Optimal,
        $false)

    if (-not (Test-Path -LiteralPath $output)) {
        throw "ZIP creation returned without producing $output"
    }
    $length = (Get-Item -LiteralPath $output).Length
    if ($length -le 0) { throw "Diagnostics ZIP is empty: $output" }

    $success = $true
    Write-CollectorLog "COLLECT_OK output=$output; bytes=$length"
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue

    try {
        Start-Process explorer.exe -ArgumentList ('/select,"{0}"' -f $output) -ErrorAction Stop
    } catch {
        Write-CollectorLog "EXPLORER_SELECT_FAILED $($_.Exception.Message)"
    }
    Write-Host ""
    Write-Host "Diagnostics created successfully:"
    Write-Host $output
    exit 0
}
catch {
    Write-CollectorLog "COLLECT_FAILED $($_.Exception.GetType().FullName): $($_.Exception.Message)"
    Write-Host ""
    Write-Host "Diagnostics collection failed." -ForegroundColor Red
    Write-Host "The uncompressed diagnostic files were preserved at:"
    Write-Host $work
    Write-Host "Collector log:"
    Write-Host $collectorLog
    exit 1
}
