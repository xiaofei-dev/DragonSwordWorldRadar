param(
    [Parameter(Mandatory=$true)]
    [string]$ContextPath
)

$ErrorActionPreference='Stop'

$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName

. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
. (Join-Path $toolsDir 'core\StaticCache.ps1')

$ctx=Import-ModuleContext -ContextPath $ContextPath

try{
    Write-Host '[MOLE 1/3] Loading shared PAK cache...'

    $fp=Get-StaticCacheFingerprint -ProbeRoot ([string]$ctx.root)
    $cacheRoot=Get-StaticCacheRoot -ProbeRoot ([string]$ctx.root) -Fingerprint ([string]$fp.fingerprint)
    $cacheStatic=Join-Path $cacheRoot 'assault-static'

    $localProbe=Join-Path $moduleDir 'mole_static_probe'
    Remove-Item -LiteralPath $localProbe -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $localProbe | Out-Null

    if(Test-Path -LiteralPath $cacheStatic -PathType Container){
        Copy-Item -LiteralPath $cacheStatic\* -Destination $localProbe -Recurse -Force
    }
    else{
        throw 'Shared static PAK cache is missing. pak_static must run first.'
    }

    Write-Host '[MOLE 2/3] Analyzing all 40 Mole nodes and related static tables...'

    & (Join-Path $moduleDir 'Analyze.ps1') -ContextPath $ContextPath

    Write-Host '[MOLE 3/3] Mole discovery complete.'
}
catch{
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleResult -Context $ctx -Status 'error' `
        -Blockers @($message) `
        -NextAction 'Review mole discovery logs.' | Out-Null
    exit 1
}
