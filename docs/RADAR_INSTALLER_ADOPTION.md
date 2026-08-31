# Applying the Shared Installer Flow to Legacy DragonSwordWorldRadar

## Scope distinction

This document applies only to the legacy external Overlay project at
`DragonSwordWorldRadar`. It does not describe the native in-game UMG product at
`DragonSwordNativeWorldRadarPostRender`.

The native product owns its implemented 1.0.0 release pipeline, Setup installer,
ExperimentalNested manual package, embedded runtime payload, exact-tree upgrade
ownership checks, and R7 acceptance boundaries. Use that project's `README.md`,
`docs/INSTALL.md`, `docs/MANUAL_INSTALL.md`, `docs/RELEASE.md`, and
`metadata/installer-product-profile.json` as its authoritative deployment
documents. Do not route native Radar release work through the legacy Overlay
generation/watcher pipeline below.

## Current Radar boundary

`DragonSwordWorldRadar` already has a substantial installer and release system.
It is not a loose three-file Mod and must not be reduced to the AutoPickup
payload model.

Current product-owned behavior includes:

- `build/Build-Release.ps1`: source gates, compile/refactor/catalog tests,
  source-to-release mapping, manifest creation, ZIP generation, and independent
  archive audit;
- `src/installer/Install.ps1`: configuration migration, exact overlay and
  installer source compilation, data generation, install-state validation,
  `mods.txt` handling, watcher setup, and legacy cleanup;
- `src/installer/Core/InstallationPipeline.cs`: current-game PAK extraction and
  install-generated Treasure, Boss, Assault, Mole/Fly, and owner-pointer data;
- preservation of user-owned `scripts/config.lua` and treasure overrides;
- pre/post executable and PAK identity locking so a game update cannot mix data
  from two builds;
- one bundled `ooz.exe` and SQLCipher runtime dependency with product-specific
  notices and package gates.

The future one-click installer should wrap and harden this pipeline, not replace
its dynamic generation with AutoPickup constants.

Radar's current documentation and deployment tooling target only the owner's
ExperimentalNested layout. StableRoot support is a new compatibility target,
not an existing Radar capability.

## Reuse matrix

| Shared element | Reuse for Radar | Radar-specific work |
|---|---|---|
| Exact executable picker and game-root derivation | Reuse directly | Also discover and lock the authoritative game PAK/Oodle inputs |
| StableRoot/ExperimentalNested loader and proxy detection | Reuse after filling Radar fingerprints | Prove the Radar Lua/host payload on each supported layout; dual native DLLs are required only if Radar ships ABI-specific native code |
| Official stable UE4SS bootstrap | Reuse | Verify every Radar runtime dependency and watcher path after bootstrap |
| Transaction, backups, atomic writes, rollback, reparse checks | Reuse as a shared shell | Expand the mutation inventory to generated data, config migration, host/Overlay files, shortcuts, install state, and legacy cleanup |
| `mods.txt` normalization | Reuse the byte-preserving implementation | Decide how to migrate Radar's current packaged `enabled.txt` and its existing policy of preserving an explicit `mods.txt` value |
| Optional PAK checkbox | Reuse only if Radar later owns an optional PAK | Do not bundle AutoPickup's 5x PAK or another Mod's assets |
| Installer WinForms UI and embedded payload manifest | Reuse structure | Replace hotkey/PAK controls with Radar options and generation status |
| Isolated installation matrix | Reuse harness design | Add data generation, config/override migration, watcher, compiled Overlay, and update-drift cases |
| Installer-first release ZIP and manifest | Reuse | Decide whether to keep a separate source/manual-install archive for advanced users |
| Gameplay evidence states | Reuse exactly | Radar owns F7/F8, compact/expanded map, transition, save, clock, and performance acceptance |

## Decisions required before implementation

1. **Load authority.** Radar currently packages `enabled.txt` and preserves an
   existing `mods.txt` value. Choose whether public 1.0.0-style installation
   migrates to authoritative `mods.txt` or formally retains the current dual
   behavior. Do not copy AutoPickup's deletion rule without this decision.
2. **Supported UE4SS layouts.** Decide whether Radar will support both layouts
   publicly. Static path adaptation is not proof that the Lua callback behavior,
   host process, Overlay startup, or teardown is compatible.
3. **Distribution channels.** Decide whether users receive only Setup or also a
   manual/source package. The current Radar ZIP is a self-installing source
   package, not an installer-first archive.
4. **Public diagnostics.** The current acceptance build defaults Debug On.
   Choose the public default only after final performance and failure-attribution
   testing.
5. **Game compatibility policy.** Keep install-time generation for a supported
   changed game build only when every extractor and output-shape gate passes;
   otherwise fail closed.
6. **Redistribution and source obligations.** The current notice explicitly
   says that `ooz.exe` provenance and redistribution rights are unresolved.
   Resolve that before a public installer embeds it. Radar is GPL-3.0 and ships
   compilable source, so the installer/archive must retain the corresponding
   source, GPL text, Apache/SQLCipher notices, and every bundled-tool notice.
7. **Long-running UI work.** PAK extraction, C# compilation, and catalog
   generation can take materially longer than AutoPickup's file copy. Run that
   work through a controlled worker or subprocess with progress reporting;
   never freeze the WinForms UI or move UE object/runtime work into it.

## Recommended migration phases

### Phase 1: freeze the existing product profile

Copy the shared JSON template into Radar metadata and populate the current
version, runtime files, user-owned files, generated outputs, dependencies,
loader layouts, load control, and gate commands. Do not change installation
behavior in this phase.

### Phase 2: separate preparation from mutation

Refactor the existing installer into:

- read-only discovery and validation;
- deterministic generation into an installer-owned temporary/staging tree;
- a complete mutation plan;
- transactional application and post-write verification.

The expensive PAK extraction, source compilation, and data validation should
finish before the first game-directory mutation whenever possible.

Add a preparation mode to Radar's existing installer that accepts the selected
game path explicitly, writes only to an installer-owned staging tree, does not
edit `mods.txt`, and does not start the watcher. The outer transaction should
then own the Mod-folder commit, external files, load control, watcher/shortcut
activation, verification, and rollback. The current `Install.ps1` mutation
sequence is not a complete transaction journal.

### Phase 3: add the one-click wrapper

Adapt the AutoPickup WinForms shell to select the exact executable, show the
detected UE4SS layout, expose Radar-specific choices, and invoke the prepared
Radar installation plan. Embed or stage only allowlisted Radar payloads and
notices.

Use the existing verified Radar release staging as the single canonical
embedded payload. Do not reproduce its source-to-release mapping inside the GUI
installer, because two independent mappings will drift.

### Phase 4: expand the isolated matrix

In addition to the shared layout/security tests, cover:

- fresh install and upgrade from the current Radar package;
- valid and malformed `scripts/config.lua`;
- preservation of `show_assaults`, debug choice, and treasure overrides;
- missing/corrupt `ooz.exe`, SQLCipher, executable, PAK, or generated catalog;
- executable/PAK identity changing before and after generation;
- exact 9 Boss, 40 Assault, and current 83-record generated mini-game catalog
  gate, including its Fly/Mole/Wave subcontracts;
- Overlay and installer source-set compilation with all `obj/bin` paths excluded;
- watcher/shortcut installation and rollback;
- injected late failure after generated data and load-control changes;
- no logs, runtime bridge data, diagnostics, databases, or local backups in the
  public archive.

### Phase 5: run exact-artifact acceptance

After static and installer gates pass, deploy one exact candidate and verify:

- clean cold start, F7 activation gate, and complete F8 shutdown;
- compact and expanded maps, repeated open/close, dragging, and resize;
- treasures, Bosses, Assaults, Mole/Fly, completion filtering, and clock;
- menus, cutscenes, dungeon/teleport/world transitions, and return to open world;
- config/override preservation and game-update reinstallation behavior;
- same-session F8/F7 performance comparison and long-session accumulation;
- clean game exit with no UE4SS, host, Overlay, or installer exception.

Only owner-observed results for the exact installed hashes can promote the
candidate to gameplay accepted.

## Suggested implementation starting points

Copy structure and tests, not product constants:

- UI/resource embedding: `DragonSwordNativeAutoPickup/installer/*`
- layout/transaction model: `DragonSwordNativeAutoPickup/installer/InstallerEngine.cs`
- isolated fixtures: `DragonSwordNativeAutoPickup/installer/tests/InstallerIntegration.Tests.ps1`
- release hard gate: `DragonSwordNativeAutoPickup/tools/Build-Release.ps1`
- archived Radar generation: `Archive/DragonSwordWorldRadar/src/installer/Core/*`
- archived Radar install orchestration: `Archive/DragonSwordWorldRadar/src/installer/Install.ps1`
- archived Radar package audit: `Archive/DragonSwordWorldRadar/build/Test-ReleasePackage.ps1`

The target is one user-facing installer with Radar's existing generation and
runtime contracts behind it, not a second independent installer that can drift.
