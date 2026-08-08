param([Parameter(Mandatory=$true)][string]$ContextPath)
$ErrorActionPreference='Stop'
$moduleDir=Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir=(Get-Item -LiteralPath $moduleDir).Parent.Parent.FullName
. (Join-Path $toolsDir 'Common.ps1')
. (Join-Path $toolsDir 'core\ModuleApi.ps1')
$ctx=Import-ModuleContext -ContextPath $ContextPath

try {
    Write-ModuleProgress -Context $ctx -Step 1 -Total 5 `
        -Stage 'resolve_game_layout' `
        -Detail 'Resolving Win64, DS root and game root.'
    $probeRoot=[IO.Path]::GetFullPath([string]$ctx.root).TrimEnd('\')
    $modsRoot=Split-Path -Parent $probeRoot
    $win64=Split-Path -Parent $modsRoot
    $binaries=Split-Path -Parent $win64
    $dsRoot=Split-Path -Parent $binaries
    $gameRoot=Split-Path -Parent $dsRoot

    Write-ModuleProgress -Context $ctx -Step 2 -Total 5 `
        -Stage 'validate_game_files' `
        -Detail 'Checking executable and target PAK.'

    $exe=Join-Path $win64 'DSClient-Win64-Shipping.exe'
    $pak=Join-Path $gameRoot 'DS\Content\Paks\pakchunk109-WindowsClient.pak'

    if(-not(Test-Path -LiteralPath $exe -PathType Leaf)){throw ('game executable missing: '+$exe)}
    if(-not(Test-Path -LiteralPath $pak -PathType Leaf)){throw ('target PAK missing: '+$pak)}

    $exeItem=Get-Item -LiteralPath $exe
    $pakItem=Get-Item -LiteralPath $pak

    Write-ModuleProgress -Context $ctx -Step 3 -Total 5 `
        -Stage 'build_fingerprint' `
        -Detail 'Computing executable/PAK fingerprint.'

    $source=
        $exeItem.Length.ToString()+'|'+$exeItem.LastWriteTimeUtc.Ticks.ToString()+'|'+
        $pakItem.Length.ToString()+'|'+$pakItem.LastWriteTimeUtc.Ticks.ToString()

    $sha=[Security.Cryptography.SHA256]::Create()
    try {
        $fingerprint=([BitConverter]::ToString(
            $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($source))
        )).Replace('-','').ToLowerInvariant()
    }
    finally {$sha.Dispose()}

    $payload=[ordered]@{
        schema_version=1
        build_fingerprint=$fingerprint
        probe_root=$probeRoot
        mods_root=$modsRoot
        win64=$win64
        ds_root=$dsRoot
        game_root=$gameRoot
        exe_path=$exe
        pak_path=$pak
        exe_size=$exeItem.Length
        pak_size=$pakItem.Length
        generated_utc=(Get-Date).ToUniversalTime().ToString('o')
    }

    Write-ModuleProgress -Context $ctx -Step 4 -Total 5 `
        -Stage 'write_game_layout' `
        -Detail ('fingerprint='+$fingerprint)

    $output=Join-Path ([string]$ctx.output_dir) 'game-layout.json'
    Write-JsonUtf8NoBom -Path $output -Value $payload -Depth 8

    Write-ModuleProgress -Context $ctx -Step 5 -Total 5 `
        -Stage 'finalize_static_inventory' `
        -Detail 'Writing module result.'

    Write-ModuleResult -Context $ctx -Status success `
        -Metrics @{file_count=2} `
        -Outputs @($output) `
        -Blockers @() `
        -NextAction 'Game layout/build fingerprint confirmed. Runtime assault snapshot may be analyzed without PAK.' `
        -Fingerprint $fingerprint | Out-Null
}
catch {
    $message=Get-ExceptionSummary -ErrorObject $_
    Write-ModuleLog -Context $ctx -Event 'ERROR' -Message $message
    Write-ModuleResult -Context $ctx -Status blocked -Blockers @($message) `
        -NextAction 'Verify direct Win64\Mods installation.' | Out-Null
    exit 1
}
