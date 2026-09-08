# DragonSword Multi-Mod Repository Context

## Purpose

`G:\my_projects\game_mods\DragonSword` is the control root for all maintained
DragonSword: Awakening Mod development. This file is the first cross-chat
orientation document. Read the selected product's own context and current Git
diff before making product-level changes.

## Ownership map

### Maintained gameplay products

- `DragonSwordNativeWorldRadarPostRender/` owns native World Radar rendering,
  data selection, and its own F6/F7/F8 controls.
- `DragonSwordNativeAutoPickup/` owns automatic interaction selection and
  Enhanced Input pickup invocation.
- `DragonSwordPickupRangeExpansion/` owns authored optional interaction-range
  PAK variants only.
- `DragonSwordNativeAllMountsFreeFlight/` owns the removable pure-resource
  all-mount free-flight change only.

### Research and shared support

- `DragonSwordWorldDataProbe/` is an independent read-only data-capture test
  Mod. It remains at the repository root even while inactive and must not be
  moved into `Archive/` merely because it is not currently running.
- `DragonSwordUE4SSCompatibilityRuntime/` is a pinned compatibility payload,
  not an independently enabled gameplay Mod.
- `docs/` owns repository-wide installation, release, integration, and
  acceptance guidance.

### Archive and removed prototypes

- `Archive/DragonSwordWorldRadar/` is the frozen external-renderer Radar
  implementation retained for history and comparison. It is not active.
- `DragonSwordNativeWorldRadar` and `DragonSwordWorldRadarObjectState` were
  superseded prototypes and are intentionally absent from the working tree.
  Git history remains their archive of record.
- There is no repository-root `tools/` directory. Each product owns its own
  build and verification tooling.

## Current local release handoff

See `docs/RELEASE_CLOSEOUT_2026_09_07.md` and each product's
`docs/RELEASE_STATUS.md` for AutoPickup 1.3.1 and Radar 2.3.0. These are
complete local package sets, not authorization to publish or deploy them.
Use current manifests for byte identities; dated receipts below are history.

The owner subsequently published AutoPickup 1.3.1 and Radar 2.3.0 on Nexus
and authorized the source/documentation main sync on 2026-09-07. See
`docs/GITHUB_CLOSEOUT_2026_09_07.md` for the final source handoff and evidence
boundary. This does not clear the separately recorded third-party rights gate.

## Historical accepted product state

The owner reported completed gameplay testing and acceptance on 2026-08-31 for
the then-installed `DragonSwordNativeWorldRadarPostRender` 2.1.0 and
`DragonSwordNativeAutoPickup` 1.3.0 builds.

One diagnostic session observed a temporary native `F` prompt loss and missing
AutoPickup confirmations during mounted flight. The prompt and automatic
behavior later recovered during continued flight. Logs showed successful input
injection without confirmation, a mounted Pawn transition, and later
`ClientRestart` lifecycle activity. They did not prove a direct Radar hook
conflict. Preserve the observation in `docs/INTEGRATION_STATUS.md`; do not
rewrite it as either a proven Radar conflict or a resolved root cause.

Owner acceptance is an authoritative product decision. It does not by itself
prove the SHA-256 identity of an installed artifact, fill every detailed test
matrix row, or clear third-party redistribution rights.

## Publication boundary

- The repository root is the Git root. The canonical remote is
  `xiaofei-dev/DragonSwordMods`; the old DragonSwordWorldRadar URL redirects.
- Public commits may contain first-party source, tests, metadata,
  documentation, and first-party media.
- As of 2026-09-08, Nexus publishing copy and artwork, gameplay screenshots,
  and extracted research reference datasets are local-only. Keep their local
  files while removing Git tracking; do not force-add them. Required F6 UI
  resources are implementation assets, not promotional media. See
  `docs/PUBLIC_SOURCE_POLICY.md` and its staged-tree check.
- Do not commit runtime logs, build output, release archives, extracted game
  files, save data, databases, keys, credentials, or local deployment state.
- PostRender's local `assets/vendor/sqlcipher/e_sqlcipher.dll` and
  `src/data/generated/` catalogs remain excluded from new public commits until
  provenance and redistribution review is complete.
- A public checkout is not guaranteed to reproduce a full binary release until
  those local-only dependencies are independently cleared and supplied.
- User authorization to upload the workspace does not silently assert ownership
  of third-party or game-derived material.
- First-party work uses `GPL-3.0-only` under the exact boundary recorded in
  `LICENSE_SCOPE.md`; a copied GPL text never relicenses excluded material.
- Each independently distributed project must carry its applicable `LICENSE`
  plus all relevant third-party notices and upstream license files.

## Safety and evidence rules

- Preserve unrelated dirty work and inspect root-level Git state before edits.
- Never copy secrets, PAK keys, database keys, absolute process addresses, or
  extracted proprietary payloads into source or documentation.
- Keep source/static, build/package, installed-hash, runtime, owner-acceptance,
  and publication-rights evidence separate.
- Archived and research code never becomes active product behavior implicitly.
- Repository cleanup does not authorize deployment, game launch, game process
  termination, or modification of the game installation.

## Continuation checklist

1. Confirm the target product directory.
2. Read its `README.md`, `PROJECT_CONTEXT.md`, metadata, and relevant evidence.
3. Inspect the root Git diff and preserve work outside the requested scope.
4. Use product-owned verification scripts without deploying or launching the
   game unless that separate action is explicitly authorized.
5. Record new evidence at the correct level and retain uncertainty where the
   evidence does not establish a root cause.
