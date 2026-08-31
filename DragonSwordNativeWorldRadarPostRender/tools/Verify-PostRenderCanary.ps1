[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Remove-CppComments {
    param([string]$Text)
    $withoutBlocks = [regex]::Replace($Text, '/\*[\s\S]*?\*/', '')
    return [regex]::Replace($withoutBlocks, '//[^\r\n]*', '')
}

function Get-NativeOwnerFunction {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?(?:bool|void)\s+' +
        [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "Native owner function was not found: $Name"
    return $match.Value
}

function Get-VisibilityHubMethod {
    param([string]$Text, [string]$Name)
    $pattern =
        '(?ms)^(?:RadarVisibilityHubResult|bool|void)\s+' +
        'RadarVisibilityHub::' + [regex]::Escape($Name) +
        '\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}'
    $match = [regex]::Match($Text, $pattern)
    Assert-True $match.Success "Visibility Hub method was not found: $Name"
    return $match.Value
}

$config = Get-Content (Join-Path $projectRoot 'config\postrender_canary.ini') -Raw
$source = Get-Content (Join-Path $projectRoot 'src\native\postrender_canary.cpp') -Raw
$lateSource = Get-Content (Join-Path $projectRoot 'src\native\late_present_canary.cpp') -Raw
$umgSource = Get-Content (Join-Path $projectRoot 'src\native\umg_minimap_canary.cpp') -Raw
$umgHeader = Get-Content (Join-Path $projectRoot 'src\native\umg_minimap_canary.hpp') -Raw
$main = Get-Content (Join-Path $projectRoot 'src\native\main.cpp') -Raw
$nativeEventLog = Get-Content `
    (Join-Path $projectRoot 'src\native\native_event_log.cpp') -Raw
$visibilityHub = Get-Content `
    (Join-Path $projectRoot 'src\native\radar_visibility_hub.cpp') -Raw
$visibilityHubHeader = Get-Content `
    (Join-Path $projectRoot 'src\native\radar_visibility_hub.hpp') -Raw
$visibilityConfig = Get-Content `
    (Join-Path $projectRoot 'config\visibility.ini') -Raw
$visibilityParser = Get-Content `
    (Join-Path $projectRoot 'include\dswros\visibility_config.hpp') -Raw
$diagnosticsConfig = Get-Content `
    (Join-Path $projectRoot 'config\diagnostics.ini') -Raw
$diagnosticsParser = Get-Content `
    (Join-Path $projectRoot 'include\dswros\diagnostics_config.hpp') -Raw
$diagnosticsFormatter = Get-Content `
    (Join-Path $projectRoot 'include\dswros\diagnostic_log_format.hpp') -Raw
$objectState = Get-Content `
    (Join-Path $projectRoot 'include\dswros\object_state.hpp') -Raw
$nativeTests = Get-Content `
    (Join-Path $projectRoot 'tests\native_state_tests.cpp') -Raw
$cmake = Get-Content (Join-Path $projectRoot 'CMakeLists.txt') -Raw
$deploy = Get-Content (Join-Path $projectRoot 'tools\Deploy-NativePrototype.ps1') -Raw
$metadata = Get-Content (Join-Path $projectRoot 'metadata\release.json') -Raw | ConvertFrom-Json
$mainCode = Remove-CppComments $main
$nativeEventLogCode = Remove-CppComments $nativeEventLog
$visibilityHubCode = Remove-CppComments $visibilityHub

Assert-True ($config -match '(?m)^postrender_hook_enabled=false$') `
    'The PostRender hook must be disabled by default.'
Assert-True ($config -match '(?m)^postrender_canary_enabled=false$') `
    'The PostRender canary must be disabled by default.'
Assert-True ($config -match '(?m)^postrender_relative_marker_enabled=false$') `
    'The relative marker must be disabled by default.'
Assert-True ($config -match '(?m)^postrender_vtable_slot=112$') `
    'The audited candidate slot must remain explicit.'
Assert-True ($config -match '(?m)^late_present_hook_enabled=false$') `
    'The late Present hook must be disabled by default.'
Assert-True ($config -match '(?m)^late_present_canary_enabled=false$') `
    'The late Present canary must be disabled by default.'
Assert-True ($config -match '(?m)^late_present_relative_marker_enabled=false$') `
    'The late Present relative marker must be disabled by default.'
Assert-True ($metadata.name -eq 'DragonSwordNativeWorldRadarPostRender') `
    'Release metadata names the wrong mod.'
Assert-True ($metadata.version -eq '2.1.0') `
    'Release metadata version is stale for the current native milestone.'
Assert-True ($cmake -notmatch 'src/native/late_present_canary\.cpp') `
    'The runtime-rejected late Present source must not be compiled.'
Assert-True ($cmake -notmatch 'src/native/postrender_canary\.cpp' `
    -and $main -notmatch `
        'PostRenderCanary|postrender_canary_|publish_render_marker|SharedPublisher|CreateFileMappingW|MapViewOfFile|publisher_') `
    'Retired PostRender and external shared-memory diagnostics must not be compiled or connected to the production owner.'
Assert-True ($cmake -notmatch '\bd3d12\b') `
    'The dev6 native target must not link D3D12.'
Assert-True ($main -notmatch 'LatePresentCanary') `
    'The dev6 owner must not construct the rejected late Present path.'
Assert-True ($umgSource -match 'FindFirstOf\(L"DLayerMiniMap"\)') `
    'The bounded activation-time minimap lookup is missing.'
Assert-True ($umgSource -match 'L"/Script/UMG.UserWidget:AddToViewport"') `
    'The canary must bypass the rejected minimap child composition.'
Assert-True ($umgSource -match 'WidgetBlueprintLibrary:Create') `
    'The canary must construct the game widget through its Blueprint lifecycle.'
Assert-True ($umgSource -match 'GetParmsSize\(\) != size') `
    'The reflected viewport-widget ABI must fail closed before ProcessEvent.'
Assert-True ($umgSource -match 'set_position_in_viewport_, 17') `
    'SetPositionInViewport must use Unreal reflected ParmsSize without C++ tail padding.'
Assert-True ($umgSource -match '/Script/UMG\.Widget:GetOwningPlayer') `
    'GetOwningPlayer must be resolved from its reflected UWidget owner.'
Assert-True ($umgSource -match 'L"PlayerIcon_MiniMap"') `
    'The canary must use the currently visible minimap player image as its visual source.'
Assert-True ($umgSource -match 'copy_property\(source_image, marker_image, L"Brush"\)') `
    'The canary must copy a live game brush before the image is attached.'
Assert-True ($umgSource -match 'attach_attempted_') `
    'The preserved historical UMG witness must remain bounded.'
Assert-True ($umgSource -notmatch 'kAttachRetryWindow|kAttachRetryInterval') `
    'The preserved historical UMG witness must not contain an attachment retry loop.'
Assert-True ($umgSource -match 'RemoveFromParent') `
    'The UMG child cleanup path is missing.'
Assert-True ($umgHeader -match 'FWeakObjectPtr') `
    'The UMG lifecycle must retain weak identities only.'
Assert-True ($deploy -match "'host', 'installer', 'scripts', 'src', 'tools', 'assets'") `
    'The native-only deployment prohibited-path gate is missing.'
Assert-True ($deploy -match "'runtime\\bridge'.*'runtime\\diagnostics'.*'runtime\\launch.request'") `
    'The native-only deployment must quarantine stale overlay runtime triggers.'
Assert-True ($deploy -match "enabled\.txt") `
    'The deployment must neutralize exact enabled.txt auto-start markers.'
Assert-True ($deploy -match 'function\s+Remove-ValidModEntries' `
    -and $deploy -match 'Remove-ValidModEntries\s+\$stableName' `
    -and $deploy -notmatch 'Set-ModState\s+\$stableName\s+0') `
    'The deployment must remove predecessor mods.txt authority without adding a redundant disabled entry.'
Assert-True ($deploy -match `
        '\$matchingIndexes\s*=\s*\[System\.Collections\.Generic\.List\[int\]\]::new\(\)' `
    -and $deploy -match `
        '\$lines\.RemoveAt\(\$matchingIndexes\[\$matchIndex\]\)' `
    -and $deploy -match `
        '\$stableModLines\.Count\s*-ne\s*0' `
    -and $deploy -match `
        '\$nativeModLines\.Count\s*-ne\s*1' `
    -and $deploy -match `
        '\$allRadarModLines\.Count\s*-ne\s*1') `
    'Deployment must collapse duplicate Radar entries and verify the exact final mods.txt states.'
Assert-True ($deploy -match 'TextInfo\.ANSICodePage' `
    -and $deploy -match 'EncoderFallback\]::ExceptionFallback' `
    -and $deploy -match 'DecoderFallback\]::ExceptionFallback' `
    -and $deploy -notmatch '\[System\.Text\.Encoding\]::Default' `
    -and $deploy -match `
        '\$installedModsDocument\s*=\s*Read-ModsTextDocument') `
    'Deployment must use one strict explicit encoding reader for legacy mods.txt mutation and verification.'

$deployTokens = $null
$deployParseErrors = $null
$deployAst = [System.Management.Automation.Language.Parser]::ParseInput(
    $deploy, [ref]$deployTokens, [ref]$deployParseErrors)
Assert-True (@($deployParseErrors).Count -eq 0) `
    'Deployment script could not be parsed for the mods.txt round-trip gate.'
foreach ($functionName in @(
        'Read-ModsTextDocument', 'Write-ModsTextDocumentAtomically')) {
    $definition = $deployAst.Find({
            param($node)
            $node -is `
                [System.Management.Automation.Language.FunctionDefinitionAst] `
                -and $node.Name -eq $functionName
        }, $true)
    Assert-True ($null -ne $definition) `
        "Deployment helper is missing: $functionName"
    . ([scriptblock]::Create($definition.Extent.Text))
}
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)
$strictUtf16Le = [System.Text.UnicodeEncoding]::new(
    $false, $false, $true)
$ansiCodePage = `
    [System.Globalization.CultureInfo]::CurrentCulture.TextInfo.ANSICodePage
$strictAnsi = [System.Text.Encoding]::GetEncoding(
    $ansiCodePage,
    [System.Text.EncoderFallback]::ExceptionFallback,
    [System.Text.DecoderFallback]::ExceptionFallback)
$ansiMarker = [char]0x00E9
$encodingCases = @(
    [pscustomobject]@{
        Name = 'utf8-no-bom-crlf'
        Encoding = $strictUtf8
        Preamble = [byte[]]@()
        Text = "ThirdPartyMod : 1 # e`r`nOtherMod : 0`r`n"
    },
    [pscustomobject]@{
        Name = 'utf8-bom-lf'
        Encoding = $strictUtf8
        Preamble = [byte[]]@(0xEF, 0xBB, 0xBF)
        Text = "ThirdPartyMod : 1 # e`nOtherMod : 0"
    },
    [pscustomobject]@{
        Name = 'utf16le-crlf'
        Encoding = $strictUtf16Le
        Preamble = [byte[]]@(0xFF, 0xFE)
        Text = "ThirdPartyMod : 1 # e`r`nOtherMod : 0"
    },
    [pscustomobject]@{
        Name = 'ansi-lf'
        Encoding = $strictAnsi
        Preamble = [byte[]]@()
        Text = "ThirdPartyMod : 1 # $ansiMarker`nOtherMod : 0`n"
    }
)
$roundTripRoot = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('dsnwr-mods-roundtrip-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $roundTripRoot | Out-Null
try {
    foreach ($case in $encodingCases) {
        $body = $case.Encoding.GetBytes([string]$case.Text)
        $preamble = [byte[]]$case.Preamble
        $fixture = [byte[]]::new($preamble.Length + $body.Length)
        if ($preamble.Length -ne 0) {
            [Array]::Copy($preamble, 0, $fixture, 0, $preamble.Length)
        }
        [Array]::Copy($body, 0, $fixture, $preamble.Length, $body.Length)
        $fixturePath = Join-Path $roundTripRoot ($case.Name + '.txt')
        [System.IO.File]::WriteAllBytes($fixturePath, $fixture)
        $document = Read-ModsTextDocument -Path $fixturePath
        $fixtureLines = [System.Collections.Generic.List[string]]::new()
        foreach ($line in @($document.Lines)) {
            $fixtureLines.Add([string]$line)
        }
        Write-ModsTextDocumentAtomically -Path $fixturePath `
            -Lines $fixtureLines -Document $document
        $actualBytes = [System.IO.File]::ReadAllBytes($fixturePath)
        Assert-True ([Convert]::ToBase64String($actualBytes) -eq `
                [Convert]::ToBase64String($fixture)) `
            "mods.txt exact encoding round-trip failed: $($case.Name)"
    }
    $mixedPath = Join-Path $roundTripRoot 'mixed-eol.txt'
    [System.IO.File]::WriteAllBytes(
        $mixedPath, $strictUtf8.GetBytes("one`r`ntwo`n"))
    $mixedRejected = $false
    try {
        Read-ModsTextDocument -Path $mixedPath | Out-Null
    } catch {
        $mixedRejected = $true
    }
    Assert-True $mixedRejected `
        'mods.txt mixed-line-ending input must fail closed.'
} finally {
    $temporaryPrefix = [System.IO.Path]::GetFullPath(
        [System.IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $roundTripFull = [System.IO.Path]::GetFullPath($roundTripRoot)
    if ($roundTripFull.StartsWith(
            $temporaryPrefix,
            [System.StringComparison]::OrdinalIgnoreCase) `
        -and [System.IO.Path]::GetFileName($roundTripFull) -like `
            'dsnwr-mods-roundtrip-*') {
        Remove-Item -LiteralPath $roundTripFull -Recurse -Force
    }
}

$deploymentGateBlock = [regex]::Match(
    $deploy,
    '(?ms)\$preDeploymentGates\s*=\s*@\([\s\S]*?\)\s*\r?\n\s*#').Value
Assert-True ($deploymentGateBlock -match 'Verify-NativeCompactRenderer\.ps1' `
    -and $deploymentGateBlock -match 'Verify-NativeWorldMapCanary\.ps1' `
    -and $deploymentGateBlock -match 'Verify-PostRenderCanary\.ps1' `
    -and $deploymentGateBlock -match 'Verify-ReleaseHygiene\.ps1') `
    'Deployment does not declare all four required pre-mutation static gates.'
$gateInvocationIndex = $deploy.IndexOf(
    'foreach ($gate in $preDeploymentGates)',
    [StringComparison]::Ordinal)
$firstInstallMutationIndex = $deploy.IndexOf(
    'New-Item -ItemType Directory -Path $backup',
    [StringComparison]::Ordinal)
Assert-True ($gateInvocationIndex -ge 0 `
    -and $firstInstallMutationIndex -gt $gateInvocationIndex `
    -and $deploy -match `
        '(?ms)foreach \(\$gate in \$preDeploymentGates\).*?try\s*\{.*?& \(Join-Path \$PSScriptRoot \$gate\).*?\}\s*catch\s*\{.*?Pre-deployment static gate failed' `
    -and $deploy -notmatch `
        '(?ms)foreach \(\$gate in \$preDeploymentGates\).*?\$LASTEXITCODE') `
    'All static gates must finish successfully before the first install or backup mutation.'
$gameGuard = [regex]::Match(
    $deploy,
    "(?s)function\s+Assert-DragonSwordStopped\s*\{.*?Get-Process\s+-Name\s+'DSClient-Win64-Shipping'.*?\}")
$gameProcessChecks = [regex]::Matches(
    $deploy,
    "Get-Process\s+-Name\s+'DSClient-Win64-Shipping'").Count
$gameGuardCalls = [regex]::Matches(
    $deploy,
    'Assert-DragonSwordStopped\s+-Operation').Count
$mutationArmedIndex = $deploy.IndexOf(
    '$mutationStarted = $true', [StringComparison]::Ordinal)
$finalGameGuardIndex = $deploy.LastIndexOf(
    "Assert-DragonSwordStopped -Operation 'mutate the installed mods'",
    [StringComparison]::Ordinal)
$preMutationGap = if ($finalGameGuardIndex -ge 0 `
        -and $mutationArmedIndex -gt $finalGameGuardIndex) {
    $deploy.Substring(
        $finalGameGuardIndex,
        $mutationArmedIndex - $finalGameGuardIndex)
} else {
    ''
}
$mutationBody = [regex]::Match(
    $deploy,
    '(?ms)^\$mutationStarted\s*=\s*\$true\s*\r?\n(?<body>.*?)^\}\s*catch\s*\{').Groups['body'].Value
Assert-True ($gameGuard.Success `
    -and $gameProcessChecks -eq 1 `
    -and $gameGuardCalls -ge 3 `
    -and $finalGameGuardIndex -ge 0 `
    -and $finalGameGuardIndex -lt $mutationArmedIndex `
    -and $preMutationGap -notmatch `
        'Move-Item|Remove-Item|Copy-Item|New-Item|Write-ModsTextDocumentAtomically|Set-ModState' `
    -and -not [string]::IsNullOrWhiteSpace($mutationBody) `
    -and $mutationBody -notmatch `
        'Assert-DragonSwordStopped|Get-Process') `
    'Deployment must use one stopped-game helper, perform its final check immediately before the first install mutation, and avoid late checks that could trigger rollback writes while the game is running.'
Assert-True ($deploy -match `
        'function\s+Assert-PlainDirectoryIfPresent' `
    -and $deploy -match `
        '\$distRootFull\s*=\s*\[System\.IO\.Path\]::GetFullPath' `
    -and $deploy -match `
        '\$workRootFull\s*=\s*\[System\.IO\.Path\]::GetFullPath' `
    -and $deploy -match `
        '\$deploymentRootFull\s*=\s*\[System\.IO\.Path\]::GetFullPath' `
    -and $deploy -match `
        '\$deployBackupRootFull\s*=\s*\[System\.IO\.Path\]::GetFullPath' `
    -and $deploy -match `
        'Deployment backup root escaped dist/work/deployment' `
    -and $deploy -match `
        '\$deploymentRoots\s*=\s*@\(' `
    -and $deploy -match `
        'Assert-PlainDirectoryIfPresent\s+-Path\s+\$root\.Path' `
    -and $deploy -match `
        '\$backup\s*=\s*Join-Path\s+\$deployBackupRootFull') `
    'Deployment backup and install-stage writes must remain inside plain non-reparse dist/work/deployment directories.'
Assert-True ($deploy -match `
        '\$modsRootFull\s*=\s*\[System\.IO\.Path\]::GetFullPath\(\$ModsRoot\)' `
    -and $deploy -match `
        '\[System\.IO\.Path\]::GetDirectoryName\(\$validatedTarget\.Path\)' `
    -and $deploy -match `
        '\[System\.IO\.Path\]::GetFileName\(\$validatedTarget\.Path\)' `
    -and $deploy -match `
        '\$target\s*=\s*\$targetFull' `
    -and $deploy -match `
        '\$stableTarget\s*=\s*\$stableTargetFull' `
    -and $deploy -match `
        'Installed mod target is not a directory' `
    -and $deploy -match `
        'Refusing to mutate a reparse-point mod target' `
    -and $deploy -match `
        'Deployment target escaped the exact Mods root') `
    'Installed mutations are not constrained to canonical exact direct-child mod targets.'
Assert-True ($deploy -match `
        'Copy-Item\s+-LiteralPath\s+\$modsFile\s+-Destination\s+\(Join-Path\s+\$backup\s+''mods\.txt''\)' `
    -and $deploy -match `
        'Get-Date\s+-Format\s+''yyyyMMdd-HHmmss-fff''' `
    -and $deploy -match `
        'Refusing to reuse an existing deployment backup' `
    -and $deploy -match `
        '\$backupInstalledTarget\s*=\s*Join-Path\s+\$backup\s+''installed-native''' `
    -and $deploy -match `
        'Get-ChildItem\s+-LiteralPath\s+\$target\s+-Force\s*\|\s*Copy-Item[\s\S]*?-Destination\s+\$backupInstalledTarget\s+-Recurse\s+-Force' `
    -and $deploy -match `
        '\$backupStableMarkers\s*=\s*Join-Path\s+\$backup\s+''stable-markers''' `
    -and $deploy -match `
        'foreach\s*\(\$markerName\s+in\s+@\(''enabled\.txt'',\s*''enabled\.disabled\.txt''\)\)[\s\S]*?Copy-Item\s+-LiteralPath\s+\$stableMarker[\s\S]*?-Destination\s+\(Join-Path\s+\$backupStableMarkers\s+\$markerName\)') `
    'Deployment must back up mods.txt, the full prior native target, and both stable marker states before installation.'
$installTransactionIndex = [regex]::Match(
    $deploy,
    '(?ms)^\$mutationStarted\s*=\s*\$false\s*\r?\ntry\s*\{[\s\S]*?# Replace the exact validated native target').Index
$modsBackupIndex = $deploy.IndexOf(
    'Copy-Item -LiteralPath $modsFile -Destination (Join-Path $backup ''mods.txt'')',
    [StringComparison]::Ordinal)
$targetBackupIndex = $deploy.IndexOf(
    'Get-ChildItem -LiteralPath $target -Force | Copy-Item',
    [StringComparison]::Ordinal)
$stableBackupIndex = $deploy.IndexOf(
    'Copy-Item -LiteralPath $stableMarker',
    [StringComparison]::Ordinal)
Assert-True ($installTransactionIndex -gt 0 `
    -and $modsBackupIndex -ge 0 `
    -and $modsBackupIndex -lt $installTransactionIndex `
    -and $targetBackupIndex -ge 0 `
    -and $targetBackupIndex -lt $installTransactionIndex `
    -and $stableBackupIndex -ge 0 `
    -and $stableBackupIndex -lt $installTransactionIndex) `
    'Every recoverable installed input must be backed up before the installation transaction begins.'
$recursiveTargetRemovals = [regex]::Matches(
    $deploy,
    'Remove-Item\s+-LiteralPath\s+\$([A-Za-z][A-Za-z0-9_]*)\s+-Recurse\s+-Force')
Assert-True ($recursiveTargetRemovals.Count -eq 2 `
    -and @($recursiveTargetRemovals | Where-Object {
            $_.Groups[1].Value -ne 'target'
        }).Count -eq 0 `
    -and $deploy -match `
        'Copy-Item\s+-LiteralPath\s+\(Join-Path\s+\$backup\s+''mods\.txt''\)[\s\S]*?-Destination\s+\$modsFile\s+-Force' `
    -and $deploy -match `
        'Get-ChildItem\s+-LiteralPath\s+\$backupInstalledTarget\s+-Force\s*\|\s*Copy-Item[\s\S]*?-Destination\s+\$target\s+-Recurse\s+-Force' `
    -and $deploy -match `
        '\$backupMarker\s*=\s*Join-Path\s+\$backupStableMarkers\s+\$markerName[\s\S]*?Copy-Item\s+-LiteralPath\s+\$backupMarker[\s\S]*?-Destination\s+\$stableMarker\s+-Force') `
    'Exact-mirror installation and failure rollback may recursively remove only the validated native target, then must restore every backed-up input.'
Assert-True ($deploy -match `
        '\$expectedVersion\s*=\s*\[string\]\$release\.version' `
    -and $deploy -match `
        'Native source and release metadata version do not match' `
    -and $deploy -match `
        '\$dllWrite\s*-lt\s*\$latestSourceWrite' `
    -and $deploy -match `
        '\[System\.IO\.File\]::ReadAllBytes\(\$sourceDll\)' `
    -and $deploy -match `
        '\$dllAscii\.Contains\(\$expectedVersion\)' `
    -and $deploy -match `
        '\$dllUnicode\.Contains\(\$expectedVersion\)') `
    'Deployment lost source/metadata agreement, source freshness, or embedded DLL version validation.'

$visibilityInstallBlock = [regex]::Match(
    $deploy,
    '(?ms)\$installedVisibilityConfig\s*=\s*Join-Path\s+\$target\s+''config\\visibility\.ini''[\s\S]*?^\}\s*else\s*\{[\s\S]*?^\}').Value
Assert-True ($visibilityConfig -match '(?m)^\[radar\]$' `
    -and $visibilityConfig -match '(?m)^clock=true$' `
    -and $visibilityConfig -match '(?m)^bird_eggs=true$' `
    -and $visibilityConfig -match '(?m)^\[map\]$' `
    -and $visibilityConfig -match '(?m)^\[modes\]$' `
    -and $visibilityConfig -match '(?m)^area_quests=available$' `
    -and $visibilityConfig -match '(?m)^assault=available$' `
    -and $visibilityParser -match 'kMaximumVisibilityConfigBytes\s*=\s*4096U' `
    -and $visibilityParser -match `
        'VisibilityConfigFormat::LegacySchema1[\s\S]*?VisibilityConfigFormat::LegacySchema4' `
    -and $visibilityParser -match 'parse_visibility_config\(' `
    -and $visibilityParser -match 'format_visibility_config\(' `
    -and $visibilityHubHeader -match `
        'AreaQuests,[\s\S]*?BirdEggs,[\s\S]*?Count' `
    -and $visibilityHubHeader -match `
        'enum class AssaultDisplayMode[\s\S]*?Current,[\s\S]*?All' `
    -and $visibilityHubHeader -match `
        'RadarVisibilityHubResult[\s\S]*?AssaultDisplayMode\s+assault_mode\{AssaultDisplayMode::Current\}' `
    -and $visibilityHubHeader -match `
        'kRadarVisibilityAllCategories\s*=\s*0x7FU' `
    -and $visibilityHubHeader -match `
        'kRadarVisibilityWorldCategories\s*=\s*0x3EU' `
    -and $visibilityHubCode -match `
        'std::array<RowDefinition,\s*7>[\s\S]*?L"BIRD EGGS"[\s\S]*?RadarVisibilityCategory::BirdEggs' `
    -and $visibilityHubCode -match `
        'column\s*==\s*1[\s\S]*?definition\.category\s*==\s*RadarVisibilityCategory::Clock[\s\S]*?definition\.category[\s\S]*?==\s*RadarVisibilityCategory::BirdEggs[\s\S]*?add_text\([\s\S]*?L"-"' `
    -and $visibilityHubCode -match `
        'column\s*==\s*1[\s\S]*?category\s*==\s*static_cast<std::size_t>\([\s\S]*?RadarVisibilityCategory::Clock\)[\s\S]*?category\s*==\s*static_cast<std::size_t>\([\s\S]*?RadarVisibilityCategory::BirdEggs\)[\s\S]*?continue' `
    -and $deploy -match `
        '\$sourceVisibilityConfig\s*=\s*Join-Path\s+\$projectRoot\s+''config\\visibility\.ini''' `
    -and $deploy -match `
        '\$sourceDll,[\s\S]*?\$sourceBuildReceipt,\s*\$sourceVisibilityConfig' `
    -and $deploy -notmatch '\$sourceConfig' `
    -and $deploy -match `
        '\$preservedVisibility\s*=\s*Join-Path\s+\$backup\s+''preserved\\visibility\.ini''' `
    -and $deploy -match `
        'Copy-Item\s+-LiteralPath\s+\$currentVisibility[\s\S]*?-Destination\s+\$preservedVisibility\s+-Force' `
    -and $visibilityInstallBlock -match `
        'if\s*\(Test-Path\s+-LiteralPath\s+\$preservedVisibility[\s\S]*?Copy-Item\s+-LiteralPath\s+\$preservedVisibility[\s\S]*?else[\s\S]*?Copy-Item\s+-LiteralPath\s+\$sourceVisibilityConfig' `
    -and [regex]::Matches(
        $deploy,
        'Copy-Item\s+-LiteralPath\s+\$sourceVisibilityConfig').Count -eq 1) `
    'Exact-mirror deployment, compact-only bird eggs, and the readable AVAILABLE public defaults no longer agree.'
Assert-True ($deploy -match `
        '\[ValidateSet\(''Preserve'',\s*''Enable'',\s*''Disable''\)\]' `
    -and $deploy -match `
        '\$preservedDiagnostics\s*=\s*Join-Path\s+\$backup\s+''preserved\\diagnostics\.ini''' `
    -and $deploy -match 'debug_logging=true' `
    -and $deploy -match 'debug_logging=false' `
    -and $deploy -match 'AllowUserDiagnostics') `
    'Deployment must preserve diagnostics by default while supporting one explicit test-install override.'
Assert-True ($deploy -match `
        '\. \(Join-Path \$PSScriptRoot ''ReleaseLayout\.ps1''\)' `
    -and $deploy -match `
        'New-DsnwrRuntimePayload[\s\S]*?-DestinationRoot \$installStage' `
    -and [regex]::Matches(
        $deploy, 'Test-DsnwrRuntimePayload').Count -eq 2 `
    -and $deploy -match `
        'Get-ChildItem\s+-LiteralPath\s+\$installStage\s+-Force\s*\|\s*Copy-Item[\s\S]*?-Destination\s+\$target\s+-Recurse\s+-Force') `
    'Deployment must build, verify, install, and re-verify one exact allowlisted runtime payload.'

$runtimeVisibilityService = Get-NativeOwnerFunction `
    $mainCode 'service_runtime_visibility_edges'
$retireAtlasForRuntimeDelta = Get-NativeOwnerFunction `
    $mainCode 'refresh_world_map_atlas_for_runtime_delta'
$captureWorldMapCandidate = Get-NativeOwnerFunction `
    $mainCode 'capture_world_map_candidate_unsafe'
$consumeEncounterCandidates = Get-NativeOwnerFunction `
    $mainCode 'consume_created_encounter_candidates'
$applySaveResult = Get-NativeOwnerFunction `
    $mainCode 'apply_full_save_reconcile_result'
$engineTickUnsafe = Get-NativeOwnerFunction $mainCode 'engine_tick_unsafe'
$engineTick = Get-NativeOwnerFunction $mainCode 'engine_tick'
$serviceEngineTickFault = Get-NativeOwnerFunction `
    $mainCode 'service_engine_tick_fault'
$serviceVisibilityHub = Get-NativeOwnerFunction `
    $mainCode 'service_visibility_hub'
$applyVisibilityHubResult = Get-NativeOwnerFunction `
    $mainCode 'apply_visibility_hub_result'
$collectWorldMapSnapshot = Get-NativeOwnerFunction `
    $mainCode 'collect_world_map_marker_snapshot'
$rebuildCompactSnapshot = Get-NativeOwnerFunction `
    $mainCode 'rebuild_compact_snapshot'
$observeEncounter = Get-NativeOwnerFunction $mainCode 'observe_encounter'
$probeObservedObjects = Get-NativeOwnerFunction `
    $mainCode 'probe_observed_objects'
$recoverEncounterEnd = Get-NativeOwnerFunction `
    $mainCode 'recover_unobserved_encounter_end'
$encounterDeathPreUnsafe = Get-NativeOwnerFunction `
    $mainCode 'encounter_death_pre_unsafe'
$evaluateAreaQuestCondition = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}\[\[nodiscard\]\]\s+dswros::AreaQuestEligibilityProof\s+evaluate_area_quest_condition\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
$activateOwner = Get-NativeOwnerFunction $mainCode 'activate'
$disableOwner = Get-NativeOwnerFunction $mainCode 'disable'
$disableForMainMenuOwner = Get-NativeOwnerFunction `
    $mainCode 'disable_for_main_menu_owner_boundary'
$transitionOwner = Get-NativeOwnerFunction $mainCode 'transition_begin'
$shutdownOwner = Get-NativeOwnerFunction `
    $mainCode 'shutdown_for_process_lifetime'
$hubInitialize = Get-VisibilityHubMethod `
    $visibilityHubCode 'initialize'
$hubOpenService = Get-VisibilityHubMethod `
    $visibilityHubCode 'service_open_panel'
$hubToggle = Get-VisibilityHubMethod `
    $visibilityHubCode 'toggle'
$hubOpenUnsafe = Get-VisibilityHubMethod `
    $visibilityHubCode 'open_unsafe'
$hubServiceUnsafe = Get-VisibilityHubMethod `
    $visibilityHubCode 'service_unsafe'
$hubDetachUnsafe = Get-VisibilityHubMethod `
    $visibilityHubCode 'detach_unsafe'
$setHubText = [regex]::Match(
    $visibilityHubCode,
    '(?ms)^void\s+set_text\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
$closedHubGuardIndex = $serviceVisibilityHub.IndexOf(
    'if (!visibility_hub_.is_open()', [StringComparison]::Ordinal)
$closedHubControllerIndex = $serviceVisibilityHub.IndexOf(
    'current_player_controller(engine)', [StringComparison]::Ordinal)
$hubOpenGuardIndex = $hubOpenService.IndexOf(
    'if (state_ != RadarVisibilityHubState::Open)',
    [StringComparison]::Ordinal)
$hubOpenControllerIndex = $hubOpenService.IndexOf(
    'if (!current_controller)', [StringComparison]::Ordinal)
$hubGuardedServiceIndex = $hubOpenService.IndexOf(
    'service_guarded(current_controller)', [StringComparison]::Ordinal)
Assert-True ([regex]::Matches(
        $mainCode,
        'register_keydown_event\(Input::Key::F6').Count -eq 1 `
    -and $mainCode -match `
        'kVisibilityHubServiceInterval\s*=\s*std::chrono::milliseconds\{50\}' `
    -and $mainCode -match `
        'kVisibilityHubToggleDebounce\s*=\s*std::chrono::milliseconds\{250\}' `
    -and $engineTickUnsafe -match `
        'f6_requests_\.exchange\(0,\s*std::memory_order_acq_rel\)' `
    -and $engineTickUnsafe -match `
        '!enabled_\s*\|\|\s*transition_active_[\s\S]*?reason=radar_not_active' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'service_visibility_hub\(engine,\s*now\)').Count -eq 1 `
    -and $mainCode -match 'std::atomic<std::uint32_t>\s+f6_requests_') `
    'F6 visibility-Hub ownership must be one debounced engine-thread transaction.'
Assert-True ($hubToggle -match `
        'state_\s*==\s*RadarVisibilityHubState::Faulted[\s\S]*?faults_before_detach\s*=\s*fault_count_[\s\S]*?detach_guarded\(current_controller\)[\s\S]*?fault_count_\s*==\s*faults_before_detach[\s\S]*?abi_failure_mask_\s*==\s*0[\s\S]*?state_\s*=\s*RadarVisibilityHubState::Ready') `
    'A prior runtime-only Hub fault must be recoverable only by a later explicit F6 after clean guarded handle reset and valid ABI.'
Assert-True ($closedHubGuardIndex -ge 0 `
    -and $closedHubControllerIndex -gt $closedHubGuardIndex `
    -and $serviceVisibilityHub -match `
        '\{\s*if\s*\(!visibility_hub_\.is_open\(\)' `
    -and $serviceVisibilityHub -match `
        'now\s*<\s*visibility_hub_service_after_[\s\S]*?return;' `
    -and $serviceVisibilityHub -match `
        'visibility_hub_service_after_\s*=\s*now\s*\+\s*kVisibilityHubServiceInterval' `
    -and $hubOpenGuardIndex -ge 0 `
    -and $hubOpenService -match `
        '\{\s*if\s*\(state_\s*!=\s*RadarVisibilityHubState::Open\)' `
    -and $hubOpenControllerIndex -gt $hubOpenGuardIndex `
    -and $hubGuardedServiceIndex -gt $hubOpenGuardIndex `
    -and $hubOpenService -notmatch `
        'ProcessEvent|FindAllOf|FindFirstOf|StaticFindObject|filesystem|fstream|std::vector|\bnew\b' `
    -and $mainCode -match `
        'visibility_hub=f6_transient_native_umg_auto_apply_change_only_x_close_cursor_reassert_open_only') `
    'A closed Visibility Hub must return before controller lookup or any UObject/reflection/file work; 50 ms service is open-only.'
Assert-True ($applyVisibilityHubResult -match `
        'result\.action\s*!=\s*dsnwr::RadarVisibilityHubAction::Applied[\s\S]*?\|\|\s*!result\.changed[\s\S]*?return;' `
    -and $applyVisibilityHubResult -match `
        'visibility_masks_\s*=\s*result\.packed_masks' `
    -and $applyVisibilityHubResult -match `
        'area_quest_display_mode_\s*=\s*result\.area_quest_mode' `
    -and $applyVisibilityHubResult -match `
        'assault_display_mode_\s*=\s*result\.assault_mode' `
    -and [regex]::Matches(
        $mainCode,
        'persist_visibility_settings\(').Count -eq 2 `
    -and [regex]::Matches(
        $mainCode,
        'load_visibility_settings\(').Count -eq 2 `
    -and $mainCode -match `
        'area_quest_mode=available\|all|AreaQuestDisplayMode::AllUnfinished' `
    -and $mainCode -match 'format_visibility_config\(' `
    -and $mainCode -match 'parse_visibility_config\(' `
    -and $visibilityParser -match 'VisibilityConfigFormat::LegacySchema4' `
    -and $mainCode -match `
        'assault_mode=[\s\S]*?AssaultDisplayMode::All[\s\S]*?"current"') `
    'Visibility settings must load once at owner construction, migrate legacy schema 1-4, and persist the readable format only after a real Hub selection change.'
Assert-True ($applyVisibilityHubResult -match `
        'if\s*\(world_changed\)[\s\S]*?visibility_hub_\.is_open\(\)[\s\S]*?visibility_hub_world_map_baseline_mask_[\s\S]*?visibility_hub_world_map_baseline_area_quest_mode_[\s\S]*?visibility_hub_world_map_baseline_assault_mode_' `
    -and $mainCode -match `
        'VISIBILITY_HUB_WORLD_MAP_REFRESH[\s\S]*?action=single_coalesced_rebuild' `
    -and $mainCode -match `
        'RadarVisibilityHubAction::Opened[\s\S]*?visibility_hub_world_map_baseline_valid_\s*=\s*true[\s\S]*?visibility_hub_world_map_baseline_mask_[\s\S]*?visibility_hub_world_map_baseline_area_quest_mode_[\s\S]*?visibility_hub_world_map_baseline_assault_mode_' `
    -and [regex]::Matches(
        $mainCode,
        'flush_visibility_hub_world_map_refresh\("(?:x_close|f6_close)"\)').Count -eq 2) `
    'Hub changes must persist immediately while only the final net expanded-map change can cause one close-edge rebuild.'
Assert-True ($visibilityHubHeader -match `
        'RadarVisibilityHubAction[\s\S]*?Applied,[\s\S]*?Closed,[\s\S]*?Rejected' `
    -and $visibilityHubHeader -match 'FWeakObjectPtr\s+close_control_' `
    -and $visibilityHubHeader -notmatch `
        'apply_control_|cancel_control_|RadarVisibilityHubAction::Cancelled' `
    -and $visibilityHubCode -notmatch `
        'L"APPLY"|L"CANCEL"|apply_control_|cancel_control_' `
    -and $hubToggle -match `
        'state_\s*==\s*RadarVisibilityHubState::Open[\s\S]*?RadarVisibilityHubAction::Closed' `
    -and $hubServiceUnsafe -match `
        'close_control_\.Get\(\)[\s\S]*?is_checked\(close_control,[\s\S]*?detach_unsafe\(current_controller\)[\s\S]*?RadarVisibilityHubAction::Closed' `
    -and $hubServiceUnsafe -match `
        'pending_masks_\s*==\s*source_masks_[\s\S]*?pending_area_quest_mode_\s*==\s*source_area_quest_mode_[\s\S]*?pending_assault_mode_\s*==\s*source_assault_mode_[\s\S]*?RadarVisibilityHubAction::None' `
    -and $hubServiceUnsafe -match `
        'const RadarVisibilityMaskWord applied\s*=\s*pending_masks_[\s\S]*?source_masks_\s*=\s*applied[\s\S]*?source_area_quest_mode_\s*=\s*applied_mode[\s\S]*?source_assault_mode_\s*=\s*applied_assault_mode[\s\S]*?RadarVisibilityHubAction::Applied' `
    -and $hubServiceUnsafe -notmatch `
        'source_masks_\s*=\s*applied[\s\S]*?detach_unsafe') `
    'Hub selections must auto-apply exactly on mask, area-mode, or Assault-mode changes while X or a second F6 closes without Apply/Cancel controls.'
$hubOpenInputIndex = $hubOpenUnsafe.IndexOf(
    'set_input_mode_game_and_ui_, &input', [StringComparison]::Ordinal)
$hubOpenCursorIndex = $hubOpenUnsafe.IndexOf(
    'write_cursor_visible(current_controller, true)',
    [StringComparison]::Ordinal)
Assert-True ($hubOpenInputIndex -ge 0 `
    -and $hubOpenCursorIndex -gt $hubOpenInputIndex `
    -and $hubServiceUnsafe -match `
        'if\s*\(owns_input_mode_\)[\s\S]*?read_cursor_visible\(current_controller,\s*cursor_visible\)[\s\S]*?if\s*\(!cursor_visible\)[\s\S]*?set_input_mode_game_and_ui_[\s\S]*?write_cursor_visible\(current_controller,\s*true\)' `
    -and $hubServiceUnsafe -notmatch `
        'if\s*\(cursor_visible\)[\s\S]*?set_input_mode_game_and_ui_') `
    'Hub opening must set GameAndUI before the cursor write and reassert input only after an open-panel cursor overwrite is observed.'
Assert-True ($visibilityHubCode -match `
        'kReferencePanelWidth\s*=\s*620\.0' `
    -and $visibilityHubCode -match `
        'kReferencePanelHeight\s*=\s*526\.0' `
    -and $hubOpenUnsafe -match `
        'std::min\(\s*viewport_size\.return_value\.x\s*/\s*2560\.0,\s*viewport_size\.return_value\.y\s*/\s*1440\.0\)' `
    -and $hubOpenUnsafe -match `
        'const double unit_scale\s*=\s*display_scale\s*/\s*static_cast<double>\(viewport_scale\.return_value\)' `
    -and $hubOpenUnsafe -match `
        '\(viewport_size\.return_value\.x\s*-\s*physical_width\)\s*\*\s*0\.5' `
    -and $hubOpenUnsafe -match `
        '\(viewport_size\.return_value\.y\s*-\s*physical_height\)\s*\*\s*0\.5') `
    'Hub layout must preserve aspect-ratio-aware viewport scaling, DPI conversion, and centering.'
Assert-True ($visibilityHubCode -match `
        '/Script/UMG\.Widget:SetRenderScale' `
    -and $visibilityHubCode -match `
        '/Script/UMG\.Widget:SetRenderTransformPivot' `
    -and $hubInitialize -match `
        'require_parameters\(1U\s*<<\s*25U,\s*set_render_scale_,\s*16\)' `
    -and $hubInitialize -match `
        'require_parameters\(1U\s*<<\s*26U,\s*set_render_pivot_,\s*16\)' `
    -and $hubOpenUnsafe -match `
        'VectorParameters\s+pivot\{\{0\.0,\s*0\.0\}\}[\s\S]*?text_block->ProcessEvent\(set_render_pivot_,\s*&pivot\)' `
    -and $hubOpenUnsafe -match `
        'const double scaled_text\s*=\s*unit_scale\s*\*\s*text_scale[\s\S]*?VectorParameters\s+scale\{\{scaled_text,\s*scaled_text\}\}[\s\S]*?text_block->ProcessEvent\(set_render_scale_,\s*&scale\)') `
    'Every Hub TextBlock must use the same open-time viewport/DPI scale as its panel geometry with a top-left pivot.'
Assert-True ($visibilityHubCode -match 'kPanelFrame' `
    -and $visibilityHubCode -match 'kContentBackground' `
    -and $visibilityHubCode -match 'kCompactHeader' `
    -and $visibilityHubCode -match 'kWorldHeader' `
    -and $visibilityHubCode -match `
        'L"RADAR"[\s\S]*?0\.66' `
    -and $visibilityHubCode -match `
        'L"MAP"[\s\S]*?0\.66' `
    -and $visibilityHubCode -notmatch `
        'L"MINIMAP"|L"WORLD MAP"' `
    -and $hubOpenUnsafe -match `
        'const double scaled_text\s*=\s*unit_scale\s*\*\s*text_scale' `
    -and $visibilityHubCode -match `
        'row\s*%\s*2U\s*==\s*0U\s*\?\s*kBaseRow\s*:\s*kAlternateRow') `
    'Hub visual hierarchy must retain its framed panel, column chips, and alternating row surfaces.'
Assert-True ($visibilityHubHeader -match `
        'enum class AreaQuestDisplayMode[\s\S]*?Available,[\s\S]*?AllUnfinished' `
    -and $visibilityHubHeader -match `
        'area_mode_available_control_[\s\S]*?area_mode_all_control_' `
    -and $visibilityHubCode -match `
        'L"AREA QUEST MODE"[\s\S]*?L"AVAILABLE"[\s\S]*?L"ALL"' `
    -and $hubServiceUnsafe -match `
        'available_checked[\s\S]*?all_checked[\s\S]*?pending_area_quest_mode_[\s\S]*?set_checked' `
    -and $applyVisibilityHubResult -match `
        'area_quest_mode_changed[\s\S]*?compact_changed[\s\S]*?world_changed' `
    -and $mainCode -match `
        'area_quest_visible_for_selected_mode\(index\)') `
    'F6 must expose one mutually exclusive AVAILABLE/ALL area-quest presentation mode and invalidate both existing render snapshots only on a real mode edge.'
Assert-True ($visibilityHubHeader -match `
        'enum class AssaultDisplayMode[\s\S]*?Current,[\s\S]*?All' `
    -and $visibilityHubHeader -match `
        'assault_mode_current_control_[\s\S]*?assault_mode_all_control_[\s\S]*?assault_mode_current_visual_[\s\S]*?assault_mode_all_visual_' `
    -and $visibilityHubCode -match `
        'L"ASSAULT MODE"[\s\S]*?L"AVAILABLE"[\s\S]*?L"ALL"' `
    -and $hubOpenUnsafe -match `
        'source_assault_mode_\s*=\s*current_assault_mode[\s\S]*?pending_assault_mode_\s*=\s*current_assault_mode' `
    -and $hubServiceUnsafe -match `
        'assault_current_checked[\s\S]*?assault_all_checked[\s\S]*?pending_assault_mode_[\s\S]*?selected_assault_current[\s\S]*?set_checked' `
    -and $applyVisibilityHubResult -match `
        'assault_mode_changed[\s\S]*?compact_changed[\s\S]*?world_changed' `
    -and $applyVisibilityHubResult -match `
        'if\s*\(assault_mode_changed\)[\s\S]*?establish_encounter_visibility_baseline\(unix_seconds\(\)\)' `
    -and ($hubOpenUnsafe + $hubServiceUnsafe) -notmatch `
        'std::chrono|Clock::|sqlite|sqlcipher|execute_optional_sql|request_area_quest_scan|service_runtime_visibility_edges|FindAllOf|FindFirstOf') `
    'F6 Assault AVAILABLE/ALL must be mutually exclusive, invalidate both existing snapshots on a real edge, re-baseline the existing mask, and introduce no timer, scan, or SQL work.'
Assert-True ($setHubText.Length -gt 0 `
    -and $hubInitialize -match `
        'CastField<FTextProperty>\([\s\S]*?find_function_property\(set_text_,\s*L"InText"\)' `
    -and $hubInitialize -match `
        'set_text_value_property_->GetOffset_Internal\(\)\s*!=\s*0' `
    -and $hubInitialize -match `
        'set_text_value_property_->GetSize\(\)[\s\S]*?sizeof\(FText\)' `
    -and $setHubText -match `
        'ProcessEvent\(function,\s*&parameters\)[\s\S]*?text_property->DestroyValue_InContainer\(&parameters\)' `
    -and [regex]::Matches(
        $setHubText,
        'DestroyValue_InContainer\(&parameters\)').Count -eq 1) `
    'Each reflected Hub FText temporary must be ABI validated and explicitly destroyed exactly once after SetText.'

Assert-True ($engineTick -match `
        '__except\s*\(EXCEPTION_EXECUTE_HANDLER\)[\s\S]*?engine_tick_fault_code_\s*=\s*GetExceptionCode\(\)[\s\S]*?engine_tick_fault_pending_\s*=\s*true[\s\S]*?enabled_\s*=\s*false[\s\S]*?transition_active_\s*=\s*true' `
    -and $engineTick -match `
        'engine_tick_fault_cleanup_in_progress_[\s\S]*?engine_tick_fault_cleanup_failed_\s*=\s*true' `
    -and $engineTick -notmatch `
        'detach\s*\(|activate\s*\(|disable\s*\(|append_log\s*\(' `
    -and $engineTickUnsafe -match `
        'if\s*\(engine_tick_fault_pending_\)[\s\S]*?service_engine_tick_fault\(engine\)[\s\S]*?return;' `
    -and $serviceEngineTickFault -match `
        'engine_tick_fault_recovery_attempts_\s*==\s*0' `
    -and $serviceEngineTickFault -match `
        '\+\+engine_tick_fault_recovery_attempts_[\s\S]*?disable\(\)[\s\S]*?world_map_umg_renderer_\.detach\(\)[\s\S]*?if\s*\(!recover\)[\s\S]*?return;[\s\S]*?activate\(engine\)' `
    -and $serviceEngineTickFault -match `
        'if\s*\(engine_tick_fault_cleanup_failed_\)[\s\S]*?reason=cleanup_fault[\s\S]*?return;' `
    -and $serviceEngineTickFault -match `
        'engine_tick_fault_cleanup_in_progress_\s*=\s*true[\s\S]*?disable\(\)[\s\S]*?engine_tick_fault_cleanup_in_progress_\s*=\s*false' `
    -and $serviceEngineTickFault -notmatch `
        'while\s*\(|sleep_for|FindAllOf|FindFirstOf|StaticFindObject') `
    'A top-level engine-tick fault must defer guarded cleanup to the next game-thread tick, permit only one automatic activation, and stop without retry loops after another failure.'
Assert-True ($engineTickUnsafe -match `
        'retryable_attach_failure\s*=[\s\S]*?WorldMapUmgRendererState::Faulted[\s\S]*?area_quest_scan_faulted_' ) `
    'Explicit F7 must run a full bounded activation for world-map or area-task scan faults.'

$appendEventLog = [regex]::Match(
    $nativeEventLogCode,
    '(?ms)^void\s+append_native_event_log\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
$appendEventLine = [regex]::Match(
    $nativeEventLogCode,
    '(?ms)^bool\s+append_line_locked\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
$tryEnqueue = [regex]::Match(
    $nativeEventLogCode,
    '(?ms)^void\s+try_enqueue\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
$writerLoop = [regex]::Match(
    $nativeEventLogCode,
    '(?ms)^void\s+writer_loop\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
$immediateFlushPolicy = [regex]::Match(
    $nativeEventLogCode,
    '(?ms)^bool\s+requires_immediate_flush\s*\([^;]*?\)[^{]*\{(?:(?!^\}).)*^\}').Value
Assert-True ($appendEventLog.Length -gt 0 `
    -and $appendEventLine.Length -gt 0 `
    -and $tryEnqueue.Length -gt 0 `
    -and $writerLoop.Length -gt 0 `
    -and $immediateFlushPolicy.Length -gt 0 `
    -and $nativeEventLogCode -match `
        'kMaximumLogBytes\s*=\s*1024U\s*\*\s*1024U' `
    -and $nativeEventLogCode -match `
        'kBufferedLineLimit\s*=\s*16U' `
    -and $nativeEventLogCode -match `
        'kQueueCapacity\s*=\s*256U' `
    -and $nativeEventLogCode -match `
        'kWriterIdleFlushInterval\s*=\s*std::chrono::milliseconds\{250\}' `
    -and $nativeEventLogCode -match `
        'DragonSwordNativeWorldRadarPostRender\.Native\.previous\.log' `
    -and $nativeEventLogCode -match 'MoveFileExW' `
    -and $nativeEventLogCode -match `
        'if\s*\(!rotate_current_output\(state\)\)[\s\S]*?open_truncated_output\(state\)' `
    -and $appendEventLog -notmatch `
        'create_directories|mod_directory|std::ofstream\s+output|append_line_locked|\.flush\(' `
    -and $mainCode -match `
        'load_native_event_log_enabled\([\s\S]*?diagnostics\.ini[\s\S]*?begin_native_event_log_session\(\s*directory,\s*diagnostics_enabled\)' `
    -and $mainCode -match 'flush_native_event_log\(\)' `
    -and $cmake -match 'src/native/native_event_log\.cpp' `
    -and $appendEventLine -match `
        'if\s*\(!state\.output\.good\(\)\)[\s\S]*?return\s+false' `
    -and $appendEventLog -match 'try_enqueue\(RecordKind::Text' `
    -and $tryEnqueue -match 'std::try_to_lock' `
    -and $tryEnqueue -match `
        'queue_size\s*>=\s*state\.queue\.size\(\)[\s\S]*?dropped_records\.fetch_add' `
    -and $writerLoop -match 'write_queued_record' `
    -and $writerLoop -match 'kWriterIdleFlushInterval' `
    -and $nativeEventLogCode -match `
        'std::thread\{writer_loop,\s*&state\}' `
    -and $nativeEventLogCode -match `
        'writer\.join\(\)') `
    'Native diagnostics must enqueue through one bounded non-blocking queue and perform bounded batching, file I/O, rollover, drain, and flush only on the dedicated writer.'
$appendFastReturn = $appendEventLog.IndexOf(
    'if (!event_log_state().enabled.load', [StringComparison]::Ordinal)
$appendEnqueue = $appendEventLog.IndexOf(
    'try_enqueue', [StringComparison]::Ordinal)
Assert-True ($diagnosticsConfig -match '(?m)^\[diagnostics\]$' `
    -and $diagnosticsConfig -match '(?m)^debug_logging=false$') `
    'Release-source diagnostics must default to exactly disabled.'
Assert-True ($diagnosticsParser -match `
        'kMaximumDiagnosticsConfigBytes\s*=\s*2048' `
    -and $diagnosticsParser -match 'parse_event_log_enabled' `
    -and $diagnosticsParser -match 'debug_logging' `
    -and $diagnosticsParser -match 'event_log_enabled=true' `
    -and $diagnosticsParser -match 'value\s*==\s*"true"' `
    -and $diagnosticsParser -match 'value\s*==\s*"false"' `
    -and $nativeTests -match 'exact legacy diagnostics true value must remain accepted' `
    -and $nativeTests -match 'duplicate diagnostics keys must fail closed' `
    -and $nativeTests -match 'oversized diagnostics input must fail closed') `
    'Diagnostics parsing must remain bounded, strict, tested, and fail closed.'
Assert-True ($nativeEventLogCode -match 'SESSION_BEGIN' `
    -and $nativeEventLogCode -match 'format_diagnostic_log_metadata' `
    -and $diagnosticsFormatter -match 'seq=' `
    -and $diagnosticsFormatter -match 'utc_ms=' `
    -and $diagnosticsFormatter -match 'elapsed_ms=' `
    -and $nativeTests -match 'stable sequence and time fields') `
    'Enabled diagnostics must expose one bounded session header plus allocation-free sequence and timing metadata.'
Assert-True ($nativeEventLogCode -match 'std::atomic_bool\s+enabled' `
    -and $appendFastReturn -ge 0 `
    -and $appendEnqueue -gt $appendFastReturn `
    -and $appendEventLog -notmatch `
        'std::format|std::mutex|std::lock_guard|std::unique_lock|std::ofstream|\.write\(|\.flush\(' `
    -and $mainCode -match `
        '#define\s+append_log[\s\S]*?native_event_log_enabled\(\)[\s\S]*?append_native_event_log' `
    -and $mainCode -match `
        'runtime_diagnostics=startup_config_once config_debug_logging=\{\} log_schema=2') `
    'Disabled diagnostics must avoid formatting, locking, queue work, directory creation, and file I/O after one startup-only config read.'
Assert-True ($mainCode -match `
        'append_native_engine_tick_slow\([\s\S]*?append_native_engine_tick_profile\(' `
    -and $mainCode -notmatch `
        'append_log\("ENGINE_TICK_(?:SLOW|PROFILE)"' `
    -and $nativeEventLogCode -match `
        'RecordKind::EngineTickSlow[\s\S]*?RecordKind::EngineTickProfile' `
    -and $nativeEventLogCode -match `
        'logger_dropped=\{\}[\s\S]*?logger_truncated=\{\}') `
    'Per-tick performance telemetry must enqueue fixed numeric records; formatting and logger-health expansion belong only to the writer thread.'
Assert-True ($immediateFlushPolicy -notmatch `
        'COMPACT_POOL_STATE|WORLD_MAP_STATE' `
    -and $immediateFlushPolicy -match 'START' `
    -and $immediateFlushPolicy -match 'F7_ACTIVATED' `
    -and $immediateFlushPolicy -match 'F8_DISABLED' `
    -and $immediateFlushPolicy -match 'FAILED' `
    -and $immediateFlushPolicy -match 'FAULT') `
    'Renderer state diagnostics must use the bounded ordinary batch while critical lifecycle and fault events remain immediate.'
$transitionHubDetachIndex = $transitionOwner.IndexOf(
    'visibility_hub_.detach();', [StringComparison]::Ordinal)
$transitionCompactDetachIndex = $transitionOwner.IndexOf(
    'compact_umg_renderer_.detach();', [StringComparison]::Ordinal)
Assert-True ($activateOwner -match 'visibility_hub_\.detach\(\)' `
    -and $disableOwner -match 'visibility_hub_\.detach\(\)' `
    -and $shutdownOwner -match 'visibility_hub_\.detach\(\)' `
    -and $transitionHubDetachIndex -ge 0 `
    -and $transitionCompactDetachIndex -gt $transitionHubDetachIndex `
    -and $transitionOwner -notmatch 'release_for_travel') `
    'F8, reactivation, shutdown, and travel must detach the Hub; travel must restore input before old-world UMG becomes stale.'
$hubOwnerLookupIndex = $hubDetachUnsafe.IndexOf(
    'host->ProcessEvent(get_owning_player_', [StringComparison]::Ordinal)
$hubCurrentFallbackIndex = $hubDetachUnsafe.IndexOf(
    'input_controller = current_controller', [StringComparison]::Ordinal)
Assert-True ($hubDetachUnsafe -match `
        'if\s*\(host\s*&&\s*owns_input_mode_\)[\s\S]*?get_owning_player_[\s\S]*?input_controller\s*=\s*owning_player\.return_value' `
    -and $hubDetachUnsafe -match `
        'if\s*\(!input_controller\)[\s\S]*?input_controller\s*=\s*current_controller' `
    -and $hubOwnerLookupIndex -ge 0 `
    -and $hubCurrentFallbackIndex -gt $hubOwnerLookupIndex) `
    'Hub cleanup must restore input through the panel owning controller before falling back to a newer current controller.'
$encounterVisibilityMask = [regex]::Match(
    $mainCode,
    '(?ms)^\s{4}(?:\[\[nodiscard\]\]\s*)?std::uint64_t\s+encounter_visibility_mask\s*\([^;]*?\)[^{]*\{(?:(?!^\s{4}\}).)*^\s{4}\}').Value
$runtimeEdgeCallIndex = $engineTickUnsafe.IndexOf(
    'service_runtime_visibility_edges(now);',
    [StringComparison]::Ordinal)
$positionBranchIndex = $engineTickUnsafe.IndexOf(
    'if (now >= next_position_)',
    [StringComparison]::Ordinal)
Assert-True ([regex]::Matches(
        $mainCode,
        'encounter_next_available_unix_seconds_\.clear\(\)').Count -eq 1 `
    -and $disableForMainMenuOwner -match `
        'encounter_next_available_unix_seconds_\.clear\(\)' `
    -and $applySaveResult -match `
        'encounter_next_available_unix_seconds_\[field\.id\]\s*=\s*dswros::merge_encounter_cooldown\(\s*current_next_available,\s*next_available\)' `
    -and $applySaveResult -match `
        'rebuild_area_quest_world_map_eligibility\(\)[\s\S]*?refresh_world_map_atlas_for_runtime_delta\(' `
    -and $objectState -match `
        'merge_encounter_cooldown[\s\S]*?current_next_available_unix_seconds\s*<\s*candidate_next_available_unix_seconds' `
    -and $nativeTests -match `
        'an older save snapshot must not roll back a newer runtime cooldown' `
    -and $nativeTests -match `
        'a newer save snapshot must advance the effective cooldown') `
    'The deployment gate must preserve max-merged runtime encounter cooldowns outside the exact TitleMap save-owner boundary and retire an attached stale atlas after delayed save application.'
Assert-True ($encounterVisibilityMask.Length -gt 0 `
    -and $mainCode -match 'kExpectedEncounterCount\s*=\s*49' `
    -and $encounterVisibilityMask -match `
        'std::min\(\s*encounter_catalog_\.size\(\),\s*kExpectedEncounterCount\)' `
    -and $encounterVisibilityMask -match `
        'encounter_visible_for_selected_mode_at_hour\([\s\S]*?spec,\s*now_unix_seconds,\s*world_hour\)' `
    -and $encounterVisibilityMask -match `
        'mask\s*\|=\s*std::uint64_t\{1\}\s*<<\s*index' `
    -and $rebuildCompactSnapshot -match `
        'encounter_visible_for_selected_mode\([\s\S]*?spec,\s*now_unix_seconds\)' `
    -and $collectWorldMapSnapshot -match `
        'encounter_visible_for_selected_mode\([\s\S]*?spec,\s*now_unix_seconds\)' `
    -and [regex]::Matches(
        $mainCode,
        'encounter_visible_for_selected_mode\(').Count -eq 3 `
    -and [regex]::Matches(
        $mainCode,
        'encounter_visible_for_selected_mode_at_hour\(').Count -eq 2 `
    -and [regex]::Matches(
        $mainCode,
        'dswros::encounter_visible_for_display_mode\(').Count -eq 2 `
    -and $objectState -match `
        'encounter_visible_for_display_mode[\s\S]*?is_assault\s*&&\s*show_all_assaults[\s\S]*?encounter_available_now' `
    -and $nativeTests -match `
        'AVAILABLE Assault display must preserve the live time window' `
    -and $nativeTests -match `
        'ALL Assault display must expose an out-of-window static record' `
    -and $nativeTests -match `
        'ALL Assault display must include cooling-down Assaults' `
    -and $nativeTests -match `
        'ALL Assault display must expose the static catalog before save state is ready' `
    -and $nativeTests -match `
        'ALL Assault display must not bypass a Boss time condition' `
    -and $nativeTests -match `
        'AVAILABLE Assault display must fail closed before state is ready' `
    -and $runtimeVisibilityService -match `
        'next_runtime_visibility_edge_probe_\s*=\s*now\s*\+\s*kMinimapScaleSampleInterval' `
    -and $runtimeVisibilityService -match `
        '!world_hour_changed\s*&&\s*!cooldown_edge_reached[\s\S]*?encounter_visibility_mask_valid_' `
    -and $runtimeVisibilityService -match `
        '!encounter_visibility_changed\s*&&\s*!area_quest_visibility_changed[\s\S]*?return;' `
    -and $runtimeVisibilityService -match `
        'refresh_world_map_atlas_for_runtime_delta\(' `
    -and $runtimeEdgeCallIndex -ge 0 `
    -and $positionBranchIndex -gt $runtimeEdgeCallIndex `
    -and $engineTickUnsafe -match `
        'if\s*\(now\s*>=\s*next_activity_probe_\)\s*\{\s*next_activity_probe_\s*=\s*now\s*\+\s*kDiscoveryInterval;\s*probe_activity_context_guarded\(engine\);\s*if\s*\(!enabled_\)\s*\{\s*return;\s*\}\s*apply_compact_suppression\(\);\s*service_runtime_visibility_edges\(now\);\s*\}' `
    -and [regex]::Matches(
        $engineTickUnsafe,
        'service_runtime_visibility_edges\(now\)').Count -eq 1) `
    'The deployment gate does not enforce the bounded 1 Hz 49-bit presentation service or the exclusive compact/world/mask Assault display boundary.'
Assert-True ($evaluateAreaQuestCondition.Length -gt 0 `
    -and $consumeEncounterCandidates -match `
        'encounter_available\([\s\S]*?encounter_catalog_\[index\],\s*now_unix_seconds\)' `
    -and $observeEncounter -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $probeObservedObjects -match `
        'encounter_available\([\s\S]*?\*current_encounter,\s*now_unix_seconds\)' `
    -and $recoverEncounterEnd -match `
        'encounter_available\(\*spec,\s*now_unix_seconds\)' `
    -and $encounterDeathPreUnsafe -match `
        'encounter_available\(\*spec,\s*unix_seconds\(\)\)' `
    -and $evaluateAreaQuestCondition -match `
        'DynamicQuestConditionType::MonsterAlive[\s\S]*?encounter_available\(\*encounter,\s*unix_seconds\(\)\)' `
    -and ($consumeEncounterCandidates + $observeEncounter `
        + $probeObservedObjects + $recoverEncounterEnd `
        + $encounterDeathPreUnsafe + $evaluateAreaQuestCondition) -notmatch `
        'encounter_visible_for_(?:selected_mode|display_mode)') `
    'The deployable runtime must keep strict current encounter availability in binding, observation, completion/recovery, and MONSTER_ALIVE paths regardless of Assault display mode.'
Assert-True ($retireAtlasForRuntimeDelta -match `
        'world_map_marker_snapshot_built_\s*=\s*false' `
    -and $retireAtlasForRuntimeDelta -match `
        'if\s*\(attached\s*&&\s*visibly_open\)[\s\S]*?world_map_umg_renderer_\.begin_activation\(\);[\s\S]*?reset_world_map_runtime\(true\)' `
    -and $retireAtlasForRuntimeDelta -match `
        'else\s+if\s*\(attached\)[\s\S]*?world_map_session_pending_\s*=\s*false[\s\S]*?deferred_until_set_world_map_image' `
    -and $mainCode -match 'Clock::time_point\s+next_runtime_visibility_edge_probe_' `
    -and $mainCode -match 'std::int64_t\s+next_encounter_cooldown_edge_unix_seconds_' `
    -and $mainCode -match 'std::uint64_t\s+encounter_visibility_mask_' `
    -and $mainCode -match 'bool\s+encounter_visibility_mask_valid_') `
    'The deployment gate does not enforce exact-visible current-session rebuild and hidden-layer deferred refresh.'
Assert-True ($captureWorldMapCandidate -match `
        'set_image_rearm_allowed\s*=\s*same_layer[\s\S]*?set_world_map_image_event[\s\S]*?!world_map_set_image_rearm_consumed_[\s\S]*?world_map_serviced_serial_\s*==\s*world_map_candidate_serial_' `
    -and $captureWorldMapCandidate -match `
        'retryable_not_ready\(\)[\s\S]*?world_map_service_attempts_[\s\S]*?>=\s*kWorldMapMaxServiceAttempts' `
    -and $captureWorldMapCandidate -match `
        'reset_world_map_runtime\(true\);[\s\S]*?world_map_set_image_rearm_consumed_\s*=\s*true' `
    -and $mainCode -match `
        'world_map_readiness=set_world_map_image_one_shot_serial_matched_budget_rearm') `
    'The deployment gate does not enforce the one-shot serial-matched SetWorldMapImage readiness rearm.'
Assert-True ($mainCode -match 'kEncounterObservationRadius\s*=\s*10000\.0' `
    -and $mainCode -match `
        'kEncounterCandidateProbeBudgetPerControlTick\s*=\s*8' `
    -and $consumeEncounterCandidates -match `
        'if\s*\(activity_suppressed_\s*\|\|\s*!position_valid_\)' `
    -and $consumeEncounterCandidates -match `
        'position_queries\s*<\s*kEncounterCandidateProbeBudgetPerControlTick' `
    -and $consumeEncounterCandidates -match `
        'read_actor_position\(actor,\s*&actor_position\)[\s\S]*?distance_squared\(player_,\s*actor_position\)' `
    -and $consumeEncounterCandidates -notmatch `
        'actor_begin_unsafe|FindAllOf|FindFirstOf|StaticFindObject|std::vector|\bnew\b') `
    'The deployment gate does not enforce suppressed-skip, current-actor 100 m encounter discovery with an eight-query budget.'

$umgUpdate = [regex]::Match(
    $umgSource,
    'void UmgMiniMapCanary::update_unsafe[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
Assert-True ($umgUpdate.Length -gt 0) 'The steady-state UMG update was not found.'
foreach ($forbidden in @('FindAllOf', 'FindFirstOf', 'StaticFindObject', 'StaticConstructObject', 'ofstream', 'ifstream', 'new ')) {
    Assert-True ($umgUpdate -notmatch [regex]::Escape($forbidden)) `
        "UMG steady-state update contains forbidden work: $forbidden"
}
Assert-True ($source -match 'kExpectedGameTimestamp\s*=\s*0x691B0D98U') `
    'The locked game PE timestamp is missing.'
Assert-True ($source -match 'kExpectedUe4ssTimestamp\s*=\s*0x6A76FCA3U') `
    'The locked UE4SS PE timestamp is missing.'
Assert-True ($source -match 'InterlockedCompareExchangePointer') `
    'Hook ownership is not installed and restored atomically.'
Assert-True ($source -match 'module_contains\(game, original\)') `
    'The candidate original is not constrained to the game image.'
Assert-True ($source -match 'original\(viewport, canvas\);[\s\S]*draw_guarded\(canvas\)') `
    'The original PostRender must execute exactly before custom drawing.'
Assert-True ($main -notmatch 'delete\s+static_cast<NativeObjectState') `
    'The process-lifetime owner must not be deleted during uninstall.'

$detour = [regex]::Match(
    $source,
    'void __fastcall PostRenderCanary::detour[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$draw = [regex]::Match(
    $source,
    'void PostRenderCanary::draw_canary[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$drawLine = [regex]::Match(
    $source,
    'void PostRenderCanary::draw_line[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$markerRead = [regex]::Match(
    $source,
    'bool PostRenderCanary::read_relative_marker[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
Assert-True ($detour.Length -gt 0 -and $draw.Length -gt 0 `
    -and $drawLine.Length -gt 0 -and $markerRead.Length -gt 0) `
    'Render hot-path functions were not found.'
foreach ($forbidden in @('FindAllOf', 'FindFirstOf', 'StaticFindObject', 'ofstream', 'ifstream', 'append_log', 'new ')) {
    foreach ($hotPath in @($detour, $draw, $drawLine, $markerRead)) {
        Assert-True ($hotPath -notmatch [regex]::Escape($forbidden)) `
            "PostRender hot path contains forbidden work: $forbidden"
    }
}

$present = [regex]::Match(
    $lateSource,
    'long __stdcall LatePresentCanary::present_detour[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$lateDraw = [regex]::Match(
    $lateSource,
    'void LatePresentCanary::draw\([\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$execute = [regex]::Match(
    $lateSource,
    'void __stdcall LatePresentCanary::execute_command_lists_detour[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
$d3d12Draw = [regex]::Match(
    $lateSource,
    'LatePresentCanary::D3d12DrawResult LatePresentCanary::draw_d3d12[\s\S]*?^}',
    [Text.RegularExpressions.RegexOptions]::Multiline).Value
Assert-True ($present.Length -gt 0 -and $lateDraw.Length -gt 0 `
    -and $execute.Length -gt 0 -and $d3d12Draw.Length -gt 0) `
    'Late Present hot-path functions were not found.'
Assert-True ($lateSource -match 'context_->ClearView') `
    'The D3D11 late canary must use a pipeline-state-neutral ClearView path.'
Assert-True ($d3d12Draw -match 'ClearRenderTargetView' `
    -and $d3d12Draw -match 'ResourceBarrier') `
    'The D3D12 late canary must use bounded clears with explicit state transitions.'
Assert-True ($execute -match 'original\(queue, count, command_lists\)') `
    'The original ExecuteCommandLists call must be chained exactly once.'
Assert-True ($d3d12Draw -notmatch 'WaitForSingleObject') `
    'The steady-state D3D12 Present path must never wait for the GPU.'
foreach ($forbidden in @('FindAllOf', 'FindFirstOf', 'StaticFindObject', 'ofstream', 'ifstream', 'append_log', 'new ')) {
    foreach ($hotPath in @($present, $lateDraw, $execute, $d3d12Draw)) {
        Assert-True ($hotPath -notmatch [regex]::Escape($forbidden)) `
            "Late Present hot path contains forbidden work: $forbidden"
    }
}

Write-Host 'Native runtime safety and retired canary source gates passed.'
