# DragonSword suite source closeout - 2026-09-10

The owner requested a complete workspace closeout and synchronization to GitHub
main, covering every project rather than Radar alone. This is a source and
documentation handoff, not a new binary release, deployment, or gameplay fix.

## Product inventory

| Project | Current identity | Scope in this sync |
| --- | --- | --- |
| Native World Radar PostRender | Published 3.0.0; local candidate `radar-3.0.0-sg16-guide4-20260909` | Scene markers, focus modes, F6 redesign/Guide, live map settings, localization, source gates, tests and release documentation |
| Native AutoPickup | Published 1.3.1; separate unreleased `frame-debug-20260908` source candidate | Bounded repeated-dispatch accounting, frame-paced work, deferred scalar diagnostics, tests and candidate evidence; no replacement of published binaries |
| Pickup Range Expansion | 1.3.1; 3x/5x/10x/15x/20x variants | No source change; existing range package verified with the AutoPickup release manifest |
| All-Mounts Free Flight | 1.4.0 in product metadata | No source or artifact change; existing `NOT_VALIDATED` gameplay status retained |
| World DataProbe | 1.0.55; healthcheck-only active profile | Corrected the production Radar reference; no collection, probe activation or runtime change |
| UE4SS Compatibility Runtime | v3.0.1 Beta #0, commit `1c1a1497` | No change to the pinned support component |
| Archived external WorldRadar | Frozen reference implementation | Unchanged; not an active runtime target |

The root README, project context and integration index now distinguish current
published versions from historical acceptance and unreleased working source.
The invalid root Radar `Verify-Source.ps1` example is replaced with actual
product-owned core/source verification commands.

## Radar release and support

The owner reports publishing 3.0.0 and supplied screenshots of the Nexus file
page. This is publication evidence, not an independent hash check of remote
downloads. The local three archives in `dist/final-3.0.0/` were freshly checked
against `release-manifest.json`; all sizes and SHA-256 values match. The exact
Guide4 package, installer and deployment receipts remain in the product's
`docs/RELEASE_STATUS.md` and local candidate directory.

Two user reports remain open and await logs/reproduction:

1. Some chests reportedly remain marked after having been opened before Mod
   installation. The source already reads save completion records on activation;
   mid-playthrough installation is not an expected limitation. No save/ID bug
   or other specific cause has been established from the comment alone.
2. Settings and clock work at first world entry, but markers appear only after
   fast travel. Initialization timing and conflicts are unconfirmed hypotheses.
   Radar debug and UE4SS logs have been requested; the owner reports not being
   able to reproduce the issue locally.

The current override file accepts `ignore <save ID>` on separate lines and is
loaded at startup. Native 3.0 has no on-screen chest-ID setting. The archived
external renderer's `diagnostic_verbose` option does not apply to Native 3.0.
Easier individual-marker hiding is a future consideration, not shipped work.

## AutoPickup release boundary

The published 1.3.1 archive set remains unchanged. All four ZIPs, including the
independent Range package, were freshly size/hash-checked against
`dist/releases/1.3.1/DragonSwordAutoPickup-v1.3.1.release.json` and match.

The working source contains the separately documented September 8 candidate.
It preserves an exact-Component retry budget across unconfirmed dispatches,
processes confirmation before dispatch, uses frame-paced active work and
bounded idle scanning, and keeps diagnostic formatting after decisions.
Its earlier deployment/READY evidence does not establish that all reported
interaction blockage is fixed. Committing this candidate to main does not
publish it as a new release or overwrite the existing 1.3.1 archives.

## Fresh validation

- AutoPickup `Verify-Source.ps1`: selector, 88-file manifest, package-layout,
  mods.txt gates and core build/CTest passed (1/1).
- Radar `Build-Core.ps1`: isolated core build/CTest passed (9/9).
- Radar F6 assets: all 11 languages checked; Guide has 539 text cells and
  40 icon examples, with all three height columns and the final entry order.
- Radar compact renderer, world-map canary, PostRender safety and release
  hygiene gates passed. Included mutation gates rejected 9 map-preview,
  10 scene-frame, 12 confirmation and 8 Guide regressions.
- Radar 3/3 and AutoPickup/Range 4/4 existing release archives match manifests.

The initial sandboxed Radar core build could not write its isolated build
directory; the scoped elevated rerun passed. No native gameplay DLL rebuild,
installer-matrix rerun, game launch, deployment or package regeneration was
performed. Earlier installer/package test receipts retain their original dates.

## Git and public-source boundary

The initial successful fetch found local main equal to origin/main at
`dd8b57c0`. The remote retains the historical DragonSwordWorldRadar URL;
repository documentation identifies its canonical name as DragonSwordMods.
Synchronization uses a regular main push without history rewriting.

Only source, tests, engineering documentation, metadata, licensed build fonts
and required first-party UI resources are included. Nexus copy/screenshots,
release packages, runtime logs, SDKs, SQLCipher binaries and extracted game
catalogs remain local under the existing public-source policy. A public clone
still needs independently supplied excluded inputs for a complete release build.

The staged index passed `docs/Test-PublicSourceBoundary.ps1` (676 tracked files,
14 path fixtures), whitespace, file-size and changed-text credential-pattern
checks. The 185 changed files include no file larger than 50 MiB; the largest
is a 1,336,049-byte F6 tooltip asset. Root documentation links resolve locally.
Both pinned font binaries and their original license files match the index
byte-for-byte. Narrow `.gitattributes` rules preserve the two upstream license
files, including their original whitespace, instead of modifying licensed inputs.
Final commit and
remote SHA equality are verified after push and reported in the task; this
pre-commit record does not itself assert a successful remote delivery.
