[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ModsTxtPath,
    [ValidateSet(0, 1)][int]$Enabled = 1
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$path = [IO.Path]::GetFullPath($ModsTxtPath)
if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
    throw "mods.txt was not found: $path"
}

$bytes = [IO.File]::ReadAllBytes($path)
$hasUtf8Bom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
$encoding = [Text.UTF8Encoding]::new($hasUtf8Bom)
$text = [IO.File]::ReadAllText($path, $encoding)
$newline = if ($text.Contains("`r`n")) { "`r`n" } else { "`n" }
$pattern = '(?m)^[\t ]*DragonSwordNativeAutoPickup[\t ]*:[\t ]*[01][\t ]*(?=\r?$)'
$matches = [regex]::Matches($text, $pattern)

$entry = "DragonSwordNativeAutoPickup : $Enabled"
if ($matches.Count -ge 1) {
    $updated = $text
    for ($index = $matches.Count - 1; $index -ge 1; $index--) {
        $duplicate = $matches[$index]
        $updated = $updated.Remove($duplicate.Index, $duplicate.Length)
    }
    $first = [regex]::Match($updated, $pattern)
    $updated = $updated.Remove($first.Index, $first.Length).Insert($first.Index, $entry)
} else {
    $separator = if ($text.Length -eq 0 -or $text.EndsWith("`n")) { '' } else { $newline }
    $updated = $text + $separator + $entry + $newline
}

[IO.File]::WriteAllText($path, $updated, $encoding)
$verify = [IO.File]::ReadAllText($path, $encoding)
$active = [regex]::Matches($verify, '(?m)^[\t ]*DragonSwordNativeAutoPickup[\t ]*:[\t ]*' + $Enabled + '[\t ]*(?=\r?$)')
$allEntries = [regex]::Matches($verify, $pattern)
if ($active.Count -ne 1 -or $allEntries.Count -ne 1) {
    $escaped = $verify.Replace("`r", '<CR>').Replace("`n", '<LF>')
    throw "mods.txt verification failed after update: expected_enabled=$Enabled active=$($active.Count) all=$($allEntries.Count) text=$escaped"
}

[pscustomobject]@{
    path = $path
    enabled = $Enabled
    entry = $entry
    preserved_utf8_bom = $hasUtf8Bom
    newline = if ($newline -eq "`r`n") { 'CRLF' } else { 'LF' }
    duplicate_entries_removed = [Math]::Max(0, $matches.Count - 1)
}
