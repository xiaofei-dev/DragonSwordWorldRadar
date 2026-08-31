# Release Procedure

## Public release

- Version: `1.3.0`
- Runtime marker: `DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0`
- Supported ABI: UE4SS v3.0.1 Beta #0 commit `1c1a1497`, ExperimentalNested only
- Launch state: Off
- Main-menu/save/World initialization state: forced Off; toggle required again
- Default toggle: F9
- Primary interaction mode: saved semantic `INTERACT` binding
- Fallback interaction key: F
- Public Debug: Off
- Installer actions: Install, Upgrade, Repair, Uninstall
- Optional range: Original, 3x, 5x, 10x, 15x, or 20x; Original by default
- StableRoot payload: not included
- Game executable hash: diagnostic only, never a release gate
- Selector resolution policy:
  `runtime_reflection_dual_caller_rel32_consensus_fail_closed`
- Selector compatibility: loaded PE32+ plus x64 `.pdata`/`CHAININFO` bounds;
  reflected `Server_RunInteractV2` uses its unique virtual slot and CDO native
  implementation, while the complete reflected `SetInteractUIV2` exec wrapper
  uses one terminal `E8 rel32` native-implementation call; both implementations
  must resolve the same selector address; no fixed RVA, hash table, or fallback
  address
- Action lifecycle: one global pending action keyed to the exact returned
  Component; exact Actor/Component invalidation or exact Component inactive
  confirms; the first 650 ms timeout permits one selector-represented retry
  after 100 ms; the second timeout quarantines that exact Component
- Interaction-owner lifecycle: replacement clears pending and attempt records
  and enforces a new 1500 ms settle deadline before the next scan
- Toggle lifecycle: one transition per physical press; release required before
  another F9 transition
- Feature status: corrective and compatibility-repair source implemented after
  deployed 1.2.0 action-storm evidence
- Exact-package status: fresh static, core, built-artifact, installer 10/10,
  deterministic ZIP, exact-entry, and checksum gates passed for the latest
  exact-Component preflight and strict owner-settle source; selector in-process
  resolution, deployment, gameplay acceptance, and owner smoke testing remain
  pending

## Release artifacts

The canonical 1.3.0 release artifacts are under
`dist/releases/1.3.0` and contain exactly four public archives:

1. One-click installer ZIP with README and checksums.
2. Manual Mod-only ZIP without UE4SS or range PAKs.
3. Manual direct-paste ZIP with the exact compatible UE4SS runtime and no range
   PAKs.
4. Standalone range PAK ZIP with 3x, 5x, 10x, 15x, and 20x choices, its README,
   and checksums.

Do not publish a separate standalone UE4SS runtime archive as part of this Auto
Pickup release.

The pinned UE4SS compatibility runtime remains a build input only. The five
approved range PAKs remain owned by the range-expansion project. Each now
contains 50 reviewed gather/animal packages and 19 structured type-7 monster-
drop packages; treasure assets are excluded.

The current action-lifecycle artifact identities with the 750 ms confirmation
window and 200 ms retry cooldown are:

- native DLL: 919,552 bytes, SHA-256
  `10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
- unsigned Setup: 13,001,728 bytes, SHA-256
  `2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`;
- installer ZIP: 8,556,743 bytes, SHA-256
  `70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`;
- manual without UE4SS ZIP: 362,076 bytes, SHA-256
  `9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`;
- manual with UE4SS ZIP: 8,446,667 bytes, SHA-256
  `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`;
- standalone range ZIP: 3,919,600 bytes, SHA-256
  `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.

Static, source, core, built-artifact, installer 10/10, deterministic ZIP,
exact-entry, and checksum gates passed. These are offline release results only.

## Canonical command

Run the 1.3.0 release gate from the project root:

```powershell
& .\tools\Build-Release.ps1 `
  -InstallerTestGameExecutable '<path-to-DSClient-Win64-Shipping.exe>'
```

`Build-Release.ps1` is the canonical ExperimentalNested-only builder. It runs
the full source and core gate, builds and verifies the native DLL and Setup,
requires the isolated installer matrix to return exactly 10 passed and zero
failed or skipped fixtures, rebuilds each ZIP twice, and checks exact entries,
complete checksums, first-party ASCII English release text, and payload
exclusions before replacing the release directory. Pinned upstream text under
`ue4ss/Mods/shared/**` and `ue4ss/licenses/**` is exempt from the ASCII rule and
must remain byte-for-byte identical to the approved runtime archive.
`Build-Release110.ps1` is retained only as a compatibility wrapper.

`-SkipNativeBuild` may reuse an existing DLL, but the DLL still passes exact
artifact verification. It does not skip source/core, installer, archive, or
checksum gates.

Generated native, installer, test, and staging state lives only under `out/`.
If a prior Setup executable is still open, close it before rebuilding; do not
create alternate build roots. The release manifest records the exact Setup
embedded in the installer ZIP.

## Required gates

1. Build the ExperimentalNested native DLL and verify its 1.3.0 marker and ABI.
2. Verify launch-Off, forced-Off lifecycle behavior, true physical F9 edge,
   one global pending action, exact invalidation/state confirmation, one
   selector-represented bounded retry, second-timeout quarantine, and
   interaction-owner context reset in source and tests.
3. Verify the selected game filename rule and diagnostic-only game hash path.
   The hash must never select, authorize, or fall back to a selector address.
4. Verify the PE32+ and x64 runtime-function fixtures, real instruction-boundary
   decoding contract, Server virtual-slot/CDO resolution, complete UI exec-
   wrapper bounds, unique terminal `E8 rel32` UI implementation call,
   implementation-level selector structural contracts, final address consensus,
   no fixed RVA, and fail-closed ambiguity/disagreement/fault paths.
5. Verify Install, Upgrade, Repair, and Uninstall state inspection against isolated
   absent, owned, and unknown same-name fixtures; the current release matrix
   must return exactly 10 passed, 0 failed, and 0 skipped.
6. Confirm exact-runtime Upgrade preserves `config.ini`, does not create a
   persistent conversion backup, and restores temporary transactional changes
   on failure.
7. Confirm Uninstall removes only owned Auto Pickup files, the authoritative
   `mods.txt` entry, and owned approved range PAKs while preserving UE4SS and
   unrelated Mods.
8. Confirm runtime conversion requires consent, creates a complete verified
   backup, removes the old active layout, migrates user Mods/settings, and uses
   `_2`/`_3` collision suffixes.
9. Verify Original, 3x, 5x, 10x, 15x, and 20x selection, mutual exclusion, and
   filename-owned replacement of prior exact-name PAKs.
10. Inspect all four archives and confirm that no StableRoot payload, standalone
   UE4SS runtime archive, unrelated third-party Mod, debug log, or non-English
   first-party text is included. Preserve approved upstream text under
   `ue4ss/Mods/shared/**` and `ue4ss/licenses/**` byte-for-byte; those pinned
   third-party paths are exempt from the first-party ASCII rule.
11. Record archive sizes and SHA-256 values only after the final build.
12. Keep selector runtime resolution, in-game behavior, and performance
    acceptance separate from static,
    compile, installer, and package evidence.

The canonical compatibility-repair build completed and the final artifact sizes
and SHA-256 values are recorded above and in `docs/EVIDENCE.md`. A successful
offline release gate does not replace deployment, in-process selector
resolution, or an exact-package gameplay smoke test,
including reference-build Server virtual-path/UI direct-wrapper
`SELECTOR_RESOLVED` consensus, fail-closed Off behavior for an incompatible
contract, held-F9 repeat, pending blocking, exact-Component timeout
quarantine, continued handling of unrelated candidates, manual F,
on-foot/mounted pickup, travel, clean exit, and performance.
