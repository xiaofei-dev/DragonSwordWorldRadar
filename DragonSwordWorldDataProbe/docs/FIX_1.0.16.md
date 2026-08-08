# 1.0.16

The 1.0.15 diagnostic exposed two implementation bugs:

1. `find_map()` referenced `access_probe` before the local function entered lexical scope.
   Lua therefore resolved it as a global and raised:
   `attempt to call a nil value (global 'access_probe')`.

2. Windows PowerShell 5.1 rejected an inline `if` used as a command argument:
   `-NextAction ( if(...) )`.
   The branch is now assigned to `$nextAction` before `Write-ModuleResult`.

No research route was changed. Both the runtime table route and the bounded
Kind/Place fallback are retained.
