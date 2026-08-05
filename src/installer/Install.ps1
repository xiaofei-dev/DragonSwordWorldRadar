$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$installerRoot = $PSScriptRoot
$modRoot = [IO.Path]::GetFullPath((Split-Path -Parent $installerRoot))
. (Join-Path $modRoot 'host\DragonSwordWorldRadar.Common.ps1')

$runtime = Join-Path $modRoot 'runtime'
$logDir = Join-Path $runtime 'logs'
$archiveDir = Join-Path $logDir 'archive'
$bridgeDir = Join-Path $runtime 'bridge'
$dataRoot = Join-Path $modRoot 'data'
$generatedRoot = Join-Path $dataRoot 'generated'
$metadataRoot = Join-Path $modRoot 'metadata'
New-Item -ItemType Directory -Force -Path $runtime,$logDir,$archiveDir,$bridgeDir,$dataRoot,$generatedRoot,$metadataRoot | Out-Null
$log = Join-Path $logDir 'DragonSwordWorldRadar.Install.log'
$utf8 = [Text.UTF8Encoding]::new($false)

function Log([string]$message) {
    [IO.File]::AppendAllText(
        $log,
        ("[{0:yyyy-MM-dd HH:mm:ss.fff}] {1}{2}" -f [DateTime]::Now,$message,[Environment]::NewLine),
        $utf8)
}

function Merge-LuaConfig {
    param([string]$DefaultPath,[string]$UserPath)
    if (-not (Test-Path -LiteralPath $UserPath)) {
        Copy-Item -LiteralPath $DefaultPath -Destination $UserPath
        return
    }
    $existing = @(Get-Content -LiteralPath $UserPath)
    $default = @(Get-Content -LiteralPath $DefaultPath)
    $pattern = '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*='
    $known = @{}
    foreach ($line in $existing) {
        $match = [regex]::Match(($line -replace '--.*$',''),$pattern)
        if ($match.Success) { $known[$match.Groups[1].Value.ToLowerInvariant()] = $true }
    }
    $missing = New-Object Collections.Generic.List[string]
    foreach ($line in $default) {
        $match = [regex]::Match(($line -replace '--.*$',''),$pattern)
        if ($match.Success -and -not $known.ContainsKey($match.Groups[1].Value.ToLowerInvariant())) {
            $missing.Add($line)
            $known[$match.Groups[1].Value.ToLowerInvariant()] = $true
        }
    }
    if ($missing.Count -eq 0) { return }
    $closing = -1
    for ($index = $existing.Count - 1; $index -ge 0; $index--) {
        if ($existing[$index] -match '^\s*}\s*,?\s*$') { $closing = $index; break }
    }
    if ($closing -lt 0) { throw 'scripts\config.lua is malformed; could not merge new settings.' }
    $updated = @($existing[0..($closing-1)]) + @('','    -- Settings added by DragonSwordWorldRadar upgrade.') + $missing + @($existing[$closing..($existing.Count-1)])
    [IO.File]::WriteAllLines($UserPath,$updated,$utf8)
}

function Stop-ExistingWatcher {
    $stopPath = Join-Path $runtime 'watcher.stop'
    [IO.File]::WriteAllText($stopPath,[DateTime]::UtcNow.ToString('O'),$utf8)
    Start-Sleep -Milliseconds 900
    try {
        $escapedRoot = $modRoot.ToLowerInvariant()
        Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
            Where-Object {
                $_.ProcessId -ne $PID -and $_.CommandLine -and
                $_.CommandLine.ToLowerInvariant().Contains($escapedRoot) -and
                ($_.Name -match '^(powershell|pwsh|wscript)\.exe$') -and
                ($_.CommandLine -match 'DragonSwordWorldRadar\.Watcher\.(ps1|vbs)|DragonSwordWorldRadarWatcher\.ps1')
            } | ForEach-Object {
                Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue
            }
    } catch {}
    Remove-Item -LiteralPath $stopPath -Force -ErrorAction SilentlyContinue
}

try {
    if (Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue) {
        throw 'Close DragonSword Awakening before installing or regenerating DragonSwordWorldRadar data.'
    }
    if (Test-Path -LiteralPath $log) {
        $archive = Join-Path $archiveDir ('DragonSwordWorldRadar.Install.' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff') + '.log')
        Move-Item -LiteralPath $log -Destination $archive -Force
    }
    Log 'INSTALL_START version=0.3.2c mode=self-contained-bootstrap-with-regeneration'

    $releasePath = Join-Path $metadataRoot 'release.json'
    if (-not (Test-Path -LiteralPath $releasePath)) { throw 'metadata\release.json is missing.' }
    $release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
    if ([string]$release.version -ne '0.3.2c') { throw "Unexpected release version: $($release.version)" }

    $layout = Resolve-DragonSwordWorldRadarGameLayout -ModDir $modRoot
    Log ("GAME_LAYOUT root={0}; exe={1}; pak={2}; oodle={3}" -f $layout.GameRoot,$layout.ExecutablePath,$layout.PakPath,$layout.OodleLibraryPath)

    $bundledOozPath = Join-Path $modRoot 'tools\ooz.exe'
    $bundledOozSha256 = '520e3596e50859194e1fcbf9cfda09ea0e33a70e2793be277f0a0070bd22dc8c'
    if (-not (Test-Path -LiteralPath $bundledOozPath -PathType Leaf)) {
        throw 'Bundled tools\ooz.exe is missing. Re-extract the complete DragonSwordWorldRadar release package.'
    }
    $actualOozSha256 = (Get-FileHash -LiteralPath $bundledOozPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualOozSha256 -ne $bundledOozSha256) {
        throw ("Bundled tools\ooz.exe failed integrity validation. Expected={0}; actual={1}" -f $bundledOozSha256,$actualOozSha256)
    }
    if ([IO.Path]::GetFullPath([string]$layout.OodleLibraryPath) -ne [IO.Path]::GetFullPath($bundledOozPath)) {
        throw 'Installer did not select the bundled tools\ooz.exe as the preferred decoder.'
    }
    Log ("TOOL_VALIDATED name=ooz.exe; sha256={0}; path={1}" -f $actualOozSha256,$bundledOozPath)

    $modsRoot = Split-Path -Parent $modRoot
    $oldModRoot = Join-Path $modsRoot 'eventrader'
    $defaultConfig = Join-Path $modRoot 'scripts\config.default.lua'
    $userConfig = Join-Path $modRoot 'scripts\config.lua'
    if (-not (Test-Path -LiteralPath $userConfig) -and (Test-Path -LiteralPath (Join-Path $oldModRoot 'scripts\config.lua'))) {
        Copy-Item -LiteralPath (Join-Path $oldModRoot 'scripts\config.lua') -Destination $userConfig
        Log 'MIGRATED config.lua from legacy eventrader folder'
    }
    Merge-LuaConfig -DefaultPath $defaultConfig -UserPath $userConfig

    $overridePath = Join-Path $dataRoot 'treasure_overrides.txt'
    $legacyOverridePath = Join-Path $modRoot 'treasure_overrides.txt'
    $defaultOverridePath = Join-Path $dataRoot 'defaults\treasure_overrides.txt'
    if (-not (Test-Path -LiteralPath $overridePath)) {
        if (Test-Path -LiteralPath $legacyOverridePath) {
            Copy-Item -LiteralPath $legacyOverridePath -Destination $overridePath
            Log 'MIGRATED legacy treasure_overrides.txt to data\treasure_overrides.txt'
        } elseif (Test-Path -LiteralPath (Join-Path $oldModRoot 'data\treasure_overrides.txt')) {
            Copy-Item -LiteralPath (Join-Path $oldModRoot 'data\treasure_overrides.txt') -Destination $overridePath
            Log 'MIGRATED treasure_overrides.txt from legacy eventrader folder'
        } else {
            Copy-Item -LiteralPath $defaultOverridePath -Destination $overridePath
            Log 'CREATED data\treasure_overrides.txt from defaults'
        }
    } else {
        Log 'PRESERVED data\treasure_overrides.txt'
    }

    Stop-ExistingWatcher

    $identityBeforeData = Get-DragonSwordWorldRadarGameIdentity -Layout $layout
    $bundledFingerprint = '20c421817e932c8bb0e6d761a24ea247a1ae91abd32629150cc9761a9067343b'
    $bundledCatalog = Join-Path $modRoot 'scripts\treasures.lua'
    $results = @()

    if ($layout.OodleLibraryPath) {
        $installerSources = @(Get-ChildItem -LiteralPath (Join-Path $modRoot 'src\installer') -Recurse -Filter '*.cs' -File |
            Sort-Object FullName | Select-Object -ExpandProperty FullName)
        if ($installerSources.Count -lt 8) { throw "Installer source set is incomplete: $($installerSources.Count) files." }
        Add-Type -AssemblyName System.Xml
        $installerRefs = @(
            [System.Xml.XmlDocument].Assembly.Location,
            [System.Security.Cryptography.Aes].Assembly.Location,
            [System.Linq.Enumerable].Assembly.Location,
            [System.Uri].Assembly.Location
        ) | Select-Object -Unique
        Log ("INSTALLER_COMPILE sources={0}" -f $installerSources.Count)
        Add-Type -Path $installerSources -ReferencedAssemblies $installerRefs -ErrorAction Stop
        Log 'INSTALLER_COMPILE_OK'
        $results = @([DragonSwordWorldRadar.Installer.InstallationPipeline]::GenerateData(
            $modRoot,
            $layout.GameRoot,
            $layout.OodleLibraryPath))
        if ($results.Count -eq 0) { throw 'The data provider pipeline returned no datasets.' }
    } else {
        if ([string]$identityBeforeData.fingerprint -ne $bundledFingerprint) {
            throw ("This game build differs from the bundled dataset and ooz.exe is unavailable. Current fingerprint={0}. Place ooz.exe in DragonSwordWorldRadar\tools or DS\Binaries\Win64\Mods, then run Install.cmd again." -f $identityBeforeData.fingerprint)
        }
        if (-not (Test-Path -LiteralPath $bundledCatalog -PathType Leaf)) {
            throw 'The bundled bootstrap treasure catalog is missing.'
        }
        $bootstrapText = [IO.File]::ReadAllText($bundledCatalog)
        $bootstrapCount = [regex]::Matches($bootstrapText,'(?m)\bsave_id\s*=').Count
        if ($bootstrapCount -lt 1000 -or $bootstrapText -notmatch '\bz\s*=' -or $bootstrapText -notmatch '\buid_name\s*=' -or $bootstrapText -notmatch '\bgroup_id\s*=') {
            throw "Bundled bootstrap treasure catalog validation failed (records=$bootstrapCount)."
        }
        $bootstrapOutput = Join-Path $generatedRoot 'treasures.lua'
        Copy-Item -LiteralPath $bundledCatalog -Destination $bootstrapOutput -Force
        $results = @([pscustomobject]@{
            Id = 'treasures'
            RecordCount = $bootstrapCount
            OutputPath = $bootstrapOutput
            SourcePak = $layout.PakPath
            SourceEntry = 'bundled-bootstrap-for-20c421817e93'
        })
        Log ("DATASET_BOOTSTRAP id=treasures; records={0}; reason=ooz-not-found; fingerprint={1}" -f $bootstrapCount,$identityBeforeData.fingerprint)
    }

    $datasetEntries = @()
    foreach ($result in $results) {
        if (-not (Test-Path -LiteralPath $result.OutputPath)) { throw "Generated dataset is missing: $($result.OutputPath)" }
        $datasetEntries += [ordered]@{
            id = [string]$result.Id
            record_count = [int]$result.RecordCount
            output = $result.OutputPath.Substring($modRoot.Length).TrimStart('\')
            sha256 = (Get-FileHash -LiteralPath $result.OutputPath -Algorithm SHA256).Hash.ToLowerInvariant()
            source_pak = [IO.Path]::GetFileName([string]$result.SourcePak)
            source_entry = [string]$result.SourceEntry
        }
        Log ("DATASET_GENERATED id={0}; records={1}; output={2}" -f $result.Id,$result.RecordCount,$result.OutputPath)
    }
    Write-DragonSwordWorldRadarJson -Path (Join-Path $metadataRoot 'datasets.json') -Value ([ordered]@{
        schema_version = 1
        generated_at_utc = [DateTime]::UtcNow.ToString('O')
        providers = $datasetEntries
    })

    $catalogPath = Join-Path $generatedRoot 'treasures.lua'
    $catalogText = [IO.File]::ReadAllText($catalogPath)
    $recordCount = [regex]::Matches($catalogText,'(?m)\bsave_id\s*=').Count
    if ($recordCount -lt 1000 -or $catalogText -notmatch '\bz\s*=' -or $catalogText -notmatch '\buid_name\s*=' -or $catalogText -notmatch '\bgroup_id\s*=') {
        throw "Generated treasure catalog validation failed (records=$recordCount)."
    }

    $identity = Get-DragonSwordWorldRadarGameIdentity -Layout $layout
    Write-DragonSwordWorldRadarJson -Path (Join-Path $metadataRoot 'install-state.json') -Value ([ordered]@{
        schema_version = 1
        mod_version = '0.3.2c'
        installed_at_utc = [DateTime]::UtcNow.ToString('O')
        game_root = $layout.GameRoot
        game = $identity
        datasets_manifest = 'metadata\datasets.json'
    })
    Log ("GAME_FINGERPRINT version={0}; fingerprint={1}" -f $identity.display_version,$identity.fingerprint)

    $modsRoot = Split-Path -Parent $modRoot
    $modsFile = Join-Path $modsRoot 'mods.txt'
    $lines = if (Test-Path -LiteralPath $modsFile) { @(Get-Content -LiteralPath $modsFile) } else { @() }
    function Set-ModState([string]$Name,[int]$State) {
        $script:lines = @($script:lines | Where-Object { $_ -notmatch ('^\s*' + [regex]::Escape($Name) + '\s*:') })
        $script:lines += ("{0} : {1}" -f $Name,$State)
    }
    Set-ModState 'DragonSwordTreasureMap' 0
    Set-ModState 'eventrader' 0
    Set-ModState 'DragonSwordWorldRadar' 1
    [IO.File]::WriteAllLines($modsFile,$lines,$utf8)
    Log 'MOD_ENABLED DragonSwordWorldRadar=1 oldEventRadar=0 legacyTreasureMod=0'

    # Remove stale experimental coordinate rules. The supported override
    # contract is intentionally limited to ignore and alias.
    $activeOverridePath = Join-Path $modRoot 'data\treasure_overrides.txt'
    if (Test-Path -LiteralPath $activeOverridePath) {
        $overrideLines = @(Get-Content -LiteralPath $activeOverridePath)
        $cleanOverrideLines = @($overrideLines | Where-Object {
            $_ -notmatch '^\s*ignore_coord(?:\s|$)'
        })
        $removedCoordinateRules = $overrideLines.Count - $cleanOverrideLines.Count
        if ($removedCoordinateRules -gt 0) {
            [IO.File]::WriteAllLines($activeOverridePath,$cleanOverrideLines,$utf8)
            Log ("OVERRIDE_MIGRATION removedExperimentalCoordinateRules={0}; supported=ignore,alias" -f $removedCoordinateRules)
        }
    }

    # Remove obsolete pre-0.3.2c files after migration.
    foreach ($legacy in @(
        'DragonSwordWorldRadar.ps1','DragonSwordWorldRadarWatcher.ps1','OverlayBootstrap.ps1','StartDragonSwordWorldRadar.vbs',
        'Install-FirstStep.cmd','Install-FirstStep.ps1','Collect-Diagnostics.ps1',
        'e_sqlcipher.dll','treasure_overrides.default.txt')) {
        Remove-Item -LiteralPath (Join-Path $modRoot $legacy) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $modRoot 'overlay-src') -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $legacyOverridePath -Force -ErrorAction SilentlyContinue

    $watcherScript = Join-Path $modRoot 'host\DragonSwordWorldRadar.Watcher.vbs'
    if (-not (Test-Path -LiteralPath $watcherScript -PathType Leaf)) {
        throw 'host\DragonSwordWorldRadar.Watcher.vbs is missing.'
    }
    $startupDirectory = [Environment]::GetFolderPath('Startup')
    foreach ($oldShortcut in @('EventRadarWatcher.lnk','EventRadar.lnk','DragonSwordWorldRadarWatcher.lnk','DragonSwordWorldRadar.lnk')) {
        Remove-Item -LiteralPath (Join-Path $startupDirectory $oldShortcut) -Force -ErrorAction SilentlyContinue
    }
    $shortcutPath = Join-Path $startupDirectory 'DragonSwordWorldRadar.lnk'
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = "$env:SystemRoot\System32\wscript.exe"
    $watcherArguments = '//B //NoLogo "' + $watcherScript.Replace('"','""') + '" "' + $modRoot.Replace('"','""') + '"'
    $shortcut.Arguments = $watcherArguments
    $shortcut.WorkingDirectory = $modRoot
    $shortcut.WindowStyle = 7
    $shortcut.Save()

    $watcherLog = Join-Path $logDir 'DragonSwordWorldRadar.Watcher.log'
    Remove-Item -LiteralPath $watcherLog -Force -ErrorAction SilentlyContinue
    $watcherProcess = Start-Process -FilePath $shortcut.TargetPath -ArgumentList $watcherArguments -WindowStyle Hidden -PassThru
    $ready = $false
    for ($attempt = 0; $attempt -lt 40; $attempt++) {
        Start-Sleep -Milliseconds 100
        if (Test-Path -LiteralPath $watcherLog) {
            $text = [IO.File]::ReadAllText($watcherLog)
            if ($text -match 'WATCHER_START') { $ready = $true; break }
        }
        if ($watcherProcess.HasExited) { break }
    }
    if (-not $ready) { throw 'The hidden DragonSwordWorldRadar WScript watcher did not become ready. Check runtime\logs\DragonSwordWorldRadar.Watcher.log.' }
    Log 'WATCHER_READY host=wscript powershell_lifetime=game-only'

    $allExecutables = @(Get-ChildItem -LiteralPath $modRoot -Filter '*.exe' -File -Recurse -ErrorAction SilentlyContinue)
    $unexpectedExecutables = @($allExecutables | Where-Object { [IO.Path]::GetFullPath($_.FullName) -ne [IO.Path]::GetFullPath($bundledOozPath) })
    if ($unexpectedExecutables.Count -gt 0) {
        throw ('Unexpected EXE files are not allowed: ' + (($unexpectedExecutables.FullName) -join ', '))
    }
    if ($allExecutables.Count -ne 1) { throw "Expected exactly one bundled tool executable, found $($allExecutables.Count)." }

    Log 'INSTALL_COMPLETE version=0.3.2c customExe=0 bundledToolExe=1 watcher=wscript transientPowerShell=true'
    Write-Host "DragonSwordWorldRadar 0.3.2c installed. Generated $recordCount treasure records for game version $($identity.display_version)."
    Write-Host 'Start the game normally. F7 enables the radar; F8 disables it.'
} catch {
    try { Log ("INSTALL_FAILED " + ($_ | Out-String)) } catch {}
    Write-Error $_
    exit 1
}
