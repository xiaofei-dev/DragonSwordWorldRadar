[CmdletBinding()]
param([Parameter(Mandatory)][string]$InstallerExe)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$assembly = [Reflection.Assembly]::LoadFile((Resolve-Path -LiteralPath $InstallerExe).Path)
$engine = $assembly.GetType('DragonSwordNativeAutoPickup.Installer.InstallerEngine110', $true)
$edit = $engine.GetMethod('ApplyConfiguredKeys', [Reflection.BindingFlags]'Static,NonPublic')
$utf8 = [Text.UTF8Encoding]::new($false, $true)
$script:checks = 0
function Check([bool]$value, [string]$label) {
    if (-not $value) { throw $label }
    $script:checks++
}
function EditBytes([byte[]]$bytes, [string]$toggle, [string]$fallback) {
    return ,([byte[]]$edit.Invoke($null, [object[]]@($bytes, $toggle, $fallback)))
}
function Reject([byte[]]$bytes, [string]$toggle, [string]$fallback, [string]$label) {
    $rejected = $false
    try { [void](EditBytes $bytes $toggle $fallback) } catch { $rejected = $true }
    Check $rejected $label
}
$base = "[auto_pickup]`ntoggle_hotkey = F9`ninteraction_key_fallback = F`n"
foreach ($newline in @("`n", "`r`n")) {
    foreach ($bom in @('', [string][char]0xFEFF)) {
        foreach ($trailing in @('', $newline)) {
            $text = $bom + (@('[auto_pickup]', '; keep this comment',
                ' toggle_hotkey  =  F9  ',
                "`tinteraction_key_fallback`t= F`t",
                'interaction_key = E', 'debug_logging = true', 'startup_delay_ms = 1234',
                '# final separate comment') -join $newline) + $trailing
            $bytes = $utf8.GetBytes($text)
            $same = EditBytes $bytes 'F9' 'f'
            Check ([Convert]::ToBase64String($same) -ceq [Convert]::ToBase64String($bytes)) 'Unchanged settings lost exact bytes.'
            $updated = EditBytes $bytes ' insert ' 'Gamepad_FaceButton_Bottom'
            $expected = $text.Replace('=  F9  ', '=  INSERT  ').Replace("= F`t", "= Gamepad_FaceButton_Bottom`t")
            Check ($utf8.GetString($updated) -ceq $expected) 'Editing changed bytes outside the selected values.'
            $single = EditBytes $bytes 'F9' 'HOME'
            Check ($utf8.GetString($single) -ceq $text.Replace("= F`t", "= HOME`t")) 'Single-key edit changed the other key.'
        }
    }
}
foreach ($key in @('INSERT','HOME','PAGEUP','PAGEDOWN','END','DELETE','SPACE') +
        @(1..24 | ForEach-Object { "F$_" }) + @('A','Z','0','9','NUM0','NUM9')) {
    $value = $utf8.GetString((EditBytes ($utf8.GetBytes($base)) $key 'F'))
    Check ($value.Contains("toggle_hotkey = $key`n")) "Unsupported advertised toggle: $key"
}
foreach ($key in @('E', 'HOME', 'Gamepad_FaceButton_Bottom', 'Gamepad_LeftShoulder', 'CustomInteractionKey')) {
    $value = $utf8.GetString((EditBytes ($utf8.GetBytes($base)) 'F9' $key))
    Check ($value.Contains("interaction_key_fallback = $key`n")) "Valid fallback rejected: $key"
}
foreach ($key in @('', ' ', 'F25','CTRL+K','Gamepad_FaceButton_Bottom',"F9`n",('A' * 65))) {
    Reject ($utf8.GetBytes($base)) $key 'F' 'Invalid toggle accepted.'
}
foreach ($key in @('', ' ', 'AUTO', 'auto', 'CTRL+E',"F`n",('A' * 65))) {
    Reject ($utf8.GetBytes($base)) 'F9' $key 'Invalid fallback accepted.'
}
foreach ($text in @('', $base.Replace('[auto_pickup]', '[other]'),
    ($base + "[auto_pickup]`n"), ($base + "toggle_hotkey = HOME`n"),
    ($base + "toggle_hotkey = `n"), ($base + "interaction_key_fallback = HOME`n"),
    $base.Replace('toggle_hotkey = F9', 'toggle_hotkey ='),
    $base.Replace('F9', 'CTRL+K'), $base.Replace("`n", "`r"),
    ($base + [char]0), ($base + (';' * 65536)),
    ($base + "[other]`ntoggle_hotkey=HOME`n"),
    $base.Replace('F9', 'F9 ; inline comment is not a native key'))) {
    Reject ($utf8.GetBytes($text)) 'INSERT' 'HOME' 'Malformed config accepted.'
}
Reject ([byte[]](0xFF, 0xFE, 0x00)) 'INSERT' 'HOME' 'Invalid UTF-8 accepted.'
Check ($utf8.GetString((EditBytes ($utf8.GetBytes($base.Replace('F9','f9'))) 'F9' 'F')) -ceq $base) 'Lowercase toggle was not canonicalized for the native parser.'
Write-Host "PICKUP_HOTKEY_CONFIGURATION=$script:checks/$script:checks"
