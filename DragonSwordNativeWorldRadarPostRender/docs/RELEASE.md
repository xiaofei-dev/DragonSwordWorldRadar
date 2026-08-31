# DragonSword Native World Radar 2.1.0 Release Contract

## Release identity

| Field | Value |
| --- | --- |
| Product | `DragonSwordNativeWorldRadarPostRender` |
| Version | `2.1.0` |
| Runtime label | `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0` |
| Public diagnostics | disabled |
| Installer | installer-first, unsigned |
| Supported runtime layout | ExperimentalNested |
| Game compatibility | bounded executable AMD64 PE32+ image at the exact path |
| UE4SS compatibility | bounded AMD64 PE32+ loader/proxy plus complete nested structure |
| Load authority | controlling `mods.txt` only |

The version label is an identity, not acceptance evidence. Source, build,
package, installation, gameplay, performance, and publication are separate
states.

The post-2.0.0 corrective scope and exact owner validation matrix are recorded
in [Runtime Feedback Audit for 2.1.0](RUNTIME_FEEDBACK_AUDIT_2_1_0.md).

## Exact technical evidence

Version `1.2.0` was never published; its candidate changes are superseded by
and included in `2.1.0`.

The current 2.1.0 source/static gates, clean native `/WX` build, isolated Setup
`20/20`, manual-copy `2/2`, payload-equivalence, and archive-re-extraction gates
passed. The packaged `main.dll` SHA-256 is
`D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
That DLL is sealed in the authoritative `dist/final-2.1.0` package. Older
`4AFE...` and `BDE21...` outputs are historical and non-authoritative. A local
2.1.0 diagnostic installation later produced runtime evidence, and the owner
reported completed gameplay testing and acceptance on 2026-08-31. The tested
installed DLL was not independently hash-matched to `D4EE...`. Source, build,
installer, package, installed identity, owner acceptance, and publication
rights remain separate evidence states.

The authoritative 2026-08-30 reseal records:

| Artifact | SHA-256 |
| --- | --- |
| `DragonSwordNativeWorldRadarPostRender-Setup-2.1.0.exe` | `EFACFD9B59B0DBB29A61CF7FA053F369C2452638CCC7D0EC8212D2FC64226337` |
| `DragonSwordNativeWorldRadarPostRender-v2.1.0-Installer.zip` | `C52170A38633147EF5A507C8C9BF4CDCD7B8F05BAD61B45D13038FE1FFEE6261` |
| `DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-No-UE4SS.zip` | `15A40005104BB9DC3FD1CD962B234D0D2A8A64B14A896662045A1D792E6DD757` |
| `DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | `95547B9B41169451D2A67114BF36D4BF0F3BA5B2269A15C919CC21AE0296DE3A` |

`dist/final-2.1.0/release-manifest.json` is the machine-readable authority for
the native DLL identity, Setup `20/20`, manual-copy `2/2`, payload equivalence,
clean-target validation, and archive re-extraction evidence.

The expanded-map correction projects from the live `PlayerIconWidget` pivot
into the exact retained/witnessed native icon Canvas and scales X/Y deltas by
that parent's current local width/height. It uses a five-deadline
100/250/500/1,000/1,250 ms tail with one observation per due pass, no overdue
multi-observation collapse, read-only first four passes, and final-pass-only mutation after
stable parent-local geometry. Equal extents may rebase retained hosts; one
stable extent change may consume one candidate-bound full attach. These are
source properties, not runtime acceptance.

## 2.1.0 runtime delta

The immediate treasure path requires the exact interaction receiver and either
the fresh local Pawn or, for mount-only underwater treasures, that Pawn's exact
callback-local `Rider` UObject. The mounted route additionally requires a
non-null same-World relationship and the existing eight-metre receiver bound.
Any other rejected non-Pawn event is not completion. It may arm only one
uniquely resolved treasure ID within eight metres of the current local player.
Fixed arrays wait 15 seconds before an exact-category, positive-only save
request and permit one final request 285 seconds later. Each request is capped
at 64 IDs, runs on the existing below-normal worker, skips encounter and
dynamic-quest SQL, and retains no UObject. This structural bound does not
establish exact 2.1.0 underwater gameplay or frame-time acceptance.

The same final version adds compact-only bird eggs. Exact `Bird_Egg01_C` and
`Bird_Egg02_C` creation events enter a fixed 512-slot weak pool; the existing
250 ms service examines at most eight unresolved candidates per tick. A Bird
Egg is available only when its Actor-owned `DInteractableComponent` reports
both `InteractableValue=2` and `InteractTypeValue=2`; Actor hidden state is not
used. Unknown reflection or ownership reads stay pending for the same bounded
service, and exact Bird Egg EndPlay removes the matching weak identity
immediately. A fixed nearest-16 set receives bounded checks on the same 250 ms
service edge while enabled. F6 controls this RADAR-only category independently; MAP is unavailable.
There is no new polling schedule, enumeration, SQL, dynamic queue, filesystem
polling, or expanded-map work.

The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is the
cross-save owner boundary. Entering it disables the radar, detaches both
renderers, clears weak candidates and mutable save-owned runtime state, and
latches activation off. Loading a save does not reactivate the runtime. F7 is
rejected until a fully loaded open-world identity exists; one explicit F7 there
starts a fresh activation and reconciliation. This edge reuses existing
transition/world identity signals and adds no new poll, enumeration, or SQL.

## Install versus Update / Repair

Setup selects exactly one of these modes after read-only preflight:

1. **Update / Repair**: a structurally complete ExperimentalNested UE4SS
   layout and a strictly owned existing Radar target are present. An older
   owned Radar release is eligible.
2. **Install**: the ExperimentalNested UE4SS layout is structurally complete
   and no owned Radar target exists.
3. **Bootstrap Install**: no UE4SS layout exists. Setup installs its embedded,
   integrity-verified ExperimentalNested runtime and Radar.
4. **Conversion Install**: a root, dual, incomplete, or malformed UE4SS layout
   exists. Setup requires explicit confirmation, creates and verifies a
   complete retained backup, migrates compatible Mod state, and installs the
   ExperimentalNested runtime.

Existing game, `UE4SS.dll`, and `dwmapi.dll` SHA-256 values are transaction
provenance only. They are not version allowlists and do not reject a
structurally compatible update. Embedded loader, proxy, mapping, settings, and
Radar payload hashes remain mandatory Setup-integrity checks.

Update / Repair keeps compatible existing loader, proxy, and settings bytes.
It refreshes the bundled Radar DLL, immutable generated catalogs, release
metadata, and mapping file. It does not extract or regenerate PAK catalogs on
the player's machine.

## User-owned state

Setup preserves these files byte-for-byte after strict validation:

- `config/visibility.ini`
- `config/diagnostics.ini`
- `data/defaults/treasure_overrides.txt`

A clean install creates those live files from embedded defaults. `.example.ini`
files are installer resources only and are not installed. Unknown paths,
malformed configuration, invalid treasure-ignore syntax, unsafe ownership, or
an unrecognized same-name target fails before mutation.

## Backup and rollback

Normal Install and Update / Repair use a bounded local transaction journal.
The journal is removed after a verified commit and is restored on failure. No
persistent copy of the replaced Radar is retained.

Bootstrap/Conversion retains a complete verified original UE4SS-layout backup
because it changes loader topology. The backup is created before active layout
removal and is never required for a normal compatible update.

## Runtime and load-control boundary

The installed product contains native UE4SS payload, declared catalogs,
configuration, metadata, licenses, and notices. It contains no external
renderer executable, Lua renderer, transparent overlay, watcher, or
`enabled.txt`.

The selected ExperimentalNested `mods.txt` is the sole load authority. Setup:

- normalizes exactly one
  `DragonSwordNativeWorldRadarPostRender : 1` entry;
- removes valid obsolete `DragonSwordWorldRadarObjectState` entries instead of
  adding a redundant disabled entry;
- preserves unrelated `mods.txt` bytes, encoding, BOM, and line endings;
- rejects active or malformed `DragonSwordWorldRadar` load control; and
- preserves an exact disabled `DragonSwordWorldRadar : 0` entry byte-for-byte.

## Public artifacts

After the current-source reseal passes, the recommended channel will be the
installer-first archive:

`DragonSwordNativeWorldRadarPostRender-v2.1.0-Installer.zip`

Its exact root allowlist is:

1. `DragonSwordNativeWorldRadarPostRender-Setup-2.1.0.exe`
2. `DragonSwordNativeWorldRadarPostRender-Setup-2.1.0.exe.sha256`
3. `INSTALL.md`
4. `THIRD_PARTY_NOTICES.txt`

Two additional expected archives use the same ExperimentalNested Mod payload:

- `DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-No-UE4SS.zip` for an
  already compatible ExperimentalNested runtime;
- `DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` with the
  pinned integrity-verified ExperimentalNested runtime.

Both manual archives are script-free and map their roots directly to
`DS/Binaries/Win64`. No-UE4SS omits loader, proxy, and settings but includes a
clean one-line `ue4ss/Mods/mods.txt`; it is copied only when the target file is
missing and otherwise its Radar line is merged manually. With-UE4SS contains
the pinned runtime and clean load control and is valid only for a target with
no existing UE4SS. Neither manual archive contains a StableRoot payload. The
next authoritative `dist/final-2.1.0` allowlist must be exactly those three ZIPs,
`release-manifest.json`, and `SHA256SUMS.txt`.

## Static acceptance gate

Historical pre-reseal Setup bytes compiled through the checked-in builder and
their isolated installer matrix reported exactly 20 passed, 0 failed, and 0
skipped. That matrix proved the following for those bytes and must be rerun
against a Setup embedding the current `D4EE...` native DLL:

- structurally valid changed game and UE4SS DLL hashes do not block;
- malformed binaries and unsafe layouts still fail closed;
- older strictly owned Radar versions enter Update / Repair;
- all three user-owned files remain byte-identical;
- immutable catalogs are refreshed;
- successful normal/update paths retain no persistent backup;
- confirmed uninstall removes only strictly owned Radar state;
- stale uninstall confirmation performs no mutation; and
- injected uninstall failure restores Radar and `mods.txt` exactly;
- conversion retains a verified full backup; and
- injected failures roll back byte-exactly.

The historical isolated manual-install matrix reported exactly 2 passed, 0
failed, and 0 skipped. The current reseal must re-prove payload equivalence with
Setup, the exact
ExperimentalNested load-control line, public diagnostics disabled, absence of
`enabled.txt`, and inclusion/exclusion of the pinned runtime in the correct
manual archive.

Static and installer evidence does not prove runtime gameplay or performance.

## Gameplay acceptance

The future exact resealed and installed hashes require owner testing of:

- cold start, F7 activation, F8 disable, and F8/F7 resynchronization;
- exact title-screen hard stop, no automatic activation while another save
  loads, rejection of pre-open-world F7, and one fresh explicit F7 activation
  after the loaded open world is ready;
- F6 visibility controls and persistence, including independent compact-only
  bird eggs with an unavailable MAP cell, area-quest `AVAILABLE` / `ALL`, and
  Assault `AVAILABLE` / `ALL`; ALL must show the static 40-record Assault
  catalog, and switching back to AVAILABLE must immediately restore state,
  time-window, and cooldown filters;
- compact and expanded rendering, pan, zoom, maximum zoom, close/reopen, and
  minimize/restore;
- treasure, clustered/overlapping treasure, Boss, Assault, mini-game, area
  task, compact-only bird egg, clock, and height-indicator behavior, including
  Bird Egg availability from the exact owned component values, retry of unknown
  reads, and immediate exact EndPlay removal;
- Boss/Assault defeat and area-task completion without F8/F7 dependency;
- dungeon/travel/open-world return and long-session stability; and
- diagnostic-off startup plus real frame-time comparison.

## Evidence states

| State | Current status | Meaning |
| --- | --- | --- |
| `SOURCE_VALIDATED` | `PASSED` | current source and static gates passed |
| `BUILT` | `PASSED` | current native DLL built and receipt-verified |
| `PACKAGED` | `PASSED` | current D4EE package set re-extracted byte-identically |
| `INSTALLER_TESTED` | `PASSED` | current Setup 20/20 and manual-copy 2/2 matrices passed |
| `DEPLOYED` | `OBSERVED_UNMATCHED` | a local 2.1.0 diagnostic build ran; exact installed D4EE identity was not recorded |
| `GAMEPLAY_ACCEPTED` | `OWNER_ACCEPTED_2026_08_31` | owner reported completed gameplay testing and acceptance |
| `SOURCE_PUBLICATION_AUTHORIZED` | `PASSED` | owner authorized upload of the workspace source |
| `BINARY_AND_DERIVED_DATA_PUBLICATION` | `BLOCKED` | rights/provenance review remains incomplete |

No state implies a later state.

## Publication blockers

Public binary and derived-data publication remains blocked independently of
technical readiness until:

1. Unreal Engine/UEPseudo authorization, redistribution, and license
   compatibility are confirmed.
2. Exact `e_sqlcipher.dll` source/build provenance is recorded and reviewed.
3. Redistribution rights and attribution for generated catalogs and derived
   coordinate data are confirmed.

Do not publish the blocked binary or derived inputs, a binary archive, or a
release tag until all three reviews are cleared. Source-only workspace upload
is separately authorized and excludes those local inputs.
