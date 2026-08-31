[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("dsnap-modstxt-test-" + [Guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null
    $path = Join-Path $tempRoot 'mods.txt'
    [IO.File]::WriteAllText($path, "OtherMod : 1`r`n; DragonSwordNativeAutoPickup : 0`r`n", [Text.UTF8Encoding]::new($false))
    & (Join-Path $PSScriptRoot 'Update-ModsTxt.ps1') -ModsTxtPath $path -Enabled 1 | Out-Null
    $first = [IO.File]::ReadAllText($path)
    if ($first -notmatch '(?m)^OtherMod : 1\r?$' -or
        $first -notmatch '(?m)^; DragonSwordNativeAutoPickup : 0\r?$' -or
        [regex]::Matches($first, '(?m)^DragonSwordNativeAutoPickup : 1\r?$').Count -ne 1) {
        throw 'Missing-entry merge did not preserve unrelated lines and comments.'
    }

    & (Join-Path $PSScriptRoot 'Update-ModsTxt.ps1') -ModsTxtPath $path -Enabled 0 | Out-Null
    $second = [IO.File]::ReadAllText($path)
    if ([regex]::Matches($second, '(?m)^DragonSwordNativeAutoPickup : 0\r?$').Count -ne 1) {
        throw 'Existing-entry update did not produce exactly one disabled entry.'
    }

    [IO.File]::WriteAllText($path, "DragonSwordNativeAutoPickup : 0`nDragonSwordNativeAutoPickup : 1`n",
                            [Text.UTF8Encoding]::new($false))
    & (Join-Path $PSScriptRoot 'Update-ModsTxt.ps1') -ModsTxtPath $path -Enabled 1 | Out-Null
    $normalized = [IO.File]::ReadAllText($path)
    if ([regex]::Matches($normalized, '(?m)^DragonSwordNativeAutoPickup : 1\r?$').Count -ne 1 -or
        [regex]::Matches($normalized, '(?m)^DragonSwordNativeAutoPickup : 0\r?$').Count -ne 0) {
        throw 'Duplicate AutoPickup entries were not normalized to one authoritative entry.'
    }

    Write-Output 'MODS_TXT_CONTROL_VALIDATION PASSED'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) { Remove-Item -LiteralPath $tempRoot -Recurse -Force }
}
