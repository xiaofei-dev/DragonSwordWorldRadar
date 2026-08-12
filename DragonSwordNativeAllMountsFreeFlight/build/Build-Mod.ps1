param(
    [Parameter(Mandatory)][string]$RepakPath
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$stagingRoot = Join-Path $projectRoot 'staging'
$contentRoot = Join-Path $stagingRoot 'DS\Content'
$distRoot = Join-Path $projectRoot 'dist'
$outputPak = Join-Path $distRoot 'DS_NativeAllMountsFreeFlight_P.pak'

if (-not (Test-Path -LiteralPath $RepakPath -PathType Leaf)) {
    throw "repak was not found: $RepakPath"
}

$xmlPath = Join-Path $stagingRoot 'DS\Content\__GeneratedGameData__\Server\XML\GameData\VehicleComboData.xml'
$tablePath = Join-Path $stagingRoot 'DS\Content\Design\GameData\VehicleComboData.table'
foreach ($requiredPath in @($xmlPath, $tablePath)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Required staged file was not found: $requiredPath"
    }
}

[xml]$document = Get-Content -LiteralPath $xmlPath -Raw -Encoding UTF8
$rows = @($document.SelectNodes('//*[local-name()="VehicleComboData"]'))
$freeDashRows = @($rows | Where-Object { $_.Index -eq '10' })
if ($rows.Count -ne 330 -or $freeDashRows.Count -ne 30 -or
    @($freeDashRows | Where-Object {
        $_.ActionStateType -eq 'GLIDE_FREE_DASH' -and
        $_.OpenCondition -eq 'NONE_CONDITION' -and
        $_.CoolTime -eq '3'
    }).Count -ne 30) {
    throw 'The staged combo XML does not contain 30 native free-dash entries.'
}

$tableText = Get-Content -LiteralPath $tablePath -Raw -Encoding UTF8
$freeDashPattern = '"Index"\s*:\s*10(?s:.*?)"ActionStateType"\s*:\s*"GLIDE_FREE_DASH"(?s:.*?)"OpenCondition"\s*:\s*"NONE_CONDITION"(?s:.*?)"CoolTime"\s*:\s*3'
if ([regex]::Matches($tableText, '"Index"\s*:\s*10').Count -ne 30 -or
    [regex]::Matches($tableText, '"ActionStateType"\s*:\s*"GLIDE_FREE_DASH"').Count -ne 30 -or
    [regex]::Matches($tableText, $freeDashPattern).Count -ne 30) {
    throw 'The staged combo table does not contain 30 native free-dash entries.'
}

New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
if (Test-Path -LiteralPath $outputPak) {
    Remove-Item -LiteralPath $outputPak -Force
}

& $RepakPath pack --version V4 --mount-point '../../../DS/Content/' $contentRoot $outputPak
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $outputPak -PathType Leaf)) {
    throw 'repak did not produce the expected PAK.'
}

& $RepakPath list $outputPak
Get-FileHash -LiteralPath $outputPak -Algorithm SHA256
