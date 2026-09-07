[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$InstallerExe)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$assembly = [Reflection.Assembly]::LoadFrom((Resolve-Path -LiteralPath $InstallerExe).ProviderPath)
$engine = $assembly.GetType('DragonSwordNativeWorldRadarPostRender.Installer.InstallerEngine', $true, $false)
$parse = $engine.GetMethod('ParseHotkeyName', [Reflection.BindingFlags]'Static, NonPublic')
$validate = $engine.GetMethod('ValidateHotkeyConfig', [Reflection.BindingFlags]'Static, NonPublic')
$script:assertions = 0
function Key([string]$Name, [int]$Expected) {
    $actual = $parse.Invoke($null, [object[]]@($Name))
    if ($actual -ne $Expected) { throw "Hotkey code mismatch: $Name expected $Expected got $actual" }
    $script:assertions++
}
function Config([string]$Text, [bool]$Accept, [bool]$Public = $false) {
    $accepted = $true
    # Reflection's void result must not add null entries to Test-Installer's
    # success stream; Build-Release expects exactly one matrix summary object.
    try { [void]$validate.Invoke($null, [object[]]@([Text.Encoding]::UTF8.GetBytes($Text), $Public)) }
    catch { $accepted = $false }
    if ($accepted -ne $Accept) { throw 'Hotkey config acceptance mismatch.' }
    $script:assertions++
}
1..24 | ForEach-Object { Key "F$_" (0x6F + $_); Key "f$_" (0x6F + $_) }
65..90 | ForEach-Object { Key ([string][char]$_) $_; Key ([string][char]($_ + 32)) $_ }
0..9 | ForEach-Object { Key ([string]$_) (0x30 + $_); Key "NUM$_" (0x60 + $_) }
$names = @('HOME','END','PAGEUP','PAGEDOWN','INSERT','DELETE','SPACE')
$codes = @(0x24,0x23,0x21,0x22,0x2D,0x2E,0x20)
for ($i = 0; $i -lt $names.Count; $i++) { Key $names[$i] $codes[$i]; Key $names[$i].ToLowerInvariant() $codes[$i] }
@('', 'F0', 'F25', 'F01', 'NUM10', 'Ctrl+F6', 'PGUP', 'SHIFT', 'ESC', 'Mouse1') | ForEach-Object { Key $_ -1 }
$default = "[hotkeys]`nsettings_hotkey=F6`nenable_hotkey=F7`ndisable_hotkey=F8`n"
$custom = "[hotkeys]`nsettings_hotkey=Insert`nenable_hotkey=Home`ndisable_hotkey=PageUp`n"
Config $default $true $true
Config $custom $true
Config $custom $false $true
Config ([string][char]0xFEFF + $custom.Replace("`n", "`r`n")) $true
Config ($default + '#' + ('x' * (4095 - $default.Length))) $true
foreach ($bad in @('', '[hotkeys]', $default.Replace('F8', 'f7'), $default.Replace('F8', 'F6'),
        $default.Replace('F7', 'f6'), $default.Replace('F8', 'F25'),
        $default.Replace("disable_hotkey=F8`n", ''), $default.Replace('settings_hotkey', 'unknown'),
        $default.Replace('[hotkeys]', '[other]'), $default.Replace('[hotkeys]', ''),
        ($default + "[hotkeys]`n"), ($default + 'settings_hotkey=F6'),
        ($default + [char]0), $default.Replace("`n", "`r"), ('#' + ('x' * 4096)))) {
    Config $bad $false
}
$normalize = $engine.GetMethod('NormalizeRequestedHotkeys', [Reflection.BindingFlags]'Static, NonPublic')
$apply = $engine.GetMethod('ApplyRequestedHotkeys', [Reflection.BindingFlags]'Static, NonPublic')
$read = $engine.GetMethod('ReadHotkeys', [Reflection.BindingFlags]'Static, NonPublic')
function Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }; $script:assertions++
}
function ApplyText([string]$Text, [string[]]$Names) {
    $selection = $normalize.Invoke($null, [object[]]$Names)
    $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
    $result = $apply.Invoke($null, [object[]]@($bytes, $selection))
    return [Text.Encoding]::UTF8.GetString([byte[]]$result)
}
$decorated = [string][char]0xFEFF + "# keep`r`n[hotkeys]`r`n settings_hotkey = Insert  `r`nenable_hotkey=home`r`ndisable_hotkey=PageUp`r`n"
Check ((ApplyText $decorated @('INSERT','HOME','PAGEUP')) -ceq $decorated) 'Unchanged keys lost exact original bytes.'
$expected = $decorated.Replace('= Insert  ', '= F6  ').Replace('=home', '=F7').Replace('=PageUp', '=F8')
Check ((ApplyText $decorated @('F6','F7','F8')) -ceq $expected) 'Edited keys lost BOM, comments, spacing or CRLF.'
Check ((ApplyText $custom @('F6','HOME','PAGEUP')) -ceq $custom.Replace('=Insert','=F6')) 'Editing one key changed another key or LF layout.'
$noFinalNewline = "[hotkeys]`nsettings_hotkey=INSERT`nenable_hotkey=HOME`ndisable_hotkey=PAGEUP"
Check ((ApplyText $default.TrimEnd("`n") @('INSERT','HOME','PAGEUP')) -ceq $noFinalNewline) 'No-final-newline update failed.'
foreach ($invalid in @(@('HOME','home','F8'),@('','F7','F8'),@('CTRL+F6','F7','F8'),@('F25','F7','F8'),@('F6',('x'*33),'F8'),@("F6`n",'F7','F8'),@("F6`r",'F7','F8'))) {
    $rejected = $false
    try { [void]$normalize.Invoke($null, [object[]]$invalid) } catch { $rejected = $true }
    Check $rejected 'Invalid or duplicate requested hotkeys were accepted.'
}
$largeSpacing = $default.Replace('settings_hotkey=F6', 'settings_hotkey=' + (' '*100) + 'F6')
Check ((ApplyText $largeSpacing @('F6','F7','F8')) -ceq $largeSpacing) 'Previously valid spacing stopped being readable.'
$limitText = $default + '#' + ('x' * (4095 - $default.Length))
$overflowRejected = $false
try { [void](ApplyText $limitText @('INSERT','HOME','PAGEUP')) } catch { $overflowRejected = $true }
Check $overflowRejected 'An edited hotkey file exceeding 4 KiB was accepted.'
Write-Host "Hotkey installer parser/editor: $script:assertions assertions passed; no fixture or game writes."
