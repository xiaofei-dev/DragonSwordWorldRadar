# DragonSword Mod Workspace

This repository is the shared source workspace for multiple **DragonSword Awakening** mods and research tools. Each top-level directory owns an independent runtime or preservation role. Do not copy lifecycle, deployment, or experimental code between projects without first checking that project's context and acceptance boundary.

## Projects

| Directory | Role | Current status |
|---|---|---|
| `DragonSwordWorldRadar/` | Main production radar for treasures, Bosses, Assault targets, and Mole/Fly activities | Deployed test candidate `0.4.0-dev59-ue4ssroot1`; gameplay acceptance remains pending |
| `DragonSwordNativeAutoPickup/` | Native C++ owner-authorized ordinary ground-loot pickup canary | Deployed `0.3.2-relocated-runtime-canary`; runtime acceptance remains pending |
| `DragonSwordWorldDataProbe/` | Read-only data collection and research framework | Active research; observations must not be promoted directly into production behavior |

## Runtime standard

UE4SS is rooted at `DS/Binaries/Win64/ue4ss`. Publishable UE4SS Mods install under `DS/Binaries/Win64/ue4ss/Mods/<ModName>`.

## Repository rules

- Read the target project's `PROJECT_CONTEXT.md` before making changes.
- Keep repository artifacts, code, identifiers, tests, metadata, and technical documentation in English.
- Treat `DragonSwordWorldRadar/` as the production owner unless a feature is explicitly being validated in an isolated project.
- Keep experimental probes in `DragonSwordWorldDataProbe/`; do not add them to the production Radar by default.
- Preserve historical snapshots and research evidence. Prefer append-only successors or new modules over destructive rewrites.
- Do not treat compilation or packaging as in-game acceptance.
- Do not commit generated runtime state, diagnostics, local configuration, release ZIPs, or `dist/` output.
- Do not deploy, publish, or push without explicit authorization.

## Common entry points

### Main Radar

```powershell
Set-Location .\DragonSwordWorldRadar
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
```

### Native Auto Pickup

```powershell
Set-Location .\DragonSwordNativeAutoPickup
& .\tools\Verify-Source.ps1
& .\tools\Build-Core.ps1
```

### DataProbe

Read `DragonSwordWorldDataProbe/PROJECT_CONTEXT.md` before enabling or changing a probe. Runtime collection must remain bounded, read-only, explicitly whitelisted, and evidence-labeled.

## Git history

The repository history originated in the standalone `DragonSwordWorldRadar` repository and was promoted to this workspace root when the project became a multi-mod repository. Earlier commits therefore describe the main Radar at the old repository root; current work places that source under `DragonSwordWorldRadar/` alongside the other projects.

The existing `origin` remote may retain the historical repository name until it is intentionally renamed on GitHub. Changing the local layout does not rename the remote repository automatically.

## Local-only development trees

`DragonSwordWorldRadar-Dev/` and `DragonSwordWorldRadar-Mole/` are intentionally excluded by the root `.gitignore`. They are local development or feature-validation workspaces, are not part of the shared repository, and must not be staged, committed, or published. Any accepted feature must be deliberately integrated into `DragonSwordWorldRadar/` and pass that project's validation gates before publication.
