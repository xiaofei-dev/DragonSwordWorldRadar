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
$log = Join-Path $logDir 'DragonSwordWorldRadarObjectState.Install.log'
$utf8 = [Text.UTF8Encoding]::new($false)

function Log([string]$message) {
    [IO.File]::AppendAllText(
        $log,
        ("[{0:yyyy-MM-dd HH:mm:ss.fff}] {1}{2}" -f [DateTime]::Now,$message,[Environment]::NewLine),
        $utf8)
}

function Test-LuaConfig {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }

    try {
        $raw = Get-Content -LiteralPath $Path -Raw -ErrorAction Stop
    } catch {
        return $false
    }

    if ([string]::IsNullOrWhiteSpace($raw)) {
        return $false
    }

    $withoutComments = [regex]::Replace(
        $raw,
        '(?m)--[^\r\n]*$',
        '')
    $trimmed = $withoutComments.Trim()
    if ($trimmed -notmatch '^return\s*\{') {
        return $false
    }

    $openCount = ([regex]::Matches($trimmed, '\{')).Count
    $closeCount = ([regex]::Matches($trimmed, '\}')).Count
    if ($openCount -ne 1 -or $closeCount -ne 1) {
        return $false
    }

    return $trimmed -match '\}\s*$'
}

function Migrate-AssaultConfig {
    param([string]$UserPath)
    $lines = @([IO.File]::ReadAllLines($UserPath))
    $showIndex = -1
    $legacySettingFound = $false
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $code = ($lines[$index] -split '--',2)[0]
        if ($code -match '^\s*assault_performance_ab_isolation\s*=\s*(true|false)\s*,?\s*$') {
            $legacySettingFound = $true
            continue
        }
        if ($code -match '^\s*show_assaults\s*=\s*(true|false)\s*,?\s*$') {
            $showIndex = $index
        }
    }
    $migrated = New-Object System.Collections.Generic.List[string]
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $code = ($lines[$index] -split '--',2)[0]
        if ($code -match '^\s*assault_performance_ab_isolation\s*=') {
            continue
        }
        $migrated.Add($lines[$index])
    }
    if ($showIndex -lt 0) {
        throw 'scripts\config.lua is missing the required show_assaults setting.'
    }
    if ($legacySettingFound) {
        [IO.File]::WriteAllLines($UserPath,$migrated,$utf8)
        Log 'REMOVED legacy assault_performance_ab_isolation setting from scripts\config.lua'
    }
    Log 'PRESERVED user-owned show_assaults setting in scripts\config.lua'
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
        throw 'Close DragonSword Awakening before installing or regenerating DragonSwordWorldRadarObjectState data.'
    }
    if (Test-Path -LiteralPath $log) {
        $archive = Join-Path $archiveDir ('DragonSwordWorldRadar.Install.' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff') + '.log')
        Move-Item -LiteralPath $log -Destination $archive -Force
    }
    $releasePath = Join-Path $metadataRoot 'release.json'
    if (-not (Test-Path -LiteralPath $releasePath)) { throw 'metadata\release.json is missing.' }
    $release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
    $releaseVersion = [string]$release.version
    if ([string]::IsNullOrWhiteSpace($releaseVersion)) { throw 'metadata\release.json does not define a release version.' }
    Log ("INSTALL_START version={0} mode=event-driven-single-motion-bridge-v6" -f $releaseVersion)

    # A normal Windows folder overwrite does not remove files that were
    # deleted from the new release. Remove the exact 1.7 sources retired by
    # the single-Bridge architecture before recursive Add-Type discovery, or
    # stale duplicate models/readers would make an otherwise valid upgrade
    # fail to compile. User config, overrides, logs, and generated data are not
    # touched.
    $obsoleteRelativePaths = @(
        'scripts\boss_tracker.lua',
        'src\overlay\Bridge\StaticStateBridgeReader.cs',
        'src\overlay\Models\RadarState.cs',
        'src\overlay\SaveData\BossAvailabilityTracker.cs',
        'src\overlay\SaveData\AssaultAvailabilityTracker.cs'
    )
    foreach ($obsoleteRelativePath in $obsoleteRelativePaths) {
        $obsoletePath = Join-Path $modRoot $obsoleteRelativePath
        if (Test-Path -LiteralPath $obsoletePath -PathType Leaf) {
            Remove-Item -LiteralPath $obsoletePath -Force -ErrorAction Stop
            if (Test-Path -LiteralPath $obsoletePath) {
                throw "Could not remove obsolete 1.7 file: $obsoleteRelativePath"
            }
            Log ("REMOVED_OBSOLETE path={0}" -f $obsoleteRelativePath)
        }
    }

    $layout = Resolve-DragonSwordWorldRadarGameLayout -ModDir $modRoot
    Log ("GAME_LAYOUT root={0}; exe={1}; pak={2}; oodle={3}" -f $layout.GameRoot,$layout.ExecutablePath,$layout.PakPath,$layout.OodleLibraryPath)

    $bundledOozPath = Join-Path $modRoot 'tools\ooz.exe'
    $bundledOozMetadata = @($release.bundled_tools |
        Where-Object { [string]$_.name -ieq 'ooz.exe' } |
        Select-Object -First 1)
    if ($bundledOozMetadata.Count -ne 1 -or
        [string]::IsNullOrWhiteSpace([string]$bundledOozMetadata[0].sha256)) {
        throw 'metadata\release.json does not define the bundled ooz.exe SHA-256.'
    }
    $bundledOozSha256 =
        ([string]$bundledOozMetadata[0].sha256).ToLowerInvariant()
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
    $userConfig = Join-Path $modRoot 'scripts\config.lua'
    if (-not (Test-Path -LiteralPath $userConfig) -and (Test-Path -LiteralPath (Join-Path $oldModRoot 'scripts\config.lua'))) {
        Copy-Item -LiteralPath (Join-Path $oldModRoot 'scripts\config.lua') -Destination $userConfig
        Log 'MIGRATED config.lua from legacy eventrader folder'
    }
    if (-not (Test-LuaConfig -Path $userConfig)) {
        throw 'The single authoritative scripts\config.lua is missing or malformed. Restore it from a complete release package before installation.'
    }
    Log 'VALIDATED single authoritative scripts\config.lua'
    Migrate-AssaultConfig -UserPath $userConfig
    if (-not (Test-LuaConfig -Path $userConfig)) {
        throw 'scripts\config.lua failed validation after legacy-setting cleanup.'
    }

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

    # These records are valid. Remove only the obsolete stock rules and their
    # exact stock explanations while preserving every other user override.
    $overrideLines = @(Get-Content -LiteralPath $overridePath)
    $cleanOverrideLines = @($overrideLines | Where-Object {
        $_ -notmatch '^\s*ignore\s+10220122\s*(?:#.*)?$' -and
        $_ -notmatch '^\s*#\s*Known abandoned or inaccessible chest record\.\s*$' -and
        $_ -notmatch '^\s*ignore\s+11003\s*(?:#.*)?$' -and
        $_ -notmatch '^\s*#\s*Known abandoned duplicate: 11003 overlaps 14016 at the same chest location\.\s*$' -and
        $_ -notmatch '^\s*#\s*Keep the authoritative 14016 record and suppress the offset duplicate\.\s*$'
    })
    if ($cleanOverrideLines.Count -ne $overrideLines.Count) {
        [IO.File]::WriteAllLines(
            $overridePath,
            [string[]]$cleanOverrideLines,
            $utf8)
        Log 'REMOVED obsolete treasure override rules for valid records 10220122/11003'
    }

    Stop-ExistingWatcher

    # Compile the exact overlay source set used by F7 before installing the
    # watcher. This turns missing methods and warning-as-error failures into
    # installation failures instead of a silent no-overlay runtime.
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    $overlaySourceRoot = Join-Path $modRoot 'src\overlay'
    $overlaySources = @(Get-ChildItem -LiteralPath $overlaySourceRoot -Recurse -Filter '*.cs' -File |
        Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' } |
        Sort-Object FullName | Select-Object -ExpandProperty FullName)
    if ($overlaySources.Count -ne 42) {
        throw "Overlay source set is incomplete: $($overlaySources.Count) files."
    }
    $overlayReferences = @(
        [System.Windows.Forms.Form].Assembly.Location,
        [System.Drawing.Graphics].Assembly.Location,
        [System.Linq.Enumerable].Assembly.Location,
        [System.Uri].Assembly.Location
    ) | Select-Object -Unique
    Log ("OVERLAY_COMPILE sources={0}" -f $overlaySources.Count)
    Add-Type -Path $overlaySources -ReferencedAssemblies $overlayReferences -ErrorAction Stop
    Log 'OVERLAY_COMPILE_OK'

    $identityBeforeData = Get-DragonSwordWorldRadarGameIdentity -Layout $layout
    $results = @()

    if ($layout.OodleLibraryPath) {
        $installerSources = @(Get-ChildItem -LiteralPath (Join-Path $modRoot 'src\installer') -Recurse -Filter '*.cs' -File |
            Where-Object { $_.FullName -notmatch '[\\/](?:obj|bin)[\\/]' } |
            Sort-Object FullName | Select-Object -ExpandProperty FullName)
        if ($installerSources.Count -ne 20) { throw "Installer source set is incomplete: $($installerSources.Count) files." }
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
        throw 'The matching game Oodle library is required to generate the native treasure actor catalog.'
    }

    $identityAfterData = Get-DragonSwordWorldRadarGameIdentity -Layout $layout
    if ([string]$identityAfterData.fingerprint -ne [string]$identityBeforeData.fingerprint) {
        throw 'Game executable or PAK identity changed while datasets were being generated. Installation failed closed; retry after the game update is stable.'
    }

    $ownerPointerConfigPath = Join-Path $generatedRoot 'save_owner_pointer.cfg'
    if (-not (Test-Path -LiteralPath $ownerPointerConfigPath -PathType Leaf)) {
        throw 'Installer-generated save owner-pointer configuration is missing.'
    }
    $ownerPointerConfigText = [IO.File]::ReadAllText($ownerPointerConfigPath)
    # File.WriteAllText uses the platform newline. Accept both LF and CRLF
    # without allowing trailing field data into the fingerprint-bound values.
    $ownerFingerprintMatch = [regex]::Match($ownerPointerConfigText,'(?m)^game_fingerprint=(?<value>[0-9a-f]{64})\r?$')
    $ownerLengthMatch = [regex]::Match($ownerPointerConfigText,'(?m)^executable_length=(?<value>[1-9][0-9]*)\r?$')
    $ownerRvaMatch = [regex]::Match($ownerPointerConfigText,'(?m)^owner_pointer_rva=0x(?<value>[0-9A-F]+)\r?$')
    if (-not $ownerFingerprintMatch.Success -or
        -not $ownerLengthMatch.Success -or
        -not $ownerRvaMatch.Success -or
        $ownerFingerprintMatch.Groups['value'].Value -ne [string]$identityBeforeData.fingerprint -or
        [int64]$ownerLengthMatch.Groups['value'].Value -ne [int64]$identityBeforeData.executable_length) {
        throw 'Installer-generated save owner-pointer configuration is malformed or not bound to the locked game fingerprint.'
    }
    $ownerRva = [Convert]::ToUInt64($ownerRvaMatch.Groups['value'].Value,16)
    if ($ownerRva -le 0 -or $ownerRva -ge [uint64]$identityBeforeData.executable_length) {
        throw 'Installer-generated save owner-pointer RVA is outside executable bounds.'
    }
    Log ("SAVE_OWNER_POINTER_GENERATED source=exact-executable-pattern; fingerprint={0}; rva=0x{1:X}; keyPersisted=false" -f $identityBeforeData.fingerprint,$ownerRva)

    $datasetEntries = @()
    foreach ($result in $results) {
        if (-not (Test-Path -LiteralPath $result.OutputPath)) { throw "Generated dataset is missing: $($result.OutputPath)" }
        if ([string]$result.GameFingerprint -ne [string]$identityBeforeData.fingerprint) {
            throw "Dataset $($result.Id) was not generated from the locked pre-generation game fingerprint."
        }
        $datasetEntries += [ordered]@{
            id = [string]$result.Id
            record_count = [int]$result.RecordCount
            output = $result.OutputPath.Substring($modRoot.Length).TrimStart('\')
            sha256 = (Get-FileHash -LiteralPath $result.OutputPath -Algorithm SHA256).Hash.ToLowerInvariant()
            source_pak = [IO.Path]::GetFileName([string]$result.SourcePak)
            source_entry = [string]$result.SourceEntry
            game_fingerprint = [string]$result.GameFingerprint
        }
        Log ("DATASET_GENERATED id={0}; records={1}; output={2}" -f $result.Id,$result.RecordCount,$result.OutputPath)
    }
    Write-DragonSwordWorldRadarJson -Path (Join-Path $metadataRoot 'datasets.json') -Value ([ordered]@{
        schema_version = 2
        generated_at_utc = [DateTime]::UtcNow.ToString('O')
        game_fingerprint_before_generation = [string]$identityBeforeData.fingerprint
        game_fingerprint_after_generation = [string]$identityAfterData.fingerprint
        providers = $datasetEntries
    })

    $catalogPath = Join-Path $generatedRoot 'treasures.lua'
    $catalogText = [IO.File]::ReadAllText($catalogPath)
    $recordCount = [regex]::Matches($catalogText,'(?m)\bsave_id\s*=').Count
    if ($recordCount -lt 1000 -or $catalogText -notmatch '\bz\s*=' -or $catalogText -notmatch '\buid_name\s*=' -or $catalogText -notmatch '\bgroup_id\s*=') {
        throw "Generated treasure catalog validation failed (records=$recordCount)."
    }
    $actorCatalogPath = Join-Path $generatedRoot 'treasure-actors.tsv'
    if (-not (Test-Path -LiteralPath $actorCatalogPath -PathType Leaf)) {
        throw 'Installer-generated native treasure actor catalog is missing.'
    }
    $actorCatalogLines = @(Get-Content -LiteralPath $actorCatalogPath)
    $actorRecordCount = [Math]::Max(0, $actorCatalogLines.Count - 1)
    if ($actorCatalogLines[0] -ne "SaveId`tClassName`tX`tY`tZ" -or
        $actorRecordCount -lt 1600 -or
        $actorRecordCount -gt 2500 -or
        $recordCount - $actorRecordCount -lt 0 -or
        $recordCount - $actorRecordCount -gt 2) {
        throw "Generated native treasure actor catalog validation failed (records=$actorRecordCount)."
    }
    Log ("NATIVE_TREASURE_ACTOR_CATALOG_VALIDATED records={0}; unresolved={1}; source=current-game-pak" -f $actorRecordCount,($recordCount-$actorRecordCount))

    foreach ($nativeEncounter in @(@{ Name='boss-actors.tsv'; Count=9 }, @{ Name='assault-actors.tsv'; Count=40 })) {
        $nativeEncounterPath = Join-Path $generatedRoot $nativeEncounter.Name
        if (-not (Test-Path -LiteralPath $nativeEncounterPath -PathType Leaf)) {
            throw "Installer-generated native encounter actor catalog is missing: $($nativeEncounter.Name)."
        }
        $nativeEncounterLines = @(Get-Content -LiteralPath $nativeEncounterPath)
        if ($nativeEncounterLines[0] -ne "Id`tClassName`tX`tY`tZ" -or
            $nativeEncounterLines.Count - 1 -ne $nativeEncounter.Count -or
            @($nativeEncounterLines | Select-Object -Skip 1 | Where-Object { $_ -notmatch '^\d+\t[^\t]+_C\t-?\d+(?:\.\d+)?\t-?\d+(?:\.\d+)?\t-?\d+(?:\.\d+)?$' }).Count -ne 0) {
            throw "Generated native encounter actor catalog validation failed: $($nativeEncounter.Name)."
        }
    }
    Log 'NATIVE_ENCOUNTER_ACTOR_CATALOG_VALIDATED bosses=9; assaults=40; source=current-game-pak'

    $bossCatalogPath = Join-Path $generatedRoot 'bosses.lua'
    if (-not (Test-Path -LiteralPath $bossCatalogPath -PathType Leaf)) {
        throw 'Generated world-boss catalog is missing.'
    }
    $bossCatalogText = [IO.File]::ReadAllText($bossCatalogPath)
    $bossRecordCount = [regex]::Matches($bossCatalogText,'(?m)\bboss_id\s*=').Count
    if (
        $bossRecordCount -ne 9 -or
        $bossCatalogText -notmatch '\bmap_id\s*=' -or
        $bossCatalogText -notmatch '\buid_name\s*=' -or
        $bossCatalogText -notmatch 'Icon_Mark_FieldBoss_Sprite'
    ) {
        throw "Generated world-boss catalog validation failed (records=$bossRecordCount)."
    }

    $assaultCatalogPath = Join-Path $generatedRoot 'assaults.lua'
    if (-not (Test-Path -LiteralPath $assaultCatalogPath -PathType Leaf)) {
        throw 'Generated Assault catalog is missing.'
    }
    $assaultCatalogText = [IO.File]::ReadAllText($assaultCatalogPath)
    $assaultRecordCount = [regex]::Matches($assaultCatalogText,'(?m)^\s*\{\s*place_id\s*=').Count
    $assaultConditionCount = [regex]::Matches($assaultCatalogText,'\bcondition_type\s*=\s*"world_time_window"').Count
    $assaultPolicyPath = Join-Path $metadataRoot 'assault-inference-policy.xml'
    if (-not (Test-Path -LiteralPath $assaultPolicyPath -PathType Leaf)) {
        throw 'Assault inference policy is missing.'
    }
    [xml]$assaultPolicy = [IO.File]::ReadAllText($assaultPolicyPath)
    $policyRoot = $assaultPolicy.DocumentElement
    if ($null -eq $policyRoot -or $policyRoot.LocalName -ne 'AssaultInferencePolicy' -or
        [string]$policyRoot.schema_version -ne '1' -or
        [string]$policyRoot.expected_game_fingerprint -ne [string]$identityBeforeData.fingerprint) {
        throw 'Assault inference policy schema or game fingerprint is invalid.'
    }
    $policyConditions = @($policyRoot.SelectNodes('./Condition'))
    $seenPolicyPlaces = @{}
    foreach ($condition in $policyConditions) {
        foreach ($field in @('place_id','cid','uid','uid_name','reveal_cycle_id','condition_type','provenance','missing_confirmation')) {
            if ([string]::IsNullOrWhiteSpace([string]$condition.GetAttribute($field))) {
                throw "Assault inference policy condition is missing $field."
            }
        }
        if ([string]$condition.condition_type -ne 'world_time_window' -or
            $seenPolicyPlaces.ContainsKey([string]$condition.place_id)) {
            throw 'Assault inference policy contains an unknown type or duplicate target selector.'
        }
        $seenPolicyPlaces[[string]$condition.place_id] = $true
    }
    $expectedAssaultConditionCount = $policyConditions.Count
    $escapedGenerationFingerprint = [regex]::Escape([string]$identityBeforeData.fingerprint)
    $assaultFingerprintCount = [regex]::Matches($assaultCatalogText,
        ('(?m)^\s*\{[^\r\n]*\bgame_fingerprint\s*=\s*"' + $escapedGenerationFingerprint + '"')).Count
    if ($assaultRecordCount -ne 40 -or $assaultConditionCount -ne $expectedAssaultConditionCount -or $assaultFingerprintCount -ne 40) {
        throw "Generated Assault catalog validation failed (records=$assaultRecordCount; conditions=$assaultConditionCount/$expectedAssaultConditionCount; fingerprints=$assaultFingerprintCount)."
    }
    Log ("ASSAULT_CATALOG_VALIDATED records=40; conditioned={0}; fingerprint={1}; source=install-generated-current-game-pak" -f $expectedAssaultConditionCount,$identityBeforeData.fingerprint)
    Log ("WORLD_ENCOUNTER_CATALOG_VALIDATED records=49; bosses=9; assaults=40; conditioned={0}; runtime=single-startup-fixed-catalog-query-cache" -f $expectedAssaultConditionCount)

    $moleCatalogPath = Join-Path $generatedRoot 'moles.lua'
    if (-not (Test-Path -LiteralPath $moleCatalogPath -PathType Leaf)) {
        throw 'Generated Mole/Fly catalog is missing.'
    }
    $moleCatalogText = [IO.File]::ReadAllText($moleCatalogPath)
    $moleMatches = [regex]::Matches($moleCatalogText,
        '(?m)^\s*\{[^\r\n]*\bmini_game_id\s*=\s*(?<id>110(?:0[1-9]|[12][0-9]|3[0-4]))\b[^\r\n]*\breward_save_id\s*=\s*(?<reward>[1-9]\d*)\b[^\r\n]*\bmask_bit\s*=\s*(?<bit>\d+)\b[^\r\n]*\bmap_id\s*=\s*(?<map>[1-9]\d*)\b[^\r\n]*\bposition_role\s*=\s*"(?:NPC_Start|Teleport_Start|Fly_Linked)"[^\r\n]*\bnotice_title\s*=\s*"109208"[^\r\n]*\bnotice_description\s*=\s*"109202"')
    if ($moleMatches.Count -ne 33) {
        throw "Generated Fly catalog must contain exactly 33 ordinary-world records; found $($moleMatches.Count)."
    }
    $seenMoleIds = @{}
    $seenMoleRewards = @{}
    $seenMoleBits = @{}
    foreach ($match in $moleMatches) {
        $moleId = [int]$match.Groups['id'].Value
        $rewardSaveId = [long]$match.Groups['reward'].Value
        $maskBit = [int]$match.Groups['bit'].Value
        if ($seenMoleIds.ContainsKey($moleId) -or $seenMoleRewards.ContainsKey($rewardSaveId) -or $seenMoleBits.ContainsKey($maskBit)) {
            throw 'Generated Mole/Fly catalog contains duplicate IDs, reward save IDs, or mask bits.'
        }
        $seenMoleIds[$moleId] = $true
        $seenMoleRewards[$rewardSaveId] = $true
        $seenMoleBits[$maskBit] = $true
    }
    for ($maskBit = 0; $maskBit -lt 33; $maskBit++) {
        if (-not $seenMoleBits.ContainsKey($maskBit)) {
            throw "Generated Mole/Fly catalog is missing contiguous mask bit $maskBit."
        }
    }
    $additionalMatches = [regex]::Matches($moleCatalogText,
        '(?m)^\s*\{[^\r\n]*\bmini_game_id\s*=\s*(?<id>12(?:00[1-9]|0[1-3][0-9]|040)|130(?:0[1-9]|10))\b[^\r\n]*\breward_save_id\s*=\s*(?<reward>[1-9]\d*)\b[^\r\n]*\bmask_bit\s*=\s*-1\b[^\r\n]*\bmini_game_type\s*=\s*"(?<type>mole|wave)"[^\r\n]*\bmap_id\s*=\s*(?<map>[1-9]\d*)\b[^\r\n]*\bposition_role\s*=\s*"(?:NPC_Start|Teleport_Start)"')
    if ($additionalMatches.Count -ne 50) {
        throw "Generated Mole/Wave catalog must contain exactly 50 shape-valid records; found $($additionalMatches.Count)."
    }
    $moleTypeCount = 0
    $waveTypeCount = 0
    foreach ($match in $additionalMatches) {
        $miniGameId = [int]$match.Groups['id'].Value
        $rewardSaveId = [long]$match.Groups['reward'].Value
        $sharedWaveAlias = $miniGameId -ge 13008 -and $miniGameId -le 13010 -and $rewardSaveId -eq 13008
        if ($seenMoleIds.ContainsKey($miniGameId) -or ($seenMoleRewards.ContainsKey($rewardSaveId) -and -not $sharedWaveAlias)) {
            throw 'Generated mini-game catalog contains an unsupported duplicate ID or reward save ID.'
        }
        $seenMoleIds[$miniGameId] = $true
        $seenMoleRewards[$rewardSaveId] = $true
        if ($match.Groups['type'].Value -eq 'mole') { $moleTypeCount++ } else { $waveTypeCount++ }
    }
    if ($moleTypeCount -ne 40 -or $waveTypeCount -ne 10) {
        throw "Generated mini-game type counts changed (Mole=$moleTypeCount; Wave=$waveTypeCount)."
    }
    Log "MINIGAME_CATALOG_VALIDATED records=83; fly=33; mole=40; wave=10; rewardMappings=83; source=install-generated-current-game-pak"

    $identity = Get-DragonSwordWorldRadarGameIdentity -Layout $layout
    if ([string]$identity.fingerprint -ne [string]$identityBeforeData.fingerprint) {
        throw 'Game executable or PAK identity changed after dataset validation. Install-state was not written.'
    }
    Write-DragonSwordWorldRadarJson -Path (Join-Path $metadataRoot 'install-state.json') -Value ([ordered]@{
        schema_version = 1
        mod_version = $releaseVersion
        installed_at_utc = [DateTime]::UtcNow.ToString('O')
        game_root = $layout.GameRoot
        game = $identity
        datasets_manifest = 'metadata\datasets.json'
        save_owner_pointer_config = 'data\generated\save_owner_pointer.cfg'
    })
    Log ("GAME_FINGERPRINT version={0}; fingerprint={1}" -f $identity.display_version,$identity.fingerprint)

    $modsRoot = Split-Path -Parent $modRoot
    $modsFile = Join-Path $modsRoot 'mods.txt'
    if (Test-Path -LiteralPath $modsFile) {
        $reader = New-Object -TypeName IO.StreamReader -ArgumentList @($modsFile,$true)
        try {
            $modsText = $reader.ReadToEnd()
            $modsEncoding = $reader.CurrentEncoding
        }
        finally {
            $reader.Dispose()
        }
    }
    else {
        $modsText = ''
        $modsEncoding = $utf8
    }

    $modPattern = '(?m)^[ \t]*DragonSwordWorldRadarObjectState[ \t]*:[ \t]*([01])'
    $existingModMatch = [regex]::Match($modsText,$modPattern)
    $updatedModsText = $modsText
    $configuredModValue = $null
    if ($existingModMatch.Success) {
        $configuredModValue = $existingModMatch.Groups[1].Value
        Log ("MOD_SETTING_PRESERVED DragonSwordWorldRadarObjectState={0}; all mods.txt bytes preserved" -f $configuredModValue)
    }
    else {
        $newline = if ($modsText -match "`r`n") { "`r`n" } else { "`n" }
        if (
            $updatedModsText.Length -gt 0 -and
            -not $updatedModsText.EndsWith("`n") -and
            -not $updatedModsText.EndsWith("`r")
        ) {
            $updatedModsText += $newline
        }
        $updatedModsText += 'DragonSwordWorldRadarObjectState : 1' + $newline
        $configuredModValue = '1'
        [IO.File]::WriteAllText($modsFile,$updatedModsText,$modsEncoding)
        Log 'MOD_SETTING_CREATED DragonSwordWorldRadarObjectState=1'
    }


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

    # Remove obsolete pre-0.4.0 files after migration.
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
    Stop-ExistingWatcher
    Log 'SESSION_WATCHER_READY mode=ue4ss-launched lifetime=game-process'

    $allExecutables = @(Get-ChildItem -LiteralPath $modRoot -Filter '*.exe' -File -Recurse -ErrorAction SilentlyContinue)
    $unexpectedExecutables = @($allExecutables | Where-Object { [IO.Path]::GetFullPath($_.FullName) -ne [IO.Path]::GetFullPath($bundledOozPath) })
    if ($unexpectedExecutables.Count -gt 0) {
        throw ('Unexpected EXE files are not allowed: ' + (($unexpectedExecutables.FullName) -join ', '))
    }
    if ($allExecutables.Count -ne 1) { throw "Expected exactly one bundled tool executable, found $($allExecutables.Count)." }

    Log ("INSTALL_COMPLETE version={0} datasets=treasure,boss,assault,mole encounterRecords=49 assaultRecords=40 encounterConditioned={1} customExe=0 bundledToolExe=1 watcher=per-session-wscript transientPowerShell=true" -f $releaseVersion,$expectedAssaultConditionCount)
    Write-Host ("DragonSwordWorldRadarObjectState {0} installed. Generated {1} treasure, {2} world-boss, 40 Assault ({3} conditioned), and 83 mini-game records (33 Fly, 40 Mole, 10 Wave) for game version {4}." -f $releaseVersion,$recordCount,$bossRecordCount,$expectedAssaultConditionCount,$identity.display_version)
    Write-Host 'Start the game normally. Overlay startup fixes one 49-record Boss/Assault encounter catalog and save-query shape. F7 only enables configured marker, save-state, and isolated world-clock work with normal rendering; F8 disables all active mod work for FPS comparison. When debug_logging is true, F5 adds no-paint isolation and F6 adds frozen-motion isolation. The clock performs one read after stable context and then advances locally.'
} catch {
    try { Log ("INSTALL_FAILED " + ($_ | Out-String)) } catch {}
    Write-Error $_
    exit 1
}
