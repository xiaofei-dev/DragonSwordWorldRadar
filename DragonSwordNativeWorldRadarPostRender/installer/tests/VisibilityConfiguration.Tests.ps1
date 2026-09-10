[CmdletBinding()]
param([string]$InstallerExe)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if ([string]::IsNullOrWhiteSpace($InstallerExe)) {
    # Compile only the engine into this test process. No Setup executable,
    # embedded payload, release package, or game-directory write is needed.
    $types = @(Add-Type -Path (Join-Path $projectRoot 'installer\InstallerEngine.cs') `
        -ReferencedAssemblies @('System.dll', 'System.Core.dll',
            'System.Web.Extensions.dll', 'System.IO.Compression.dll',
            'System.IO.Compression.FileSystem.dll') -PassThru)
    $engine = $types | Where-Object {
        $_.FullName -eq 'DragonSwordNativeWorldRadarPostRender.Installer.InstallerEngine'
    } | Select-Object -First 1
} else {
    $assembly = [Reflection.Assembly]::LoadFrom(
        (Resolve-Path -LiteralPath $InstallerExe).ProviderPath)
    $engine = $assembly.GetType(
        'DragonSwordNativeWorldRadarPostRender.Installer.InstallerEngine', $true, $false)
}
if ($null -eq $engine) { throw 'Installer engine compilation/loading failed.' }
$validate = $engine.GetMethod('ValidateVisibilityConfig',
    [Reflection.BindingFlags]'Static, NonPublic')
if ($null -eq $validate) { throw 'Visibility validator reflection seam is missing.' }
$script:assertions = 0

function Config([string]$Text, [bool]$Accept, [bool]$Public = $false) {
    $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
    $before = [Convert]::ToBase64String($bytes)
    $accepted = $true
    try {
        [void]$validate.Invoke($null, [object[]]@($bytes, $Public))
    } catch {
        $cause = $_.Exception
        while ($null -ne $cause.InnerException) { $cause = $cause.InnerException }
        if ($cause -isnot [IO.InvalidDataException]) { throw }
        $accepted = $false
    }
    if ($accepted -ne $Accept) {
        throw "Visibility acceptance mismatch: public=$Public expected=$Accept text=$Text"
    }
    if ([Convert]::ToBase64String($bytes) -cne $before) {
        throw 'Visibility validation changed original bytes.'
    }
    $script:assertions += 2
}

$base = "[radar]`nclock=true`ntreasure=true`nboss=true`nassault=true`n" +
    "mini_games=true`narea_quests=true`nbird_eggs=true`n" +
    "[map]`ntreasure=true`nboss=true`nassault=true`nmini_games=true`narea_quests=true`n" +
    "[modes]`narea_quests=available`nassault=available`n"
$oldHeight = "[height_arrows]`ntreasure=false`narea_quests=true`nmole=false`n"
$interface = "[interface]`nlanguage=ja`n"
$previous = $base + $oldHeight + $interface
$default = [IO.File]::ReadAllText((Join-Path $projectRoot 'config\visibility.ini'))
Config $default $true $true
Config $base $true
Config $previous $true
Config $base $false $true
Config $previous $false $true

foreach ($legacy in @(
        "compact_mask=3`nworld_mask=4`n",
        "schema_version=1`ncompact_mask=63`nworld_mask=62`n",
        "schema_version=2`ncompact_mask=5`nworld_mask=6`n",
        "schema_version=3`ncompact_mask=7`nworld_mask=10`narea_quest_mode=all`n",
        "schema_version=4`ncompact_mask=11`nworld_mask=18`narea_quest_mode=available`nassault_mode=current`n")) {
    Config $legacy $true
    Config ([string][char]0xFEFF + $legacy.Replace("`n", "`r`n")) $true
    Config $legacy $false $true
}
foreach ($language in @('auto', 'en', 'ja', 'ko', 'zh-hans', 'zh-hant',
        'fr', 'de', 'es-es', 'ru', 'th', 'pt-br')) {
    Config ($base + $oldHeight + "[interface]`nlanguage=$language`n") $true
}
foreach ($treasure in @('false', 'true')) {
    foreach ($area in @('false', 'true')) {
        $scene = "[scene]`ntreasure=$treasure`narea_quests=$area`n"
        Config ($base + $scene) $true
        Config ($previous + $scene) $true
        foreach ($boss in @('false', 'true')) {
            foreach ($assault in @('false', 'true')) {
                $current = $base + $scene + $oldHeight +
                    "boss=$boss`nassault=$assault`n" + $interface
                Config $current $true
                Config ([string][char]0xFEFF + "# preserve user choices`r`n" +
                    $current.Replace("`n", "`r`n")) $true
            }
        }
    }
}
Config ($base + $oldHeight + "boss=false`n" + $interface) $true
Config ($base + $oldHeight + "assault=false`n" + $interface) $true
Config ($default.Replace("boss=true", "boss=false")) $false $true
Config ($default.Replace("assault=true", "assault=false")) $false $true
# New public presets enable Scene, while previously saved disabled choices
# remain accepted without rewriting their bytes. Change only the Scene block.
$sceneStart = $default.IndexOf('[scene]')
$sceneEnd = $default.IndexOf('[modes]', $sceneStart)
foreach ($sceneKey in @('treasure', 'area_quests', 'mini_games')) {
    $savedOff = $default.Substring(0, $sceneStart) +
        $default.Substring($sceneStart, $sceneEnd - $sceneStart).Replace("$sceneKey=true", "$sceneKey=false") +
        $default.Substring($sceneEnd)
    Config $savedOff $true
    Config $savedOff $false $true
}
# Complete six-section files remain upgrade-readable with either new height
# field missing, while immutable public defaults must include all five.
Config ($default.Replace("mole=true", "mole=true#invalid")) $false
$lfDefault = $default.Replace("`r`n", "`n")
foreach ($key in @('boss', 'assault')) {
    $heightStart = $lfDefault.IndexOf('[height_arrows]')
    $beforeHeight = $lfDefault.Substring(0, $heightStart)
    $heightAndInterface = $lfDefault.Substring($heightStart).Replace("$key=true`n", '')
    Config ($beforeHeight + $heightAndInterface) $true
    Config ($beforeHeight + $heightAndInterface) $false $true
}

foreach ($badScene in @(
        "[scene]`n", "[scene]`ntreasure=true`n",
        "[scene]`narea_quests=false`n",
        "[scene]`ntreasure=TRUE`narea_quests=false`n",
        "[scene]`ntreasure=true`narea_quests=1`n",
        "[scene]`ntreasure=true`narea_quests=false`nboss=true`n",
        "[scene]`ntreasure=true`narea_quests=false`nclock=true`n",
        "[scene]`ntreasure=true`ntreasure=false`narea_quests=true`n",
        "[scene]`ntreasure=true`narea_quests=false`n[scene]`n",
        "[Scene]`ntreasure=true`narea_quests=false`n")) {
    Config ($previous + $badScene) $false
}
foreach ($badHeight in @('boss=1', 'assault=TRUE', ("boss=false`nboss=true"),
        ("assault=true`nassault=false"), 'unknown=true')) {
    Config ($base + $oldHeight + $badHeight + "`n" + $interface) $false
}
foreach ($bad in @('', ($base + $oldHeight), ($base + $interface),
        $previous.Replace('language=ja', 'language=unknown'),
        $previous.Replace("mole=false`n", ''),
        ($previous + "[unknown]`n"), ($previous + "language=auto`n"),
        $previous.Replace("`n", "`r"),
        "schema_version=5`ncompact_mask=127`nworld_mask=62`n",
        "compact_mask=128`nworld_mask=62`n", ('#' + ('x' * 4096)))) {
    Config $bad $false
}
Config ($lfDefault + '#' + ('x' * (4095 - $lfDefault.Length))) $true
foreach ($mode in @('off', 'central_radius', 'nearest_center', 'all')) {
    foreach ($range in @(0, 600, 1000)) {
        foreach ($limit in @(0, 24, 50)) {
            foreach ($miniGames in @('true', 'false')) {
                Config ($previous + "[scene]`ntreasure=true`narea_quests=false`n" +
                    "mini_games=$miniGames`nrange_meters=$range`nmarker_limit=$limit`ndistance_mode=$mode`n") $true
            }
        }
    }
}
foreach ($key in @('mini_games', 'range_meters', 'marker_limit', 'distance_mode')) {
    $missing = [regex]::Replace($lfDefault, '(?m)^' + $key + '=[^\n]*\n', '')
    # mini_games also occurs in Radar/Map: remove only the new Scene field.
    if ($key -eq 'mini_games') {
        $sceneStart = $lfDefault.IndexOf('[scene]')
        $modeStart = $lfDefault.IndexOf('[modes]', $sceneStart)
        $missing = $lfDefault.Substring(0, $sceneStart) +
            $lfDefault.Substring($sceneStart, $modeStart - $sceneStart).Replace("mini_games=true`n", '') +
            $lfDefault.Substring($modeStart)
    }
    Config $missing $true
    Config $missing $false $true
}
foreach ($bad in @('mini_games=TRUE', 'range_meters=1001', 'range_meters=-1',
        'range_meters=1.5', 'marker_limit=51', 'marker_limit=-1',
        'range_meters=4294967296', 'distance_mode=center', 'distance_mode=ALL',
        "range_meters=0`nrange_meters=1", "marker_limit=1`nmarker_limit=2",
        "distance_mode=off`ndistance_mode=all", "mini_games=true`nmini_games=false")) {
    Config ($previous + "[scene]`ntreasure=true`narea_quests=false`n$bad`n") $false
}
Config ($lfDefault.Replace('range_meters=600', 'range_meters=1000')) $false $true
Config ($lfDefault.Replace('marker_limit=24', 'marker_limit=50')) $false $true
Config ($lfDefault.Replace('distance_mode=nearest_center', 'distance_mode=off')) $false $true
Write-Host "Visibility installer parser: $script:assertions assertions passed; new and legacy formats checked; original bytes unchanged; no package or game writes."
