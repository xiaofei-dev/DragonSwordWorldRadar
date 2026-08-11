#requires -Version 5.1
$ErrorActionPreference='Stop';Set-StrictMode -Version 2.0
$root=Split-Path -Parent $PSScriptRoot
if($PSVersionTable.PSVersion.Major-ne 5){throw 'Windows PowerShell 5.1 is required.'}
Add-Type -AssemblyName System.Windows.Forms;Add-Type -AssemblyName System.Drawing;Add-Type -AssemblyName System.Xml
$sources=@(Get-ChildItem -LiteralPath (Join-Path $root 'src\overlay') -Recurse -Filter '*.cs' -File|Where-Object{$_.Name-ne'Program.cs'-and$_.FullName-notmatch'[\\/](?:obj|bin)[\\/]'}|Sort-Object FullName|Select-Object -ExpandProperty FullName)
$sources+=@(Get-ChildItem -LiteralPath (Join-Path $root 'src\installer\Core') -Recurse -Filter '*.cs' -File|Where-Object{$_.FullName-notmatch'[\\/](?:obj|bin)[\\/]'}|Sort-Object FullName|Select-Object -ExpandProperty FullName)
$sources+=(Join-Path $PSScriptRoot 'AssaultIntegrationHarness.cs')
$refs=@([System.Windows.Forms.Form].Assembly.Location,[System.Drawing.Graphics].Assembly.Location,[System.Xml.XmlDocument].Assembly.Location,[System.Security.Cryptography.Aes].Assembly.Location,[System.Linq.Enumerable].Assembly.Location,[System.Uri].Assembly.Location)|Select-Object -Unique
$temp=Join-Path ([IO.Path]::GetTempPath()) ('DSWR-Assault-'+[Guid]::NewGuid().ToString('N'));New-Item -ItemType Directory -Path $temp|Out-Null
try{$exe=Join-Path $temp 'AssaultTests.exe';Add-Type -Path $sources -ReferencedAssemblies $refs -OutputAssembly $exe -OutputType ConsoleApplication -ErrorAction Stop;$output=& cmd.exe /d /c ('"'+$exe+'" 2>&1');$code=$LASTEXITCODE;$output|ForEach-Object{Write-Host $_};if($code-ne 0-or($output-join"`n")-notmatch'ASSAULT_TESTS_OK'){throw "Assault executable tests failed (exit=$code)."}}
finally{Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue}
$runtime=Get-Content -Raw (Join-Path $root 'src\overlay\SaveData\EncounterAvailabilityTracker.cs')
if($runtime.Contains('143')){throw 'Runtime must not special-case CID 143.'}
$renderer=Get-Content -Raw (Join-Path $root 'src\overlay\Rendering\AssaultMarkerRenderer.cs')
if($renderer-match'new PointF\s*\['-and$renderer-notmatch'private readonly PointF\[\]'){throw 'Renderer geometry must be retained.'}
$drawBody=$renderer.Substring($renderer.IndexOf('public void DrawMarker'),$renderer.IndexOf('private void DrawSword')-$renderer.IndexOf('public void DrawMarker'))
if($drawBody-match'new\s+PointF\s*\[|new\s+Pen|new\s+SolidBrush'){throw 'Assault DrawMarker must not allocate geometry arrays or GDI resources.'}
$tracker=Get-Content -Raw (Join-Path $root 'src\overlay\SaveData\EncounterAvailabilityTracker.cs')
if($tracker-match'new\s+BossPoint|new\s+WorldAssault'){throw 'Unified encounter refresh must not allocate feature adapters.'}
$main=Get-Content -Raw (Join-Path $root 'src\ue4ss\main.lua')
if($main-match'FindAllOf|RegisterHook'){throw 'Assault candidate must preserve no-scan/no-hook architecture.'}
$defaultConfig=Get-Content -Raw (Join-Path $root 'src\ue4ss\config.default.lua')
if($defaultConfig-match'(?m)^\s*show_groundhog\s*=|(?m)^\s*show_assault\s*='){throw 'Stale singular Assault/Groundhog defaults remain.'}
if($defaultConfig-match'(?m)^\s*assault_performance_ab_isolation\s*='-or$defaultConfig-notmatch'(?m)^\s*show_assaults\s*=\s*true\s*,'){throw 'Assault diagnostic reproduction default contract is missing.'}
$debugSettings=Get-Content -Raw (Join-Path $root 'src\overlay\Configuration\DebugSettings.cs')
if(-not$debugSettings.Contains('LoadBooleanAtStartup("show_assaults", true)')){throw 'Overlay Assault diagnostic reproduction default is missing.'}
foreach($obsolete in @('AssaultIsolationSettingName','StartupAssaultIsolation','assault_performance_ab_isolation')){if($debugSettings.Contains($obsolete)-or$main.Contains($obsolete)){throw "Obsolete Assault A/B isolation remains active: $obsolete"}}
if(-not$main.Contains('local show_assaults = config.show_assaults ~= false')){throw 'Lua Assault production gate is missing.'}
$radar=Get-Content -Raw (Join-Path $root 'src\overlay\UI\RadarForm.cs')
foreach($obsolete in @('_worldAssaults','_worldBosses','_assaultAvailability','_bossAvailability','ConfigureAssaultTargets')){if($radar.Contains($obsolete)){throw "Separate Boss/Assault runtime path remains: $obsolete"}}
foreach($required in @('_worldEncounters.Load();','ConfigureEncounterTargets(','_encounterAvailability.Refresh(','RecordEncounterDraw(','WORLD_ENCOUNTER_LIFECYCLE')){if(-not$radar.Contains($required)){throw "Unified encounter runtime marker is missing: $required"}}
$constructor=$radar.Substring($radar.IndexOf('public RadarForm()'),$radar.IndexOf('protected override void OnPaint')-$radar.IndexOf('public RadarForm()'))
if(-not$constructor.Contains('_worldEncounters.Load();')-or-not$constructor.Contains('ConfigureEncounterTargets(')){throw 'Encounter catalog/filter must be fixed during Overlay construction.'}
$maintenance=$radar.Substring($radar.IndexOf('if (now >= _nextMaintenanceUtc)'),$radar.IndexOf('string rawMode = GetEffectiveMode();')-$radar.IndexOf('if (now >= _nextMaintenanceUtc)'))
if($maintenance.Contains('_worldEncounters.Load')-or$maintenance.Contains('ConfigureEncounterTargets')){throw 'F7/maintenance must not load or reconfigure encounter targets.'}
$saveState=Get-Content -Raw (Join-Path $root 'src\overlay\SaveData\TreasureSaveState.cs')
$snapshotReader=Get-Content -Raw (Join-Path $root 'src\overlay\SaveData\SaveSnapshotReader.cs')
foreach($marker in @('ConfigureEncounterTargets','request.EncounterTargetIds','WORLD_ENCOUNTER_SAVE_FILTER_CONFIGURED','Overlay startup','cacheReset=false')){if(-not$saveState.Contains($marker)){throw "Startup-fixed encounter save-filter marker is missing: $marker"}}
foreach($marker in @('BuildActorFilterSql(encounterTargetIds)','ActorFilterSignature','NormalizeEncounterTargetIds')){if(-not$snapshotReader.Contains($marker)){throw "Unified encounter SQL/cache marker is missing: $marker"}}
foreach($hardcoded in @('private static readonly int[] AssaultTargetIds','private static readonly int[] WorldBossIds','102,105,106,110','9000005, 9000007')){if($saveState.Contains($hardcoded)-or$snapshotReader.Contains($hardcoded)){throw "Hardcoded encounter target list remains: $hardcoded"}}
$availability=Get-Content -Raw (Join-Path $root 'src\overlay\SaveData\EncounterAvailabilityTracker.cs')
foreach($marker in @('IList<WorldEncounter>','GetEncounterNextAvailableUtc','world_time_window','Future weather','return false;')){if(-not$availability.Contains($marker)){throw "Unified encounter condition marker is missing: $marker"}}
$catalog=Get-Content -Raw (Join-Path $root 'src\overlay\Data\WorldEncounterCatalog.cs')
foreach($marker in @('ExpectedBossCount = 9','ExpectedAssaultCount = 40','WORLD_ENCOUNTER_CATALOG_LOADED','lifecycle=overlay_startup_once','f7_reconfiguration=false')){if(-not$catalog.Contains($marker)){throw "Unified encounter catalog marker is missing: $marker"}}
$install=Get-Content -Raw (Join-Path $root 'src\installer\Install.ps1')
foreach($required in @('identityBeforeData','identityAfterData','game_fingerprint_before_generation','game_fingerprint_after_generation','Game executable or PAK identity changed while datasets were being generated')){if(-not$install.Contains($required)){throw "Install fingerprint transaction contract missing: $required"}}
foreach($required in @('Migrate-AssaultConfig','ENABLED diagnostic Assault crash reproduction','show_assaults = true')){if(-not$install.Contains($required)){throw "Installer Assault diagnostic migration is missing: $required"}}
if($install-match'assaultConditionCount\s*-ne\s*1'){throw 'Install must derive expected Assault condition count from policy.'}
foreach($required in @('expectedAssaultConditionCount','SelectNodes(''./Condition'')','Game executable or PAK identity changed after dataset validation','40 Assault')){if(-not$install.Contains($required)){throw "Install Assault policy/final-identity contract missing: $required"}}
if($install-notmatch'\(\?m\)\^\\s\*\\\{\[\^\\r\\n\]\*\\bgame_fingerprint'){throw 'Installer Assault fingerprint validation must count data records only, not the generated header comment.'}
$policy=[xml](Get-Content -Raw (Join-Path $root 'metadata\assault-inference-policy.xml'))
$condition=$policy.AssaultInferencePolicy.Condition
foreach($required in @('place_id','cid','uid','uid_name','reveal_cycle_id','condition_type','provenance','missing_confirmation')){if([string]::IsNullOrWhiteSpace([string]$condition.$required)){throw "Inference policy field missing: $required"}}
if(([string]$policy.AssaultInferencePolicy.expected_game_fingerprint)-notmatch'^[0-9a-f]{64}$'){throw 'Inference policy fingerprint is invalid.'}
$extractor=Get-Content -Raw (Join-Path $root 'src\installer\Core\Pak\TreasurePakExtractor.cs')
$methodStart=$extractor.IndexOf('private static bool TryExtractMatchingEntry');$methodEnd=$extractor.IndexOf('private static bool TryWriteValidatedEntry');$dispatch=$extractor.Substring($methodStart,$methodEnd-$methodStart)
$revealIndex=$dispatch.IndexOf('String.Equals(directoryEntry.FileName, "RevealCycleData.xml"');$genericIndex=$dispatch.IndexOf('ReadEncodedEntry(')
if($revealIndex-lt 0-or$genericIndex-lt 0-or$revealIndex-gt$genericIndex-or-not$dispatch.Contains('return TryExtractExactRevealCycleEntry')){throw 'RevealCycle exact terminal dispatch must precede every generic decoder.'}
$refresh=Get-Content -Raw (Join-Path $root 'build\Refresh-SourceMetadata.ps1')
if(-not$refresh.Contains("Add-Mapping `$mappings 'metadata\assault-inference-policy.xml' 'metadata\assault-inference-policy.xml'")){throw 'Inference policy source-release mapping is missing.'}
$releaseMap=Get-Content -Raw (Join-Path $root 'metadata\source-release-map.json')|ConvertFrom-Json
$policyMappings=@($releaseMap.mappings|Where-Object{$_.source_path-eq'metadata/assault-inference-policy.xml'-and$_.release_path-eq'metadata/assault-inference-policy.xml'})
if($policyMappings.Count-ne 1-or-not$policyMappings[0].byte_identical){throw 'Inference policy source-release bytes are not mapped identically.'}
$mainLog=Get-Content -Raw (Join-Path $root 'src\ue4ss\main.lua')
if(-not$mainLog.Contains('world-boss, Assault, Mole/Fly')){throw 'Lua Overlay delegation log omits Assault.'}
if($mainLog.Contains('ASSAULT_LIFECYCLE')){throw 'Lua F7/transition path must not contain Assault-specific lifecycle work.'}
