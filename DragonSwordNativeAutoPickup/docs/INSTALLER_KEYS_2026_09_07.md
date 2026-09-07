# Unified Setup keys - installer-only receipt

Status: isolated installer validation passed; elevated click-through and owner
gameplay acceptance remain separate. No real game deployment or external
publication was performed for this attempt.

## Scope and cause

Pickup's UI collected custom keys, but Inspect and InstallCore overwrote both
with installed old keys during Update/Repair. The old config was then copied
unchanged. Both overrides are removed. Owned configuration now receives only
the exact confirmed key-value edits inside the existing transaction.

Both Setup UIs load installed defaults, permit editing on Install/Update/Repair,
show an explicit default-No confirmation, execute normalized plan values,
retain edits on cancel/error, and reload when the game path changes. Pickup
keeps its concrete interaction fallback and range choices; Radar keeps its
three-distinct-key rule and separate runtime/ABI policy.

## Deliverables

Paths are relative to the DragonSword workspace:

- Pickup: `DragonSwordNativeAutoPickup/out/installer/unified-keys-20260907/DragonSwordNativeAutoPickup-Setup-1.3.1.exe`
  - SHA-256: `D7D1491E4396862A32CE8ED2D759BBD3617FD99585D0B88E8B3D737676BDB676`
- Radar: `DragonSwordNativeWorldRadarPostRender/dist/work/build/installer/DragonSwordNativeWorldRadarPostRender-Setup-2.3.0.exe`
  - SHA-256: `6AB72C206E52AFACF8A430C34E20E19B30D35BC0B74CE6DE3428B488A3D319C4`
  - This is the existing verified hotkey-capable Radar EXE; its production
    source/binary did not need another change during this Pickup alignment.

Pickup baseline Setup hash:
`8B3B883EFB8BF1E269643D98A0B0E5270682E17157FF92321D8110335BAF4B8D`.
All 11 embedded resources are byte-identical between baseline and new EXE,
including DLL, config, Lua, runtime ZIP, range PAKs, notices, and payload manifest.
Native DLL remains
`44FFCECD0CCC4CB1BA30502F147E1E439F919D229A4B9DD5D72B66C1155D1B64`.
Previous release ZIPs and their manifests were not replaced. The canonical
full-release build and its mandatory fresh-native-build gate were not bypassed;
this delivery uses the separate installer builder against unchanged payloads.

## Validation

- Pickup `Build-Installer.ps1`: passed without a native rebuild.
- Pickup `HotkeyConfiguration.Tests.ps1`: 95/95 assertions.
- Pickup `Test-Installer110.ps1`: 11/11 cases, zero failures/skips, fixtures cleaned.
  - Includes custom fresh Install, older-version Update, current-version Repair,
    byte-exact no-change config, unrelated settings/mods/runtime preservation,
    invalid and stale selections/config, and actual form state without showing
    a window (same-path edit retention and changed-path defaults).
  - Injected failure after config/range/install-record writes restored original
    config, mod tree, range PAK, mods.txt, and loader.
- Radar `Test-Installer.ps1`: 177 parser/editor assertions and 20/20 integration
  cases, zero failures/skips, fixture cleanup and unchanged-source checks passed.
- Resource identities compared directly from both Pickup executable assemblies.

The first Pickup run passed 9/11 because two expected-value strings assumed
spaces around `=` while fresh config has none. The editor correctly preserved
formatting. Corrected fixture expectations and reran all cases successfully.
Native parser review also prevented an accidental compatibility expansion:
inline key comments, duplicate assignments across sections, and AUTO fallback
are rejected. Separate comments, existing BOM/newline formatting, and unrelated
settings are preserved; this is not a full native configuration validator.

No elevated Setup dialog was automatically accepted and no game process was
launched or closed. Both EXEs remain unsigned.
