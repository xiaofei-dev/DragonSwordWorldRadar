# Shared DragonSword Mod Engineering Docs

This directory owns repository-wide installation, packaging, and acceptance
standards. Product directories still own their runtime design, versioned
metadata, exact hashes, and gameplay evidence.

## Documents

| Document | Purpose |
|---|---|
| `DRAGONSWORD_MOD_INSTALLER_STANDARD.md` | Reusable design for a one-click Windows installer, UE4SS layout routing, transactional writes, configurable-input fallbacks, optional PAKs, manual-channel parity, and load control |
| `DRAGONSWORD_MOD_RELEASE_STANDARD.md` | Common build, dual installer/manual packaging, evidence, deployment, and publication gates |
| `INTEGRATION_STATUS.md` | Current product layout, owner acceptance, cross-Mod runtime observations, and public-source boundary |
| `RADAR_INSTALLER_ADOPTION.md` | Legacy external `DragonSwordWorldRadar` adoption plan and explicit separation from the native PostRender Radar 1.0 release pipeline |
| `templates/INSTALLER_PRODUCT_PROFILE.template.json` | Copyable product-input contract for a new installer |
| `templates/RELEASE_ACCEPTANCE_CHECKLIST.md` | Copyable static, installation, deployment, and gameplay checklist |

## Reference implementation

`DragonSwordNativeAutoPickup` 1.3.0 is the current reference implementation of
this standard. Its release pipeline demonstrates exact executable selection,
StableRoot and ExperimentalNested UE4SS routing, official UE4SS bootstrap,
authoritative `mods.txt` control, semantic interaction-key discovery with an
installer-selected fallback, independent 3x/5x/10x PAK choices, rollback,
separate installer/manual archives, and an offline 20-case integration matrix.

The implementation is a reference, not a generic binary library. Never copy
its product name, hashes, native DLLs, game fingerprint, PAK, version, or
acceptance claims into another Mod. Start with the product-profile template and
replace every product-owned value.

Useful reference files:

- `DragonSwordNativeAutoPickup/installer/InstallerEngine.cs`
- `DragonSwordNativeAutoPickup/installer/InstallerForm.cs`
- `DragonSwordNativeAutoPickup/tools/Build-Installer.ps1`
- `DragonSwordNativeAutoPickup/tools/Test-Installer.ps1`
- `DragonSwordNativeAutoPickup/installer/tests/InstallerIntegration.Tests.ps1`
- `DragonSwordNativeAutoPickup/tools/Build-Release.ps1`

## Evidence boundary

Keep these states separate in every project:

1. source validated;
2. compiled;
3. packaged;
4. installer-tested in isolated filesystem fixtures;
5. deployed with exact installed hashes;
6. owner-observed gameplay accepted.

A later state may cite the earlier states, but no earlier state implies a later
one. In particular, successful installation does not prove visible gameplay
behavior, frame-time cost, travel safety, or clean process teardown.
