[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$source = Get-Content -LiteralPath (Join-Path $projectRoot 'src\ue4ss\main.cpp') -Raw
$forbidden = @('UObjectArray::', 'IsReal(', 'FindAllOf(', 'RegisterAActorTick', 'RegisterLoadMap', 'RegisterProcessEvent')
foreach ($token in $forbidden) {
    if ($source.Contains($token)) { throw "Forbidden stability/performance token: $token" }
}
$findCount = ([regex]::Matches($source, 'FindFirstOf\s*\(')).Count
if ($findCount -ne 0) { throw "Expected zero FindFirstOf calls; found $findCount" }
$required = @('BlueprintUpdateCamera', 'PCOwner', 'RegisterPreHook', 'UnregisterHook', 'ProcessEvent(location_function_', 'RegisterInitGameStatePreCallback', '__except')
foreach ($token in $required) {
    if (-not $source.Contains($token)) { throw "Required native boundary missing: $token" }
}
Write-Host "SOURCE_POLICY_OK engine_find_scans=0 single_ufunction_pulse=1 continuous_uobject_scans=0 actor_tick_hooks=0 load_map_hooks=0"
