[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot

function Test-LocalOnlyPath([string]$Path) {
    $normalized = $Path.Replace('\', '/')
    return $normalized -match '(?i)(^|/)assets/(nexus|screenshots)/' -or
        $normalized -match '(?i)(^|/)NEXUS_[^/]+\.(txt|md)$' -or
        $normalized -match '^DragonSwordWorldDataProbe/(reference/|metadata/world-boss-catalog\.json$)' -or
        $normalized -match '(?i)(^|/)(dist|out|runtime|node_modules|__pycache__|\.sdk)/' -or
        $normalized -match '(?i)\.(zip|7z|pak|ucas|utoc|db|sqlite|log|dmp|key|exe|dll|pdb|pyc|obj|ilk|bak)$' -or
        $normalized -match '(?i)(^|/)\.env($|\.local$|\..*\.local$)' -or
        $normalized -match '^DragonSwordNativeWorldRadarPostRender/src/data/generated/'
}

# The guard must distinguish promotional assets from required implementation.
foreach ($path in @('Mod/assets/nexus/cover.png', 'Mod/assets/screenshots/map.png',
    'Mod/docs/NEXUS_FAQ.md', 'DragonSwordWorldDataProbe/reference/table.xml',
    'DragonSwordWorldDataProbe/metadata/world-boss-catalog.json',
    'Mod/dist/package.zip', 'Mod/runtime/session.log', 'Mod/.env.local')) {
    if (-not (Test-LocalOnlyPath $path)) { throw "Guard missed local-only path: $path" }
}
foreach ($path in @('Mod/src/main.cpp', 'Mod/LICENSE', 'Mod/tests/core_tests.cpp',
    'Mod/assets/ui/f6/language-popup.tga', 'Mod/assets/ui/f6/manifest.json',
    'docs/DEPENDENCY_SOURCES.md')) {
    if (Test-LocalOnlyPath $path) { throw "Guard excluded source input: $path" }
}

# git ls-files reads the index, so this also checks staged removals pre-commit.
$tracked = @(& git -C $repositoryRoot ls-files)
if ($LASTEXITCODE -ne 0) { throw 'Unable to read repository index.' }
$violations = @($tracked | Where-Object { Test-LocalOnlyPath $_ })
if ($violations.Count) {
    throw "Local-only files are still tracked:`n$($violations -join "`n")"
}
Write-Output "Public source boundary passed: $($tracked.Count) tracked files; 14 path fixtures."
