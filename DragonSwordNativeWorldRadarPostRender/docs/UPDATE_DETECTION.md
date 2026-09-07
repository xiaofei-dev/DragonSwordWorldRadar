# Game Update Compatibility

Native World Radar does not ship a game-update watcher, network updater,
external host, automatic reinstall prompt, or recurring compatibility scan.
A changed game executable hash alone neither blocks Setup nor disables the
installed Mod.

## Installer boundary

Setup locates Steam App `4570720` or accepts the explicitly selected executable
only at the exact `DS/Binaries/Win64/DSClient-Win64-Shipping.exe` path. Before
mutation it validates a bounded executable AMD64 PE32+ application image. It
rejects malformed, truncated, oversized, non-AMD64, non-PE32+, DLL, wrong-name,
wrong-path, or active-game inputs.

The current game SHA-256 is still calculated, but only for the confirmed
transaction identity, revalidation immediately before mutation, and the install
record. It is not compared with a fixed game-build allowlist. An already active
ExperimentalNested UE4SS loader and proxy are accepted by bounded x64 PE32+
file and directory structure, not by a fixed DLL-hash allowlist. Embedded
bootstrap/conversion and Radar payload hashes remain integrity checks for bytes
shipped inside Setup; they are not compatibility gates for existing DLLs.

Setup never launches or terminates the game, scans game objects, extracts PAKs,
regenerates catalogs, or modifies `DS/Saved`.

## Install versus Update / Repair

Setup automatically selects Update / Repair only when both a structurally
complete ExperimentalNested UE4SS layout and a strictly owned existing Radar
installation are present. It preserves `config/visibility.ini`,
`config/diagnostics.ini`, and `data/defaults/treasure_overrides.txt`, replaces
the installer-owned DLL and all bundled generated catalogs, and removes its
temporary rollback journal after a successful commit.

Missing UE4SS bootstraps the embedded runtime. Root, dual, malformed, or
incomplete UE4SS layouts use the confirmed conversion path, which retains a
complete original-layout backup and migrates Mods and load-control state.
A structurally valid older Radar release remains eligible for Update / Repair.

## Runtime owner-pointer compatibility

`save_owner_pointer.cfg` retains the source executable fingerprint, file length,
owner-pointer RVA, and provenance. It contains neither the SQLCipher key nor an
absolute process address.

The packaged owner RVA is a zero-pattern-scan fast path, and verified member
offset `0x128` is a zero-field-scan fast path. Neither is a fixed compatibility
requirement. The owner-pointer RVA and the key member's offset inside that owner
are independent compatibility layers. The September 2026
game build identified locally by Steam build `25076183`, executable length
  `162606488`, and SHA-256
  `B3E0B8CAB6752ACB981E104CA95A0105F76FCDD42EE622A8063AB8DE44FCA94C`
  moved the owner RVA to
`0x94F4FA8` and the key `FString` from owner offset `0x120` to `0x128`.

If every key candidate reached through the packaged owner fails to authenticate
the current active `.db`, a FullActivation clears the stale numeric fast-path
state, maps the current executable, and scans its executable PE sections at
most once for the lifetime of the game process. Each section scan is limited
to `min(SizeOfRawData, VirtualSize)`, so raw padding outside the mapped section
cannot create a match. This also covers a stale
packaged RVA in an update whose executable happens to retain the same file
length. The resolver:

- accepts exactly one complete owner-pointer instruction pattern whose
  RIP-relative target remains inside `SizeOfImage`;
- counts only those valid in-image targets, so an incidental complete byte
  pattern with an out-of-image target neither creates ambiguity nor wins;
- rejects zero or multiple valid in-image targets;
- caches only the resolved numeric RVA;
- reruns the same bounded key-field discovery against that owner;
- validates the current live save key against the active `.db` before
  reconciliation continues.

After resolving the owner, reconciliation tries a process-cached key-field
offset, legacy `0x120`, verified current `0x128`, and only then a bounded aligned
owner-field scan. A structurally plausible candidate is accepted only when it
authenticates the active `.db` through a real `sqlite_master` read. A `.bak`
cannot select an older key. Packaged-owner fast offsets are tried first without
opening its aligned neighborhood scan. If they fail and the structural owner is
needed, it receives only the remaining part of one shared budget: at most 24
active `.db` key validations total across both owner routes in one explicit F7
FullActivation. A successful numeric offset is cached for the process.
Neither key bytes nor absolute process addresses are logged or persisted.

Failure to find one unique structural signature or to authenticate a key after
the retry disables only that save-reconciliation attempt and remains fail
closed.
The F6 status becomes Fault, and F6 Retry or an explicit F7 starts one fresh
bounded activation.
There is no game-thread scan, recurring retry, timer, watcher, UObject
enumeration, or retained UObject. A later manual F7 can retry live-key access
through the already cached numeric result, but it cannot replenish the one
process-lifetime pattern scan.

The historical exact 2.2.0 clean-build and local developer-deployment evidence
is bound
to native DLL SHA-256
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`
from compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
The rollback-backed local deployment passed with diagnostics enabled, but it is
not Setup ownership and does not validate runtime/gameplay behavior.
`Build-Release.ps1` package validation passed for that exact DLL: Setup reports
`20/20`, Manual reports `2/2`, payload equivalence, manual layout, and clean-target
policy validation pass, and all three public ZIPs re-extract byte-identically.
These records remain historical 2.2.0 evidence. Version 2.2.1 changes only the
world-map ownership boundary. The deployed outer-atlas-rectangle DLL
`CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2`, from
compiled source
`433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`, is
runtime rejected. Its negative atlas-left outer slot changed the native parent
extent from `3000x3000` to `3191.520996x3000`, causing a self-induced rebuild
loop with 6 attaches and 5 detaches, flashing, and an empty map. The same run
populated 1,632 markers and reported no data, texture, or ABI fault. Backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`
remains rollback evidence for those rejected bytes only.

The later rejected dynamic-anchor-rebase architecture kept both outer Mod hosts and their inner
`Panel_Point` slots full stretch with zero offsets, uses maximum Z only on the
outer slots, and places `{atlas_left,atlas_top,atlas_width,atlas_height}` only in
the Image Canvas slots with Image render translation `(0,0)`. It also starts the
finite tail from a fresh post-attach clock, omits the attach-end empty Retainer
`RequestRender`, keeps rebuild requests report-only until scheduling succeeds,
and requires two matching successful extent observations before an extent-
driven rebuild. Source/static gates, Core `2/2`, release hygiene, and the local
native build pass
at DLL `5C632820CC44065AB9FEDDA260A392B5C72655D3815F445C73CF4A50A72DD606`,
from compiled source
`798297BBF7F9791A9E2FFBFF4EC5894A62CC4B16B825CB6BAEBC19A398CB2585`, size
1,110,016 bytes. Rollback-backed diagnostics-enabled local developer deployment
passes for the exact DLL. Installed identity matches, the single controlling Mod
entry is enabled (`mods=1`), and `debug_logging=true`; backup:
`dist/work/deployment/deploy-backups/20260905-191717-084-native-only-deploy`.
Package and installer remain pending; gameplay, visual, performance, and
resolution acceptance remain `NOT_VALIDATED`.

WM-06 supersedes that rejected deployment. It removes retained PlayerIcon
rebasing, keeps same-parent/same-extent refresh observation-only, and passes
source/static gates, Core `2/2`, release hygiene, native build, rollback-backed
developer deployment, Setup `20/20`, Manual `2/2`, payload equivalence, layout,
clean-target, and byte-identical re-extraction of all three ZIPs for DLL
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`
from compiled source
`0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`.
Gameplay, visual, click-target, resolution, exit, and performance acceptance
remain `NOT_VALIDATED`; binary publication remains blocked by the recorded
rights and provenance reviews.

## What F7 does

F7 activates the already installed native Mod and starts its bounded runtime
and save-backed resynchronization. It does not run Setup, replace files, extract
game data, regenerate `save_owner_pointer.cfg`, or rebuild immutable catalogs.
Therefore F7 is the correct normal recovery action after a structurally
compatible update, but it is not equivalent to installation. This mechanism is
update tolerant; it does not guarantee compatibility with every future game
version.

## When a new Radar release is still required

A new data release is required only when a game update actually changes static
records used by the immutable treasure, Boss, Assault, mini-game, or area-quest
 catalogs, changes coordinate semantics, or breaks a reflected/runtime schema.
 Runtime structural and schema validation must
omit or disable an affected feature rather than guess.

After such a confirmed change:

1. Regenerate only the affected immutable data through the separate validated
   generation pipeline.
2. Update bounded schemas, metadata, and tests without weakening fail-closed
   checks.
3. Run core tests, static gates, the pinned native build, installer tests, and
   package verification.
4. Collect fresh gameplay evidence for F7/F8, travel, save reconciliation,
   compact/expanded rendering, task completion, and clean shutdown.

File timestamps, a successful build, or a changed executable hash alone do not
prove either compatibility or incompatibility.
