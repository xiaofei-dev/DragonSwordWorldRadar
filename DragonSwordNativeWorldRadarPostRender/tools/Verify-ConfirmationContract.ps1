[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$sg10Root = Split-Path -Parent $PSScriptRoot
function Remove-Sg10Comments([string]$Text) {
 $withoutBlocks = [regex]::Replace($Text, '/\*[\s\S]*?\*/', '')
 return [regex]::Replace($withoutBlocks, '(?m)^\s*//[^\r\n]*|(?<=;)\s*//[^\r\n]*', '')
}
function Test-Sg10Confirmation([string]$Hub,[string]$Main,[string]$Model,[string]$EscapeSource) {
 $h = Remove-Sg10Comments $Hub
 $m = Remove-Sg10Comments $Model
 $i = Remove-Sg10Comments $EscapeSource
 $checks = @(
  ($h -match 'global_reset_requested\s*=\s*!close_requested\s*&&\s*confirmed_reset'),
  ($h -match 'confirmed_reset\s*=\s*response.confirmed == ConfirmationAction::RestoreDefaults'),
  ($h -match 'confirmation_\.sample\(\s*confirmation_text_ready_ && yes_checked, no_checked\)'),
  ($h -match 'if \(response.confirmed == ConfirmationAction::Endorse\)\s*return unchanged\(RadarVisibilityHubCommand::OpenEndorsement\)'),
  ($h -match 'if \(response.confirmed == ConfirmationAction::BugReport\)\s*return unchanged\(RadarVisibilityHubCommand::OpenBugReport\)'),
  ($h -match 'if \(!confirmed_reset\) return unchanged\(\);'),
  ($h -match 'cancel_confirmation && confirmation_\.active\(\)[\s\S]*?confirmation_\.clear\(\)[\s\S]*?set_confirmation_visibility_unsafe\(false, true\)[\s\S]*?return unchanged\(\);'),
  ($h -match '\(force_close \|\| cancel_confirmation\) && !current_controller'),
  ($h -match 'confirmation_dismiss_guard_ && !force_close[\s\S]*?set_confirmation_visibility_unsafe\(false\)[\s\S]*?return unchanged\(\);'),
  ($h -match 'block_background = visible \|\| guard_background' -and $h -match 'enable\(control, !block_background\)' -and $h -match 'enable\(yes, visible && confirmation_text_ready_\)'),
  ($h -match 'requested_confirmation = ConfirmationAction::RestoreDefaults' -and $h -match 'requested_confirmation = ConfirmationAction::BugReport' -and $h -match 'requested_confirmation = ConfirmationAction::Endorse'),
  ($m -match 'if \(no\) \{ clear\(\); return \{true, RadarConfirmationAction::None\}; \}'),
  ($m -match 'if \(!armed_\) \{\s*if \(!yes\) armed_ = true;\s*return \{\};'),
  ($m -match 'const auto action = pending_;\s*clear\(\);\s*return \{true, action\};'),
  ($Main.Contains('L"https://www.nexusmods.com/dragonswordawakening/mods/254"') -and $Main.Contains('L"https://www.nexusmods.com/dragonswordawakening/mods/254?tab=posts"')),
  ($Main -match 'destination = endorsement \? kEndorsementUrl : kBugReportUrl' -and $Main -match 'ShellExecuteW\(\s*nullptr, L"open", destination, nullptr, nullptr'),
  ($Main -match 'reason == dswros::EscapeCloseReason::FocusLost\s*\? visibility_hub_\.close\(controller, radar_mod_status\(\)\)\s*: visibility_hub_\.escape\(controller, radar_mod_status\(\)\)'),
  ($i -match 'close_reason_ != EscapeCloseReason::FocusLost' -and $i -match 'panel_open_ \|\| close_requested\(\)\) close_reason_ = EscapeCloseReason::FocusLost')
 )
 for ($index = 0; $index -lt $checks.Count; ++$index) { if (-not $checks[$index]) { Write-Verbose "Confirmation clause failed: $index" } }
 return -not ($checks -contains $false)
}
$sg10Sources = @{
 Hub=Get-Content -LiteralPath (Join-Path $sg10Root 'src/native/radar_visibility_hub.cpp') -Raw
 Main=Get-Content -LiteralPath (Join-Path $sg10Root 'src/native/main.cpp') -Raw
 Model=Get-Content -LiteralPath (Join-Path $sg10Root 'include/dswros/radar_confirmation.hpp') -Raw
 EscapeSource=Get-Content -LiteralPath (Join-Path $sg10Root 'include/dswros/escape_input_model.hpp') -Raw
}
if (-not (Test-Sg10Confirmation @sg10Sources)) { throw 'Confirmation transaction/input/URL source contract failed' }
$sg10Mutants = @(
 @{ File='Hub'; From='!close_requested && confirmed_reset'; To='!close_requested'; Name='unconfirmed preset' },
 @{ File='Hub'; From='confirmation_text_ready_ && yes_checked'; To='yes_checked'; Name='blind Yes with missing text' },
 @{ File='Hub'; From='response.confirmed == ConfirmationAction::Endorse'; To='response.confirmed == ConfirmationAction::None'; Name='cancel opens website' },
 @{ File='Hub'; From='if (!confirmed_reset) return unchanged();'; To='if (!confirmed_reset) return unchanged(RadarVisibilityHubCommand::OpenBugReport);'; Name='No publishes command' },
 @{ File='Hub'; From='visible || guard_background'; To='visible'; Name='closing click passes through' },
 @{ File='Hub'; From='(force_close || cancel_confirmation) && !current_controller'; To='force_close && !current_controller'; Name='modal Esc loses owner fallback' },
 @{ File='Model'; From='if (no)'; To='if (false)'; Name='No loses precedence' },
 @{ File='Model'; From='if (!armed_)'; To='if (false)'; Name='opening click confirms dialog' },
 @{ File='Model'; From='clear(); // Consume'; To='/* retained */ // Consume'; Name='repeat dispatch retains intent' },
 @{ File='Main'; From=': visibility_hub_.escape(controller, radar_mod_status());'; To=': visibility_hub_.close(controller, radar_mod_status());'; Name='Esc closes whole menu' },
 @{ File='Main'; From='nexusmods.com/dragonswordawakening/mods/254'; To='nexusmods.com/dragonswordawakening/mods/253'; Name='wrong mod destination' },
 @{ File='EscapeSource'; From='close_reason_ != EscapeCloseReason::FocusLost'; To='true'; Name='Esc overrides forced focus close' }
)
foreach ($sg10Mutant in $sg10Mutants) {
 $sg10Changed = @{} + $sg10Sources
 if (-not $sg10Changed[$sg10Mutant.File].Contains($sg10Mutant.From)) { throw "Mutation anchor missing: $($sg10Mutant.Name)" }
 $sg10Changed[$sg10Mutant.File] = $sg10Changed[$sg10Mutant.File].Replace($sg10Mutant.From,$sg10Mutant.To)
 if (Test-Sg10Confirmation @sg10Changed) { throw "Confirmation regression accepted: $($sg10Mutant.Name)" }
}
Write-Output "Confirmation transaction contract passed; $($sg10Mutants.Count) regressions rejected."
