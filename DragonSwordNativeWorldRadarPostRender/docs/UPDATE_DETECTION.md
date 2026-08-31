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

The packaged RVA is the zero-scan fast path when its executable length and live
save-key validation succeed. If a game update moves that pointer, the existing
below-normal save worker maps the current executable and scans executable PE
sections at most once for the lifetime of the game process. The resolver:

- accepts exactly one complete owner-pointer instruction pattern;
- rejects zero or multiple matches;
- requires the RIP-relative target to remain inside `SizeOfImage`;
- caches only the resolved numeric RVA;
- validates the current live save key before reconciliation continues.

Failure disables only that save-reconciliation attempt and remains fail closed.
There is no game-thread scan, recurring retry, timer, watcher, UObject
enumeration, or retained UObject. A later manual F7 can retry live-key access
through the already cached numeric result, but it cannot replenish the one
process-lifetime pattern scan.

## What F7 does

F7 activates the already installed native Mod and starts its bounded runtime
and save-backed resynchronization. It does not run Setup, replace files, extract
game data, regenerate `save_owner_pointer.cfg`, or rebuild immutable catalogs.
Therefore F7 is the correct normal recovery action after a compatible update,
but it is not equivalent to installation.

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
