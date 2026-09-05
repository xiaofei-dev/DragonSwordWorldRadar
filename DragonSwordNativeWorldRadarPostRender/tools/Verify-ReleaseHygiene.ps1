[CmdletBinding()]
param(
    [string]$NativeBuildDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($NativeBuildDirectory)) {
    $NativeBuildDirectory = Join-Path $projectRoot 'dist\work\build\native'
} elseif (-not [IO.Path]::IsPathRooted($NativeBuildDirectory)) {
    $NativeBuildDirectory = Join-Path $projectRoot $NativeBuildDirectory
}
$NativeBuildDirectory = [IO.Path]::GetFullPath($NativeBuildDirectory)

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Read-ProjectText {
    param([string]$RelativePath)
    $path = Join-Path $projectRoot $RelativePath
    Assert-True (Test-Path -LiteralPath $path -PathType Leaf) `
        "Required release source is missing: $RelativePath"
    $text = Get-Content -LiteralPath $path -Raw
    Assert-True (-not [string]::IsNullOrWhiteSpace($text)) `
        "Required release source is empty: $RelativePath"
    return $text
}

function Assert-Contains {
    param([string]$Text, [string]$Pattern, [string]$Message)
    Assert-True ($Text -match $Pattern) $Message
}

function Assert-NotContains {
    param([string]$Text, [string]$Pattern, [string]$Message)
    Assert-True ($Text -notmatch $Pattern) $Message
}

& (Join-Path $PSScriptRoot 'Verify-F6LocalizedTextOverlays.ps1') | Out-Null

$deployScript = Read-ProjectText 'tools\Deploy-NativePrototype.ps1'
Assert-NotContains $deployScript `
    '\$prohibited\s*=\s*@\([\s\S]{0,300}?["'']assets["'']' `
    'Deployment must allow the verified assets/ui/f6 runtime payload.'
Assert-Contains $deployScript `
    'Test-DsnwrRuntimePayload[\s\S]*?-InstalledConfiguration' `
    'Deployment must still validate the complete installed runtime payload.'

$release = Read-ProjectText 'metadata\release.json' | ConvertFrom-Json
$profile = Read-ProjectText 'metadata\installer-product-profile.json' |
    ConvertFrom-Json
$providers = Read-ProjectText 'metadata\data-providers.json' | ConvertFrom-Json
$version = [string]$release.version
$runtimeLabel = 'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1'

Assert-True ($version -eq '2.2.1' `
    -and [string]$release.runtime_label -eq $runtimeLabel `
    -and [string]$profile.product.public_version -eq $version `
    -and [string]$profile.product.runtime_label -eq $runtimeLabel) `
    'Release identity is inconsistent across metadata owners.'

$layouts = @($profile.ue4ss.layouts)
Assert-True ($layouts.Count -eq 1 `
    -and [string]$layouts[0].name -eq 'experimental_nested' `
    -and [string]$layouts[0].payload_variant -eq 'experimental_nested_x64' `
    -and [string]$profile.ue4ss.existing_compatibility_policy -match `
        'without_hash_allowlist' `
    -and [string]$profile.ue4ss.missing_policy -eq `
        'bootstrap_pinned_experimental_nested_transactionally' `
    -and [string]$profile.ue4ss.ambiguous_or_unknown_policy -eq `
        'confirm_complete_backup_and_convert_to_embedded_experimental_nested') `
    'The release must expose one structural ExperimentalNested installer contract.'

Assert-True (-not [bool]$profile.game.installer_validation.exact_game_hash_allowlist `
    -and [string]$profile.game.compatibility_policy -eq `
        'pe32plus-x64-runtime-unique-owner-pointer-pattern' `
    -and @($profile.game.transaction_identity_inputs).Count -eq 1 `
    -and [string]$profile.game.transaction_identity_inputs[0] -eq `
        'current_executable_sha256') `
    'The game hash must be provenance only, never a compatibility allowlist.'

$expectedUserPaths = @(
    'config/diagnostics.ini',
    'config/visibility.ini',
    'data/defaults/treasure_overrides.txt'
) | Sort-Object
$actualUserPaths = @($profile.configuration.user_owned_paths) | Sort-Object
Assert-True (($actualUserPaths -join '|') -eq ($expectedUserPaths -join '|')) `
    'The three user-owned files are not declared exactly.'
Assert-True (-not [bool]$profile.installation.successful_update_persistent_backup `
    -and [bool]$profile.installation.conversion_persistent_backup `
    -and [bool]$profile.installation.rollback_required) `
    'Update/repair and conversion backup policies are inconsistent.'

$expectedPublicFiles = @(
    "DragonSwordNativeWorldRadarPostRender-Setup-$version.exe",
    "DragonSwordNativeWorldRadarPostRender-Setup-$version.exe.sha256",
    'INSTALL.md',
    'THIRD_PARTY_NOTICES.txt'
) | Sort-Object
$expectedManualArchives = @(
    "DragonSwordNativeWorldRadarPostRender-v$version-Manual-No-UE4SS.zip",
    "DragonSwordNativeWorldRadarPostRender-v$version-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip"
) | Sort-Object
$expectedFinalFiles = @(
    "DragonSwordNativeWorldRadarPostRender-v$version-Installer.zip",
    $expectedManualArchives[0],
    $expectedManualArchives[1],
    'release-manifest.json',
    'SHA256SUMS.txt'
) | Sort-Object
$manualArchives = @($profile.release.manual_archives)
$actualManualNames = @($manualArchives | ForEach-Object { [string]$_.name }) |
    Sort-Object
Assert-True ([bool]$profile.release.installer_first `
    -and [bool]$profile.release.installer_unsigned `
    -and [bool]$profile.release.manual_package `
    -and $manualArchives.Count -eq 2 `
    -and ($actualManualNames -join '|') -eq ($expectedManualArchives -join '|') `
    -and @($manualArchives | Where-Object {
            [string]$_.layout -ne 'experimental_nested'
        }).Count -eq 0 `
    -and @($manualArchives | Where-Object {
            [bool]$_.includes_ue4ss
        }).Count -eq 1 `
    -and [int]$profile.release.expected_installer_tests.passed -eq 20 `
    -and [int]$profile.release.expected_installer_tests.failed -eq 0 `
    -and [int]$profile.release.expected_installer_tests.skipped -eq 0 `
    -and [int]$profile.release.expected_manual_install_tests.passed -eq 2 `
    -and [int]$profile.release.expected_manual_install_tests.failed -eq 0 `
    -and [int]$profile.release.expected_manual_install_tests.skipped -eq 0 `
    -and ((@($profile.release.expected_public_files) | Sort-Object) -join '|') -eq `
        ($expectedPublicFiles -join '|') `
    -and ((@($profile.release.expected_final_files) | Sort-Object) -join '|') -eq `
        ($expectedFinalFiles -join '|') `
    -and [string]$profile.release.final_output_directory -eq "dist/final-$version" `
    -and [string]$profile.acceptance.gameplay -eq 'NOT_VALIDATED' `
    -and [string]$profile.acceptance.publication -eq 'BLOCKED') `
    'The three-channel public release contract or acceptance state is inconsistent.'

Assert-True (@($release.compatibility_contract.supported_layouts).Count -eq 1 `
    -and [string]$release.compatibility_contract.supported_layouts[0] -eq `
        'experimental_nested' `
    -and [string]$release.compatibility_contract.unsupported_policy -match `
        'without a fixed DLL-hash allowlist' `
    -and [int]$release.release_layout.installer_matrix_gate.required_passed -eq 20 `
    -and [int]$release.release_layout.manual_install_matrix_gate.required_passed -eq 2 `
    -and [int]$release.release_layout.manual_install_matrix_gate.required_failed -eq 0 `
    -and [int]$release.release_layout.manual_install_matrix_gate.required_skipped -eq 0) `
    'Release metadata still exposes an obsolete hash-gated or manual-channel contract.'

Assert-True ([string]$providers.release_installer.game_compatibility_policy -match `
        'do not reject a game update by executable SHA-256' `
    -and [string]$providers.release_installer.user_state_policy -match `
        'treasure_overrides\.txt' `
    -and [string]$providers.release_installer.rollback_policy -match `
        'temporary rollback journal' `
    -and [string]$providers.update_policy -match `
        'does not block Setup') `
    'Data-provider metadata disagrees with the current update contract.'

$engine = Read-ProjectText 'installer\InstallerEngine.cs'
$visibilityConfig = Read-ProjectText 'config\visibility.ini'
$visibilityParser = Read-ProjectText 'include\dswros\visibility_config.hpp'
Assert-Contains $engine 'ProductVersion\s*=\s*"2\.2\.1"' `
    'Setup engine product version differs from the release identity.'
Assert-Contains $engine `
    'RuntimeLabel\s*=\s*"DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1"' `
    'Setup engine runtime label differs from the release identity.'
Assert-Contains $engine 'IsStructurallyValidX64Dll' `
    'Setup does not contain bounded structural UE4SS DLL validation.'
Assert-Contains $engine 'structurally complete ExperimentalNested UE4SS' `
    'Setup does not expose the structural ExperimentalNested result.'
Assert-Contains $engine 'UpdatesExistingRadar' `
    'Setup does not expose Update / Repair classification.'
Assert-Contains $engine 'TreasureOverridesPath\s*=\s*"data/defaults/treasure_overrides\.txt"' `
    'Setup does not own the treasure-ignore preservation path.'
Assert-Contains $engine 'ValidateTreasureOverrides' `
    'Setup does not strictly validate preserved treasure ignores.'
Assert-Contains $engine 'area_quest_mode' `
    'Setup does not validate the schema-3/4 area-quest visibility mode.'
Assert-Contains $engine 'assault_mode' `
    'Setup does not validate the schema-4 Assault visibility mode.'
Assert-Contains $engine 'schema < 1 \|\| schema > 4' `
    'Setup visibility validation is not bounded through schema 4.'
Assert-Contains $engine `
    'schema >= 4 && !hasAssaultMode[\s\S]*?schema < 4 && hasAssaultMode' `
    'Setup does not require Assault mode only for schema 4.'
Assert-Contains $engine 'ValidateSectionedVisibilityConfig' `
    'Setup does not validate the readable sectioned visibility format.'
Assert-Contains $engine `
    '"height_arrows"[\s\S]*?"treasure",\s*"area_quests",\s*"mole"' `
    'Setup does not validate all three height-arrow preferences.'
Assert-Contains $engine `
    '"interface"[\s\S]*?"language"' `
    'Setup does not validate the interface-language preference.'
Assert-Contains $engine `
    '"auto",\s*"en",\s*"ja",\s*"ko",\s*"zh-hans",\s*"zh-hant",[\s\S]*?"fr",\s*"de",\s*"es-es",\s*"ru",\s*"th",\s*"pt-br"' `
    'Setup does not accept exactly Auto plus the game''s 11 interface languages.'
Assert-Contains $engine `
    'legacyLayout\s*=\s*seenSections\.SetEquals\(legacySections\)[\s\S]*?currentLayout\s*=\s*seenSections\.SetEquals\(required\.Keys\)' `
    'Setup does not preserve complete old three-section settings while requiring five sections for the public default.'
Assert-Contains $engine 'strict 4 KiB size limit' `
    'Setup visibility validation is not bounded to the runtime 4 KiB limit.'
Assert-Contains $engine '"available"' `
    'Setup does not accept the default available area-quest mode.'
Assert-Contains $engine '"all"' `
    'Setup does not accept the all display modes.'
Assert-Contains $engine '"current"' `
    'Setup does not accept the default current Assault mode.'
Assert-Contains $engine `
    'embedded public visibility default must use the readable sectioned format' `
    'Setup does not require the readable format for the embedded public default.'
Assert-Contains $engine `
    'enable every display category and height indicator, use available modes, and follow the game language' `
    'Setup does not enforce the complete readable public visibility default.'
Assert-True ($visibilityConfig -match '(?m)^\[radar\]\r?$' `
    -and $visibilityConfig -match '(?m)^clock=true\r?$' `
    -and $visibilityConfig -match '(?m)^bird_eggs=true\r?$' `
    -and $visibilityConfig -match '(?m)^\[map\]\r?$' `
    -and $visibilityConfig -match '(?m)^\[modes\]\r?$' `
    -and $visibilityConfig -match '(?m)^area_quests=available\r?$' `
    -and $visibilityConfig -match '(?m)^assault=available\r?$' `
    -and $visibilityConfig -match `
        '(?ms)^\[height_arrows\]\r?\n(?:#[^\r\n]*\r?\n)*treasure=true\r?\narea_quests=true\r?\nmole=true\r?$' `
    -and $visibilityConfig -match `
        '(?ms)^\[interface\]\r?\n(?:#[^\r\n]*\r?\n)*language=auto\r?$' `
    -and [regex]::Matches(
        $visibilityConfig, '(?m)^\[[a-z_]+\]\r?$').Count -eq 5) `
    'The source public visibility default is not the complete five-section 2.2 default.'
Assert-True ($visibilityParser -match 'kMaximumVisibilityConfigBytes\s*=\s*4096U' `
    -and $visibilityParser -match 'parse_visibility_config\(' `
    -and $visibilityParser -match 'format_visibility_config\(' `
    -and $visibilityParser -match `
        'old_sectioned\s*=\s*seen_sections\s*==\s*0x07U' `
    -and $visibilityParser -match `
        'current_sectioned\s*=\s*seen_sections\s*==\s*0x1FU' `
    -and $visibilityParser -match `
        'VisibilityConfigFormat::LegacySchema1[\s\S]*?VisibilityConfigFormat::LegacySchema4') `
    'The runtime parser lost its 4 KiB bound, five-section formatter, complete old three-section migration, or legacy schema 1-4 path.'
Assert-Contains $engine 'retainBackupAfterCommit' `
    'Setup does not distinguish temporary rollback from retained conversion backup.'
Assert-Contains $engine 'Observed UE4SS SHA-256 \(provenance only\)' `
    'The install record does not label observed UE4SS hashes as provenance only.'
Assert-NotContains $engine 'SupportedGameHash' `
    'A fixed game hash compatibility constant re-entered Setup.'
Assert-NotContains $engine `
    'HashFile\(nestedDll\)[\s\S]{0,120}ExperimentalUE4SSHash' `
    'Existing nested UE4SS compatibility is still hash-gated.'
Assert-NotContains $engine `
    'HashFile\(proxy\)[\s\S]{0,120}ExperimentalDwmapiHash' `
    'Existing proxy compatibility is still hash-gated.'
Assert-NotContains $engine `
    'Process\.Start|Stop-Process|taskkill|\.Kill\(' `
    'Setup may launch or terminate a process.'
Assert-True ([regex]::Matches($engine, 'EnsureGameIsClosed\(\);').Count -eq 6) `
    'Setup no longer enforces the expected install/update/uninstall stopped-game boundaries.'

$builder = Read-ProjectText 'tools\Build-Installer.ps1'
Assert-Contains $builder 'dist\\work\\build\\native\\main\.dll' `
    'Installer builder reads the native DLL outside dist/work.'
Assert-Contains $builder 'dist\\work\\build\\installer' `
    'Installer builder writes outside dist/work.'
Assert-Contains $builder '\$version\s*=\s*''2\.2\.1''' `
    'Installer builder version differs from the release identity.'
Assert-Contains $builder `
    '\$runtimeLabel\s*=\s*''DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1''' `
    'Installer builder runtime label differs from the release identity.'
foreach ($required in @(
        'Payload.Manifest.ini',
        'Payload.ExperimentalRuntime.zip',
        'Payload.ExperimentalUE4SS.dll',
        'Payload.ExperimentalDwmapi.dll',
        'Payload.Experimental.usmap',
        'Payload.ExperimentalUE4SS-settings.ini',
        'Payload.ThirdPartyNotices.txt')) {
    Assert-Contains $builder ([regex]::Escape($required)) `
        "Installer builder omits required resource $required."
}
Assert-NotContains $builder 'Payload\.StableRuntime|Payload\.UE4SS\.zip' `
    'Installer builder still embeds an obsolete StableRoot/manual payload.'
Assert-Contains $builder '/deterministic\+' `
    'Setup compilation is not deterministic.'
Assert-Contains $builder 'Get-AuthenticodeSignature' `
    'Setup does not verify its explicit unsigned state.'

$releaseBuilder = Read-ProjectText 'tools\Build-Release.ps1'
Assert-Contains $releaseBuilder 'dist\\work\\build\\native' `
    'Release builder writes the native build outside dist/work.'
Assert-Contains $releaseBuilder '\$version\s*=\s*''2\.2\.1''' `
    'Release builder version differs from the release identity.'
Assert-Contains $releaseBuilder 'Test-Installer\.ps1' `
    'The release builder does not run the isolated installer matrix.'
Assert-Contains $releaseBuilder 'Build-FinalPackages\.ps1' `
    'The release builder does not call the verified archive builder.'
Assert-Contains $releaseBuilder 'installerTests\.expected\s*-ne\s*20' `
    'The release builder does not require the exact 20-case matrix.'
Assert-Contains $releaseBuilder 'Test-ManualInstall\.ps1' `
    'The release builder does not run the two-channel manual-install matrix.'
Assert-Contains $releaseBuilder 'manualTests\.expected\s*-ne\s*2' `
    'The release builder does not require the exact two-case manual matrix.'
Assert-Contains $releaseBuilder 'setup_manual_payload_equivalence' `
    'The release builder does not require Setup/manual byte equivalence.'
Assert-Contains $releaseBuilder 'manual_copy_layout_validation' `
    'The release builder does not require exact manual copy-layout validation.'
Assert-Contains $releaseBuilder 'manual_clean_target_policy_validation' `
    'The release builder does not require the clean-target manual policy gate.'
Assert-NotContains $releaseBuilder `
    'StableUE4SS|StableNative|OfficialUE4SS|28 passed|-ne 28' `
    'The release builder still exposes a retired StableRoot channel.'

$packageBuilder = Read-ProjectText 'tools\Build-FinalPackages.ps1'
foreach ($requiredWorkspacePath in @(
        'dist\\work\\build\\installer',
        'dist\\work\\build\\native\\main\.dll',
        'dist\\work\\staging')) {
    Assert-Contains $packageBuilder $requiredWorkspacePath `
        "Archive builder omits contained workspace path $requiredWorkspacePath."
}
Assert-Contains $packageBuilder `
    '\$installerArchiveName\s*=\s*"\$product-v\$version-Installer\.zip"' `
    'The archive builder does not emit the canonical installer name.'
foreach ($required in @(
        'INSTALL.md',
        'THIRD_PARTY_NOTICES.txt',
        'Get-AuthenticodeSignature',
        'Expand-SafeZip',
        'CreateFromDirectory',
        'Payload.ExperimentalRuntime.zip',
        'SHA256SUMS.txt',
        'release-manifest.json')) {
    Assert-Contains $packageBuilder ([regex]::Escape($required)) `
        "The archive builder omits required release behavior: $required"
}
Assert-Contains $packageBuilder `
    '\$setupName\s*=\s*"\$product-Setup-\$version\.exe"' `
    'The archive builder does not derive the canonical Setup filename.'
foreach ($required in @(
        'Manual-No-UE4SS.zip',
        'Manual-With-UE4SS-$experimentalVersion.zip',
        'New-ManualStage',
        'README.md',
        'ue4ss\Mods\mods.txt',
        'REEXTRACTED_BYTE_IDENTICAL_THREE_PUBLIC_ZIPS')) {
    Assert-Contains $packageBuilder ([regex]::Escape($required)) `
        "The archive builder omits required manual-channel behavior: $required"
}
Assert-Contains $packageBuilder `
    '\$experimentalVersion\s*=\s*''v3\.0\.1-Beta0-g1c1a1497''' `
    'The With-UE4SS archive is not bound to the pinned Experimental version.'
Assert-NotContains $packageBuilder `
    'StableRoot|StableUE4SS|OfficialUE4SS|UE4SS_v3\.0\.1\.zip' `
    'The archive builder still exposes a retired StableRoot manual route.'

$manualTest = Read-ProjectText 'tools\Test-ManualInstall.ps1'
foreach ($required in @(
        "`$expectedTestCount = 2",
        'ManualNoUE4SSArchive',
        'ManualWithUE4SSArchive',
        'Expand-SafeZip',
        'Assert-ChecksumCoverage',
        'Assert-TreeEquivalent',
        'setup_manual_payload_equivalence',
        'manual_copy_layout_validation',
        'manual_clean_target_policy_validation',
        'Manual package contains forbidden state or scripts.')) {
    Assert-Contains $manualTest ([regex]::Escape($required)) `
        "Manual-install gate omits required behavior: $required"
}
Assert-NotContains $manualTest `
    'StableRoot|OfficialUE4SSZip|UE4SS_v3\.0\.1\.zip|IncludesUE4SSArchive' `
    'The manual-install gate still contains the retired StableRoot fixture.'

Assert-NotContains $packageBuilder `
    'Install-Manual\.(?:cmd|ps1)|Install-ManualPackage\.ps1|COPY_TO_WIN64|mods\.txt\.snippet' `
    'The script-free manual package builder still carries a manual executable script.'

foreach ($identityOwner in @(
        'Install.cmd',
        'installer\Install-DragonSwordNativeWorldRadar.ps1',
        'installer\InstallerEngine.cs',
        'installer\InstallerForm.cs',
        'installer\AssemblyInfo.cs',
        'installer\app.manifest',
        'installer\tests\InstallerExperimentalConversion.Tests.ps1',
        'installer\tests\InstallerIntegration.Tests.ps1',
        'tools\Build-Installer.ps1',
        'tools\Build-Release.ps1',
        'tools\Build-FinalPackages.ps1',
        'tools\Test-ManualInstall.ps1')) {
    $obsoleteVersion = '1.2.' + '0'
    $obsoleteRuntimeLabel =
        'DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_2_' + '0'
    Assert-NotContains (Read-ProjectText $identityOwner) `
        (([regex]::Escape($obsoleteVersion)) + '|' +
            ([regex]::Escape($obsoleteRuntimeLabel))) `
        "Obsolete runtime identity remains in $identityOwner."
}

$testWrapper = Read-ProjectText 'tools\Test-Installer.ps1'
$testMatrix = Read-ProjectText `
    'installer\tests\InstallerExperimentalConversion.Tests.ps1'
$integrationMatrix = Read-ProjectText `
    'installer\tests\InstallerIntegration.Tests.ps1'
Assert-Contains $testWrapper `
    'InstallerExperimentalConversion\.Tests\.ps1' `
    'The release gate does not call the current Experimental installer matrix.'
Assert-True ([regex]::Matches($testWrapper, '-ne 20').Count -eq 2) `
    'The installer wrapper does not require exactly 20 passed cases.'
foreach ($caseName in @(
        'Updated game hash is accepted by structural preflight',
        'Malformed game image is rejected without mutation',
        'Structurally valid changed loader hashes remain update compatible',
        'Update repair preserves user settings and refreshes bundled catalogs without backup',
        'Older structurally owned Radar version is accepted for update',
        'Injected late failure restores converted loader and Mods state',
        'Confirmed uninstall removes only strictly owned Radar state',
        'Stale uninstall confirmation is rejected without mutation',
        'Injected uninstall failure restores Radar and mods.txt')) {
    Assert-Contains $testMatrix ([regex]::Escape($caseName)) `
        "Installer matrix omits required case: $caseName"
}
Assert-True ($testMatrix -match `
        'Visibility schema 4 without assault_mode was accepted' `
    -and $testMatrix -match `
        'An unsupported assault_mode was accepted' `
    -and $testMatrix -match `
        'assault_mode must be exactly ''available'' or ''all''' `
    -and $integrationMatrix -match `
        'schema 3 visibility[\s\S]*?schema_version=3[\s\S]*?area_quest_mode=all' `
    -and $integrationMatrix -match `
        'Name = ''sectioned-2\.1\.1''[\s\S]*?\[radar\][\s\S]*?\[map\][\s\S]*?\[modes\]' `
    -and $integrationMatrix -match `
        '''auto'',\s*''en'',\s*''ja'',\s*''ko'',\s*''zh-hans'',\s*''zh-hant'',[\s\S]*?''fr'',\s*''de'',\s*''es-es'',\s*''ru'',\s*''th'',\s*''pt-br''' `
    -and $integrationMatrix -match `
        'Name = ''sectioned-2\.2\.1-''\s*\+\s*\$languageId[\s\S]*?\[height_arrows\][\s\S]*?\[interface\][\s\S]*?language=\$languageId' `
    -and $integrationMatrix -match `
        'partial, unknown-language, incomplete, or non-canonical 2\.2 visibility config was accepted' `
    -and $integrationMatrix -match `
        'Recognized \$\(\$recognizedConfig\.Name\) upgrade changed visibility\.ini bytes') `
    'Installer tests do not cover schema-4 validation, complete old three-section preservation, all 12 language preferences, and current five-section rejection cases.'

$installerForm = Read-ProjectText 'installer\InstallerForm.cs'
$installerEngine = Read-ProjectText 'installer\InstallerEngine.cs'
Assert-True ($installerForm -match 'InspectInstallationState' `
    -and $installerForm -match '_installationState\.CanInstall\s*\|\|\s*_installationState\.CanUpdate' `
    -and $installerForm -match '_installationState\.CanUninstall' `
    -and $installerForm -match '\? "Repair"[\s\S]*?: "Update"' `
    -and $installerEngine -match 'without a game-version or EXE-hash allowlist') `
    'Setup does not expose the required state-driven Install, Update, Repair, and strictly owned Uninstall controls.'

foreach ($document in @(
        'README.md',
        'PROJECT_CONTEXT.md',
        'docs\INSTALL.md',
        'docs\UPDATE_DETECTION.md',
        'docs\RELEASE.md',
        'docs\ACCEPTANCE_CHECKLIST.md',
        'installer\tests\README.md')) {
    $text = Read-ProjectText $document
    Assert-NotContains $text 'exact StableRoot and|28 passed|28-case' `
        "Obsolete StableRoot/28-case release language remains in $document."
}
foreach ($document in @(
        'docs\MANUAL_INSTALL.md',
        'docs\RELEASE.md',
        'docs\ACCEPTANCE_CHECKLIST.md')) {
    $text = Read-ProjectText $document
    Assert-Contains $text 'Manual-No-UE4SS' `
        "No-UE4SS manual channel is missing from $document."
    Assert-Contains $text 'Manual-With-UE4SS' `
        "With-UE4SS manual channel is missing from $document."
}
foreach ($document in @(
        'README.md',
        'PROJECT_CONTEXT.md',
        'docs\RELEASE.md',
        'docs\ACCEPTANCE_CHECKLIST.md',
        'installer\tests\README.md')) {
    Assert-Contains (Read-ProjectText $document) '20' `
        "Current installer test count is missing from $document."
}

foreach ($scriptName in @(
        'Build-Installer.ps1',
        'Build-FinalPackages.ps1',
        'Build-Release.ps1',
        'Deploy-NativePrototype.ps1',
        'NativeBuildReceipt.ps1',
        'ReleaseLayout.ps1',
        'Test-Installer.ps1',
        'Test-ManualInstall.ps1',
        'Verify-F6LocalizedTextOverlays.ps1',
        'Verify-ReleaseHygiene.ps1')) {
    [scriptblock]::Create((Read-ProjectText "tools\$scriptName")) | Out-Null
}

. (Join-Path $PSScriptRoot 'ReleaseLayout.ps1')
$payload = @(Get-DsnwrRuntimePayloadSpecification `
    -ProjectRoot $projectRoot `
    -DllPath (Join-Path $NativeBuildDirectory 'main.dll'))
foreach ($requiredPath in @(
        'LICENSE',
        'dlls/main.dll',
        'metadata/release.json',
        'metadata/data-providers.json',
        'metadata/native-build-receipt.json',
        'metadata/native-build-lock.json',
        'data/defaults/treasure_overrides.txt')) {
    Assert-True (@($payload | Where-Object RelativePath -eq $requiredPath).Count -eq 1) `
        "The public runtime payload omits or duplicates $requiredPath."
}
$expectedF6OverlayPaths = @(
    'assets/ui/f6/ko-fault.tga',
    'assets/ui/f6/ko-off.tga',
    'assets/ui/f6/ko-on.tga',
    'assets/ui/f6/language-popup.tga',
    'assets/ui/f6/manifest.json',
    'assets/ui/f6/zh-hant-fault.tga',
    'assets/ui/f6/zh-hant-off.tga',
    'assets/ui/f6/zh-hant-on.tga'
) | Sort-Object
$actualF6OverlayPaths = @($payload | Where-Object {
        $_.RelativePath -like 'assets/ui/f6/*'
    } | ForEach-Object { $_.RelativePath }) | Sort-Object
Assert-True (($actualF6OverlayPaths -join '|') -eq `
        ($expectedF6OverlayPaths -join '|')) `
    'The public runtime payload does not contain the exact F6 overlay set.'
Assert-True (@($payload | Where-Object {
            $_.RelativePath -match `
                '(?i)(^|/)enabled\.txt$|(^|/)runtime/(logs|diagnostics|backups)(/|$)|(^|/)config/(visibility|diagnostics)\.ini$'
        }).Count -eq 0) `
    'The public embedded runtime contains live local state or legacy load authority.'

. (Join-Path $PSScriptRoot 'NativeBuildReceipt.ps1')
$dll = Join-Path $NativeBuildDirectory 'main.dll'
$receipt = Join-Path $NativeBuildDirectory 'native-build-receipt.json'
$present = @($dll, $receipt | Where-Object {
        Test-Path -LiteralPath $_ -PathType Leaf
    }).Count
Assert-True ($present -in @(0, 2)) 'A partial local native build stage is present.'
if ($present -eq 2) {
    Test-DsnwrNativeBuildReceipt -ProjectRoot $projectRoot `
        -DllPath $dll -ReceiptPath $receipt | Out-Null
}

Write-Host "Release hygiene gates passed for $version."
