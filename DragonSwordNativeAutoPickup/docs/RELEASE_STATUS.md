# AutoPickup 1.3.1 - Current Local Release

Date: 2026-09-07. Scope: complete package refresh with unified Setup key editing
and documentation. No native source changes were introduced by this refresh.

## Deliverables

Canonical directory: `dist/releases/1.3.1/`.

- `DragonSwordAutoPickup-v1.3.1-Installer.zip`
- `DragonSwordAutoPickup-v1.3.1-Manual-No-UE4SS.zip`
- `DragonSwordAutoPickup-v1.3.1-Manual-With-UE4SS.zip`
- `DragonSwordPickupRangeExpansion-v1.3.1.zip`
- Identity authority: `DragonSwordAutoPickup-v1.3.1.release.json`

Use one main package. Range PAKs are optional and independent; choose one
variant only. Install / Update / Repair load existing keys, confirm changes,
and preserve other configuration. The earlier standalone Setup checkpoint at
`out/installer/unified-keys-20260907/` is not the complete release.

## Validation

The canonical full `tools/Build-Release.ps1` run passed under Windows PowerShell
5.1 on 2026-09-07, including a fresh native build (no SkipNativeBuild):

- Source manifest: 115 files; static/source/core/selector/PE fixtures passed.
- Hotkey parser/editor: 95/95 assertions; installer matrix: 11/11, no skips.
- Deterministic archive rebuild, exact entry sets, checksum coverage, English
  first-party text, pinned upstream bytes, and range ownership gates passed.
- Independent final ZIP reopen: 4/4 hashes and entry sets matched the manifest;
  manual DLLs match the new native build and public configuration is unchanged.
- Setup: `89DE89AFC410A9DA3EEDDB9C22B3504520736CECD5DA604D2D1841986984562B`.
- Native: `BFC632CB92A96CA44F4AF949207303E08CB3442FA62DF81D3D58D8789E5F3BE6`.

| Archive | Bytes | SHA-256 |
| --- | ---: | --- |
| Installer | 8575886 | `577D51569DE61A3990423ED4AB73B83D854B7E31707F8F27315AA53105778380` |
| Manual-No-UE4SS | 374026 | `62FD554BAB1B1D6CECAE5D2E9C3DBE2A2F1D82E6F10CE2FE6CDEB84D54A92FF0` |
| Manual-With-UE4SS | 8458547 | `7103D9D76B14F293C825AC998B527DFFFE1BACB647A2093CF22FD11CB73BF854` |
| Range | 3920172 | `A346C4F20CF85C60FD2965FCC583129AFD8EB2B805CF4E20A80C1B20B79B49FE` |

The fresh DLL has a new byte identity; do not substitute the older installed
or installer-only DLL hash for this package. No native source was changed in
this closeout, and the new packages were not deployed to the game.

Public defaults: Off, F9 toggle, AUTO interaction binding, F fallback,
debug off, Original range in Setup. Native dispatch-observer and
startup-readiness corrections are part of the existing 1.3.1 source, not new
gameplay work in this packaging task. Exact-package deployment and gameplay
acceptance remain RUNTIME_PENDING; offline checks cannot replace them.

## Documentation routing

- User installation: [INSTALL.md](INSTALL.md).
- Build contract and dated receipts: [RELEASE.md](RELEASE.md).
- Earlier installer-only receipt: [INSTALLER_KEYS_2026_09_07.md](INSTALLER_KEYS_2026_09_07.md).
- Nexus publishing copy: local-only `assets/nexus/`, excluded from GitHub.
- Development attempts and runtime evidence remain in their original ledgers.

The preceding five release files, Setup, and native DLL were hash-verified and
preserved in `out/release-backups/before-full-closeout-20260907/`.
No game installation, Git push, Nexus upload, or publication-rights clearance
is performed by this local closeout.
