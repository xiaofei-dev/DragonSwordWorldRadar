[CmdletBinding()]
param([string]$PythonPath)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$verifier = Join-Path $PSScriptRoot 'Verify-F6LocalizedTextOverlays.py'

if ([string]::IsNullOrWhiteSpace($PythonPath)) {
    $candidatePaths = @()
    $userProfilePath = $env:USERPROFILE
    if (-not [string]::IsNullOrWhiteSpace($userProfilePath)) {
        $candidatePaths += Join-Path $userProfilePath `
            '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
    }
    $pathPython = Get-Command python.exe -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty Source -First 1
    if (-not [string]::IsNullOrWhiteSpace($pathPython)) {
        $candidatePaths += $pathPython
    }
    $candidatePaths = $candidatePaths | Where-Object {
        Test-Path -LiteralPath $_ -PathType Leaf
    } | Select-Object -Unique
    foreach ($candidatePath in $candidatePaths) {
        & $candidatePath -c 'import PIL' 2>$null
        if ($LASTEXITCODE -eq 0) {
            $PythonPath = $candidatePath
            break
        }
    }
}
if ([string]::IsNullOrWhiteSpace($PythonPath) -or
    -not (Test-Path -LiteralPath $PythonPath -PathType Leaf)) {
    throw 'Python 3 with Pillow was not found for the F6 localized-overlay gate.'
}
& $PythonPath -c 'import PIL' 2>$null
if ($LASTEXITCODE -ne 0) {
    throw "Python 3 does not provide Pillow: $PythonPath"
}
if (-not (Test-Path -LiteralPath $verifier -PathType Leaf)) {
    throw "F6 localized-overlay verifier is missing: $verifier"
}

& $PythonPath $verifier --project-root $projectRoot
if ($LASTEXITCODE -ne 0) {
    throw "F6 localized-overlay verification failed: $LASTEXITCODE"
}
