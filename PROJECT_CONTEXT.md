# DragonSword Multi-Mod Repository Context

## Repository purpose

This is a multi-project repository for the production DragonSword Awakening Radar, owner-authorized native Mod canaries, and read-only research tooling. The Git history was inherited from the original standalone DragonSwordWorldRadar repository and promoted to this directory so related publishable projects can be managed together.

Use this file as the first cross-chat orientation document. Then read the selected project's own `PROJECT_CONTEXT.md` and current metadata before acting.

## Ownership map

### `DragonSwordWorldRadar/`

Primary production Mod and authoritative owner of the integrated treasure, Boss, Assault, and Mole/Fly radar. Current source is the deployed `0.4.0-dev59-ue4ssroot1` candidate with one protocol-v5 Motion/control bridge. Preserve its existing dirty working tree and validate changes through the project's Windows PowerShell 5.1 gates.

### `DragonSwordNativeAutoPickup/`

Native C++ UE4SS canary for bounded ordinary ground-loot pickup. It targets the exact relocated UE4SS runtime fingerprint, is armed independently with F9, performs no recurring Lua scan, and remains fail-closed on unknown builds. Compilation and deployment do not establish gameplay acceptance.

### `DragonSwordWorldDataProbe/`

Research and diagnostics framework. It may contain active, disabled, manual-only, experimental, prohibited, and archived methods in the same tree. File presence never proves that a module is active or accepted. Keep game access bounded and read-only, preserve raw evidence, and distinguish confirmed observations from inference.

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

- Main Radar `0.4.0-dev59-ue4ssroot1` and Native Auto Pickup `0.3.2-relocated-runtime-canary` are deployed under the canonical UE4SS Mods root and are awaiting combined F7/F9 gameplay validation.
- Static, compile, package, and scheduling gates passed locally; this is not a substitute for in-game acceptance.
- DataProbe remains dirty local research work and is outside the current publication scope.

Before starting work, verify this state against the current filesystem and Git status; it may have changed since this context was written.
