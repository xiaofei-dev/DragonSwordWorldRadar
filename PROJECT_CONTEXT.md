# DragonSword Multi-Mod Repository Context

## Repository purpose

This is a multi-project repository for the production DragonSword Awakening Radar, owner-authorized native Mod canaries, and read-only research tooling. The Git history was inherited from the original standalone DragonSwordWorldRadar repository and promoted to this directory so related publishable projects can be managed together.

Use this file as the first cross-chat orientation document. Then read the selected project's own `PROJECT_CONTEXT.md` and current metadata before acting.

## Ownership map

### `DragonSwordWorldRadar/`

Primary production Mod and authoritative owner of the integrated treasure, Boss, Assault, and Mole/Fly radar. Current source is `0.4.0-dev74-processdispatchguard1-localcapfix1` with one protocol-v6 Motion/control bridge. Preserve the acceptance boundary between passed Windows/package gates and owner gameplay validation.

### `DragonSwordNativeAutoPickup/`

Native C++ UE4SS research Mod for owner-authorized ordinary ground-loot pickup. Version `0.9.0-native-visibility-f-input` uses an exact-version native visibility detour and sends one F scan-code press/release through the game's normal interaction path. It is deployed but not runtime accepted.

### `DragonSwordWorldRadarObjectState/`

Experimental native-object-state successor to the production Radar. Version `0.5.0-dev2-native-presence` moves bounded high-frequency state collection into a native provider while retaining the external Overlay and Lua world-map geometry. It is source/build validated but not production or gameplay accepted.

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

- Main Radar remains an independent deployed production candidate. Native Auto Pickup 0.9.0 is deployed for Steam build `24693558`; automatic pickup remains unaccepted until owner gameplay testing.
- Static, compile, package, and scheduling gates passed locally; this is not a substitute for in-game acceptance.
- DataProbe research source and evidence documentation are publishable, while runtime reports, diagnostics, generated local data, and installed state remain excluded.

Before starting work, verify this state against the current filesystem and Git status; it may have changed since this context was written.
