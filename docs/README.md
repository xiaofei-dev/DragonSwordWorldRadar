# Shared DragonSword Mod Engineering Docs

This directory owns repository-wide installation, packaging, and acceptance
standards. Product directories still own their runtime design, versioned
metadata, exact hashes, and gameplay evidence.

## Documents

| Document | Purpose |
|---|---|
| `GITHUB_CLOSEOUT_2026_09_10.md` | Current suite handoff: Radar 3.0, unreleased AutoPickup candidate, unchanged products, validation and source-publication boundary |
| `PUBLIC_SOURCE_POLICY.md` | GitHub source scope and local-only Nexus materials, logs, packages and extracted data |
| `DRAGONSWORD_MOD_INSTALLER_STANDARD.md` | Reusable design for a one-click Windows installer, UE4SS layout routing, transactional writes, configurable-input fallbacks, optional PAKs, manual-channel parity, and load control |
| `DRAGONSWORD_MOD_RELEASE_STANDARD.md` | Common build, dual installer/manual packaging, evidence, deployment, and publication gates |
| `INTEGRATION_STATUS.md` | Current product layout, owner acceptance, cross-Mod runtime observations, and public-source boundary |
| `RADAR_INSTALLER_ADOPTION.md` | Legacy external `DragonSwordWorldRadar` adoption plan and explicit separation from the native PostRender Radar 1.0 release pipeline |
| `templates/INSTALLER_PRODUCT_PROFILE.template.json` | Copyable product-input contract for a new installer |
| `templates/RELEASE_ACCEPTANCE_CHECKLIST.md` | Copyable static, installation, deployment, and gameplay checklist |

## Reference implementation

`DragonSwordNativeAutoPickup` 1.3.1 is the published reference implementation of
this standard; its current working source also contains an unreleased candidate.
Its release pipeline demonstrates exact executable selection,
StableRoot and ExperimentalNested UE4SS routing, official UE4SS bootstrap,
authoritative `mods.txt` control, semantic interaction-key discovery with an
installer-selected fallback, independent 3x/5x/10x/15x/20x PAK choices, rollback,
and separate installer/manual archives. Use its dated release status for the
exact installer matrix and artifact identities.

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
