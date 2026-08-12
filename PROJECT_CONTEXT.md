# DragonSword Multi-Mod Repository Context

## Repository purpose

This is a multi-project repository for the production DragonSword Awakening Radar, owner-authorized native Mod canaries, and read-only research tooling. The Git history was inherited from the original standalone DragonSwordWorldRadar repository and promoted to this directory so related publishable projects can be managed together.

Use this file as the first cross-chat orientation document. Then read the selected project's own `PROJECT_CONTEXT.md` and current metadata before acting.

## Ownership map

### `DragonSwordWorldRadar/`

Primary production Mod and authoritative owner of the integrated treasure, Boss, Assault, and Mole/Fly radar. Current source is `0.4.0-dev74-minigamecatalog2` with one protocol-v6 Motion/control bridge. Development is feature-complete for the current cycle; preserve the acceptance boundary between passed Windows/package gates and owner gameplay validation.

### `DragonSwordNativeAutoPickup/`

Native C++ UE4SS research Mod for bounded ordinary ground-loot pickup. The current `0.6.0-dropitem-closed-loop-diagnostic` is deliberately read-only: it discovers one non-template current-World `DropItemActor`, locks the exact owner/component weak identities, and traces only manual interaction calls correlated to that same object. The rejected 0.5.2-0.5.4 Vitality target/action assumptions are not used. Compilation and packaging do not establish gameplay acceptance.

### `DragonSwordWorldDataProbe/`

Research and diagnostics framework. It may contain active, disabled, manual-only, experimental, prohibited, and archived methods in the same tree. File presence never proves that a module is active or accepted. Keep game access bounded and read-only, preserve raw evidence, and distinguish confirmed observations from inference.

### `DragonSwordNativeWorldRadar/`

Isolated medium-architecture proof of concept. Native C++ owns bounded player-coordinate publication while its external Overlay reuses the Radar rendering model. It is not the production Radar, must not run alongside it, and remains not deployed/runtime-unaccepted.

### `DragonSwordNativeAllMountsFreeFlight/`

Pure-resource all-mount native free-dash Mod. Git contains its reproducible implementation, metadata, and build tooling; extracted game assets, staging trees, backups, PAK artifacts, and local verification copies remain excluded.

## Runtime standard

The canonical UE4SS root is `DS/Binaries/Win64/ue4ss`; Mod installations belong under `DS/Binaries/Win64/ue4ss/Mods/<ModName>`.

## Cross-project boundaries

- Production runtime ownership belongs to `DragonSwordWorldRadar/`.
- DataProbe evidence does not become production truth automatically.
- Generated catalogs must remain build-pinned or install-generated and fail closed when the expected shape changes.
- Runtime behavior matters more than a clean compile for gameplay fixes.

## Local-only trees

`DragonSwordWorldRadar-Dev/` and `DragonSwordWorldRadar-Mole/` may exist beside the published projects on the owner's machine. They are intentionally ignored local workspaces and are not repository content. Never force-add or publish them. Integrate accepted work into `DragonSwordWorldRadar/` explicitly and validate it again there.

## Git and publication policy

- The repository root is this directory, not an individual Mod directory.
- Preserve the inherited Git history and existing `origin` until the user explicitly requests a remote rename or replacement.
- Review root-level `git status` because changes from every Mod are now visible together.
- Scope commits by project and purpose; avoid combining unrelated production, experimental, and preservation changes.
- Do not commit, push, publish a release, or deploy to the game directory without explicit authorization.
- Exclude release ZIPs, `dist/`, runtime state, logs, diagnostics, generated user configuration, and installed data.

## Current handoff state

- Main Radar remains an independent deployed production candidate. Native Auto Pickup 0.6.0 is deployed under the canonical UE4SS root with verified hashes: it corrects the evidence boundary by requiring a same-object DropItemActor/manual-call trace before any later automatic action is implemented. Automatic pickup remains unaccepted.
- Static, compile, package, and scheduling gates passed locally; this is not a substitute for in-game acceptance.
- DataProbe research source and evidence documentation are publishable, while runtime reports, diagnostics, generated local data, and installed state remain excluded.

Before starting work, verify this state against the current filesystem and Git status; it may have changed since this context was written.
