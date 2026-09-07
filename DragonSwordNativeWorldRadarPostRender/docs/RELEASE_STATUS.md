# Native World Radar 2.3.0 - Current Local Release

Date: 2026-09-07. Scope: complete three-package release and documentation
closeout. 2.2.2 was never published; its follow-up work is included in 2.3.0.

## Deliverables

Canonical directory: `dist/final-2.3.0/`.

- `DragonSwordNativeWorldRadarPostRender-v2.3.0-Installer.zip`
- `DragonSwordNativeWorldRadarPostRender-v2.3.0-Manual-No-UE4SS.zip`
- `DragonSwordNativeWorldRadarPostRender-v2.3.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`
- Identity authority: `release-manifest.json` and `SHA256SUMS.txt`.

Choose one package. Setup is recommended for existing installations and
configuration changes. Install / Update / Repair load existing keys and apply
explicitly confirmed changes while preserving other user settings.

## Features and evidence boundary

- Configurable Settings / Enable / Disable keys; defaults F6/F7/F8.
- AUTO is first, stays AUTO, and refreshes on Settings open or Enable.
  Manual language choices remain fixed until AUTO is selected again.
- Mounted-flight map attachment and controller-menu hiding corrections.
- Retains the CM-04 native DLL; documentation repackaging does not add native fixes.
- Public diagnostics off, language AUTO, default hotkeys.
- Source-bound native receipt, core tests, compact/world-map/runtime source
  checks, and release hygiene passed under Windows PowerShell 5.1.
- Hotkey parser/editor: 177 assertions; Setup matrix: 20/20; manual-copy matrix:
  2/2. No failed or skipped fixtures. Setup/manual payload equivalence passed.
- Independent final ZIP reopen: 3/3 archive hashes and entry counts passed;
  both manual packages contain the exact retained DLL and the three validated
  public configuration files. Packaged Setup matches the manifest.

Native SHA-256: `D6C79578F3C19C033BB0A1F75EDB8382719CFDC87BB145D0D39ECB12FBDB7046`.
Setup SHA-256: `9ED38737B3E0914A784ACDC325706EA8F7D6D9F70099DAD2011B1EF84EA2E0AE`.

| Archive | Bytes | SHA-256 |
| --- | ---: | --- |
| Installer | 10151201 | `A37DD48C631358C5ABD708FCE7E5172DEF492D0E87277AEC58B1B11677941CA6` |
| Manual-No-UE4SS | 2084316 | `2611593B0E2148C59DC2099EA3B419E57C3A03F7BB8E345D2AA0E978F67B1583` |
| Manual-With-UE4SS | 10109953 | `226709595DF0C1D97E66D2637133BC86F6111D3AD70E9D18735DDBFD8D715A0C` |

Prior virtual-controller testing covered two Start / Hero / HeroSkill / return
cycles with diagnostics on. It does not prove all physical controllers, menus,
resolutions, debug-off sessions, city cold activation, or long-session gameplay.
See [2.3.0 validation plan](RELEASE_PLAN_2_3_0.md) and the controller/world-map
attempt ledgers. No universal runtime-fixed claim is made.

## Documentation routing

- Setup guide: [INSTALL.md](INSTALL.md).
- Manual guide: [MANUAL_INSTALL.md](MANUAL_INSTALL.md).
- Public copy: [Nexus index](../assets/nexus/README.md).
- Development history: [RELEASE_PLAN_2_3_0.md](RELEASE_PLAN_2_3_0.md).
- `RELEASE.md` contains historical 2.2.1 contracts, not current build commands.

This is a local handoff, not game deployment, Git push, Nexus upload, or
third-party redistribution clearance. Preserve prior exact-byte receipts;
use the new manifest rather than old ZIP hashes for this package set.

The previous five release files were copied and hash-verified at
`dist/work/candidates/before-doc-closeout-20260907/` before replacement.
Public publication status remains `BLOCKED_PENDING_RIGHTS_AND_SOURCE_PROVENANCE_REVIEW`,
as recorded in the release manifest and `metadata/release.json`. Local package
verification does not resolve that existing blocker.
