$script:StaticCacheSchema = 'pak-static-v7-assault-runtime-table-xor16'

function Get-StaticCacheFingerprint {
    param(
        [Parameter(Mandatory=$true)][string]$ProbeRoot
    )

    $probeRootFull=[IO.Path]::GetFullPath($ProbeRoot).TrimEnd('\')
    $modsRoot=Split-Path -Parent $probeRootFull
    $win64=Split-Path -Parent $modsRoot
    $binaries=Split-Path -Parent $win64
    $dsRoot=Split-Path -Parent $binaries
    $gameRoot=Split-Path -Parent $dsRoot

    $exe=Join-Path $win64 'DSClient-Win64-Shipping.exe'
    $pak=Join-Path $gameRoot 'DS\Content\Paks\pakchunk109-WindowsClient.pak'

    if(-not(Test-Path -LiteralPath $exe -PathType Leaf)){
        throw ('game executable missing: '+$exe)
    }
    if(-not(Test-Path -LiteralPath $pak -PathType Leaf)){
        throw ('target PAK missing: '+$pak)
    }

    $exeItem=Get-Item -LiteralPath $exe
    $pakItem=Get-Item -LiteralPath $pak

    $buildSource=
        $exeItem.Length.ToString()+'|'+
        $exeItem.LastWriteTimeUtc.Ticks.ToString()+'|'+
        $pakItem.Length.ToString()+'|'+
        $pakItem.LastWriteTimeUtc.Ticks.ToString()

    $sha=[Security.Cryptography.SHA256]::Create()
    try{
        $buildFingerprint=([BitConverter]::ToString(
            $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($buildSource))
        )).Replace('-','').ToLowerInvariant()

        $cacheSource=$script:StaticCacheSchema+'|'+$buildFingerprint
        $cacheFingerprint=([BitConverter]::ToString(
            $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($cacheSource))
        )).Replace('-','').ToLowerInvariant()
    }
    finally{
        $sha.Dispose()
    }

    [pscustomobject]@{
        cache_schema=$script:StaticCacheSchema
        build_fingerprint=$buildFingerprint
        fingerprint=$cacheFingerprint
        probe_root=$probeRootFull
        win64=$win64
        ds_root=$dsRoot
        game_root=$gameRoot
        exe_path=$exe
        pak_path=$pak
        exe_size=$exeItem.Length
        pak_size=$pakItem.Length
        exe_mtime_utc=$exeItem.LastWriteTimeUtc.ToString('o')
        pak_mtime_utc=$pakItem.LastWriteTimeUtc.ToString('o')
    }
}

function Get-StaticCacheRoot {
    param(
        [Parameter(Mandatory=$true)][string]$ProbeRoot,
        [Parameter(Mandatory=$true)][string]$Fingerprint
    )
    Join-Path $ProbeRoot ('runtime\cache\static\'+$Fingerprint)
}
