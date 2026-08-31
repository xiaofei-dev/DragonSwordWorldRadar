[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ExtractedRoot,
    [ValidateSet(3, 5, 10, 15, 20)][int]$Multiplier = 5
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$projectRoot = Split-Path -Parent $PSScriptRoot
$manifest = Get-Content -LiteralPath (Join-Path $projectRoot 'metadata\targets-x5.json') -Raw |
    ConvertFrom-Json
$root = [IO.Path]::GetFullPath($ExtractedRoot)
if (-not (Test-Path -LiteralPath $root -PathType Container)) {
    throw "Semantic extraction root was not found: $root"
}

$verified = 0
foreach ($target in $manifest.targets) {
    $relative = 'DS\Content\' + ([string]$target.path).Replace('/', '\') + '.properties.json'
    $propertiesPath = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $propertiesPath -PathType Leaf)) {
        throw "Semantic export is missing: $relative"
    }
    $exports = Get-Content -LiteralPath $propertiesPath -Raw | ConvertFrom-Json
    $components = @($exports | Where-Object {
        [string]$_.Type -eq 'CapsuleComponent' -and
        [string]$_.Name -eq [string]$target.component
    })
    if ($components.Count -ne 1) {
        throw "Expected exactly one approved capsule export for $($target.path); found $($components.Count)"
    }
    $scale = $components[0].Properties.RelativeScale3D
    $actual = @([double]$scale.X, [double]$scale.Y, [double]$scale.Z)
    $expected = @($target.old_scale | ForEach-Object { [double]$_ * [double]$Multiplier })
    for ($axis = 0; $axis -lt 3; $axis++) {
        if ([Math]::Abs($actual[$axis] - $expected[$axis]) -gt 0.0000001) {
            throw "Unexpected semantic scale for $($target.path), axis $axis`: $($actual -join ',')"
        }
    }

    if ([string]$target.class -eq 'normal_gather') {
        $interactables = @($exports | Where-Object { [string]$_.Type -eq 'DInteractableComponent' })
        if ($interactables.Count -lt 1 -or
            @($interactables | Where-Object {
                [string]$_.Properties.InteractTypeValue -eq 'EInteractTypeValue::NormalGather'
            }).Count -lt 1) {
            throw "NormalGather semantic contract is missing: $($target.path)"
        }
    }
    if ([string]$target.class -eq 'normal_gather_inherited') {
        $classes = @($exports | Where-Object { [string]$_.Type -eq 'BlueprintGeneratedClass' })
        $interactables = @($exports | Where-Object { [string]$_.Type -eq 'DInteractableComponent' })
        if (@($classes | Where-Object {
            [string]$_.SuperStruct.ObjectName -eq "Class'DsAnimationProp'"
        }).Count -ne 1 -or $interactables.Count -lt 1) {
            throw "Inherited production-gather semantic contract is missing: $($target.path)"
        }
    }
    if ([string]$target.class -eq 'animal') {
        $classes = @($exports | Where-Object { [string]$_.Type -eq 'BlueprintGeneratedClass' })
        if (@($classes | Where-Object {
            [string]$_.SuperStruct.ObjectName -eq "Class'DsInteractableAnimal'"
        }).Count -ne 1) {
            throw "DsInteractableAnimal semantic contract is missing: $($target.path)"
        }
    }
    $verified++
}

$propertyFiles = @(Get-ChildItem -LiteralPath $root -Recurse -Filter '*.properties.json' -File)
if ($verified -ne 50 -or $propertyFiles.Count -ne 50) {
    throw "Semantic inventory mismatch: verified=$verified properties=$($propertyFiles.Count)"
}
if (@($propertyFiles | Where-Object { $_.FullName -match '(?i)treasure|chest|box' }).Count -ne 0) {
    throw 'Semantic extraction contains a denied treasure path.'
}

Write-Output 'SEMANTIC_VALIDATION PASSED'
Write-Output "MULTIPLIER $Multiplier"
Write-Output 'AUTHORED_CAPSULE_TARGETS 50'
Write-Output 'DROP_ITEM_TARGETS VERIFIED_BY_STRUCTURED_UASSET_ROUNDTRIP_IN_BUILD_RELEASE 19'
Write-Output 'TREASURE_TARGETS 0'
Write-Output 'GAMEPLAY_ACCEPTANCE NOT_VALIDATED'
