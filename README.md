# DragonSword Mod Workspace

This repository is the control workspace for the DragonSword: Awakening Mod
suite. It uses a monorepo layout: maintained products and research projects
live at the repository root, retired implementations live under `Archive/`,
and repository-wide standards live under `docs/`.

The GitHub repository is `xiaofei-dev/DragonSwordMods`; the historical
`DragonSwordWorldRadar` URL redirects to it.

## Workspace layout

| Directory | Role | Status |
|---|---|---|
| `DragonSwordNativeWorldRadarPostRender/` | Native in-game World Radar | Current local release: `2.3.0`; see product release status |
| `DragonSwordNativeAutoPickup/` | Native automatic pickup | Current local release: `1.3.1`; see product release status |
| `DragonSwordPickupRangeExpansion/` | Optional interaction-range PAK variants | Maintained independent product |
| `DragonSwordNativeAllMountsFreeFlight/` | Pure-resource all-mount free-flight PAK | Maintained independent product |
| `DragonSwordWorldDataProbe/` | Independent read-only data-capture test Mod | Retained at the root even when inactive; it is not archived |
| `DragonSwordUE4SSCompatibilityRuntime/` | Shared pinned UE4SS compatibility payload | Maintained support component, not a gameplay feature |
| `docs/` | Shared installation, release, integration, and acceptance standards | Repository-wide documentation |
| `Archive/DragonSwordWorldRadar/` | Retired external-renderer Radar | Frozen reference; not an active build or deployment target |

The superseded `DragonSwordNativeWorldRadar` and
`DragonSwordWorldRadarObjectState` prototypes are no longer present in the
working tree. Their history remains available through Git.

## Current integration status

The complete local release handoff is indexed in
[`docs/RELEASE_CLOSEOUT_2026_09_07.md`](docs/RELEASE_CLOSEOUT_2026_09_07.md).
Package verification, gameplay acceptance, and publication are separate states.

Historically, the owner accepted Radar 2.1.0 and AutoPickup 1.3.0 on
2026-08-31. That acceptance does not validate later packages. During diagnosis, one mounted-flight session
showed a temporary loss of the native `F` interaction prompt and automatic
pickup confirmations; normal behavior later recovered during continued flight.
The evidence did not establish a direct Radar hook conflict. This observation
is retained as a known diagnostic record and is not an acceptance or
publication blocker.

See [`docs/INTEGRATION_STATUS.md`](docs/INTEGRATION_STATUS.md) for the exact
evidence boundary.

## Repository rules

- Read this file, the root `PROJECT_CONTEXT.md`, and the selected product's own
  context before changing a Mod.
- Keep source, identifiers, metadata, project documentation, tests, and commit
  messages in English.
- Keep product ownership independent. Repository-level organization does not
  make one Mod responsible for another Mod's runtime behavior.
- Do not deploy, launch, or terminate the game as part of repository cleanup.
- Keep build output, runtime logs, extracted game data, credentials, and local
  release packages out of Git.
- Treat owner gameplay acceptance, exact installed hashes, technical build
  evidence, and third-party redistribution rights as separate facts.

## Public repository boundary

The existing public GitHub repository is the workspace remote. Source,
first-party assets, tests, and documentation may be published there. Local
build output, runtime evidence, extracted game files, credentials, and release
packages remain excluded.

Nexus descriptions, support-post drafts, covers, upload artwork, and gameplay
screenshots are local-only. Extracted research reference tables and catalogs
are also excluded. Required first-party F6 UI resources remain tracked.
See [public source policy](docs/PUBLIC_SOURCE_POLICY.md). These exclusions
apply to the current tree; previous Git history has not been rewritten.

PostRender's bundled SQLCipher binary and generated/derived game catalogs are
also excluded from new public commits until their recorded provenance and
redistribution review is complete. They may remain available locally for
authorized development and packaging; a public checkout is therefore a source
review checkout, not a complete binary-release build environment.

## Starting work

Use each product's checked-in verification entry points. Common examples:

```powershell
Set-Location .\DragonSwordNativeWorldRadarPostRender
& .\tools\Verify-Source.ps1

Set-Location ..\DragonSwordNativeAutoPickup
& .\tools\Verify-Source.ps1
& .\tools\Build-Core.ps1
```

DataProbe changes require its active profile, method matrix, and explicit
read-only collection boundary to be reviewed first.

## License

First-party project work is licensed under `GPL-3.0-only`. See
[`LICENSE`](LICENSE), [`LICENSE_SCOPE.md`](LICENSE_SCOPE.md), and
[`THIRD_PARTY_NOTICES.txt`](THIRD_PARTY_NOTICES.txt). The GPL grant does not
cover game content, generated Unreal material, or third-party components.
