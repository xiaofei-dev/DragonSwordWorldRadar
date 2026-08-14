# DragonSwordWorldRadarObjectState

DragonSwordWorldRadarObjectState is an experimental successor to
DragonSwordWorldRadar. It keeps the established external WinForms rendering
and UMG world-map geometry paths while moving low-risk, high-frequency state
collection to a native UE4SS C++ provider.

Version: `0.5.0-dev2-native-presence`

This directory is independent from the frozen production
`DragonSwordWorldRadar` directory. Do not copy changes between them without an
explicit review. A successful build is not deployment or in-game acceptance.

## Runtime model

- C++ samples the fresh current Controller/Pawn position every 33 ms. Pawn,
  Controller, World, Canvas, and actor pointers are never retained across
  callbacks.
- C++ receives BeginPlay and EndPlay lifecycle callbacks. It stores only weak
  object index/serial identities and catalog IDs.
- F7 starts a new native activation and requests exactly one save
  reconciliation attempt. The attempt is consumed before fallible process or
  SQL work. Failure is not retried until the next F7.
- F8 invalidates pending save work, clears native observations, and stops
  object collection.
- A treasure actor ending with `Destroyed` near the current player is treated
  as opened for the current activation. The next F7 reconciles any discrepancy
  with the save.
- Current-activation native events are merged into a returning SQL snapshot so
  an older snapshot cannot resurrect a just-opened marker.
- The native provider publishes numeric coordinates, events, and bounded
  counters through a fixed-size seqlock memory mapping. No UObject crosses the
  process boundary.
- Lua keeps the control/status heartbeat and UMG world-map geometry. Compact
  player movement no longer causes bridge-file writes.

## Bounded discovery

Normal discovery is lifecycle-driven. Actors already alive when F7 is pressed
need one catch-up path: after the player comes within 35 meters of a catalog
point, the provider performs at most one exact-class `FindAllOf` call for that
class during that F7 activation. There is one enumeration call site, no
UObjectArray walk, and no periodic rescan.

The install-time actor catalog is generated locally from matching
`SectionTreasureBoxData.xml` and `PropTreasureBoxData.xml` files extracted from
the installed game PAK. It currently resolves 1692 of 1693 treasure records to
11 exact generated classes. More than two unresolved joins fail installation.

All 9 boss and 40 Assault actor classes are joined from current-game
`MonsterCharacterData` blueprints during installation. A nearby actor must first
be positively observed, then be absent for two consecutive 250 ms probes before
the native provider publishes completion. All 49 records still receive the
one-shot F7 save baseline.

## Build and validation

Run from Windows PowerShell:

```powershell
.\tools\Build-Core.ps1
.\build\Compile-Source.ps1
.\build\Test-Refactor.ps1
.\build\Verify-Source.ps1
.\tools\Build-Native.ps1 -UE4SSRoot ..\DragonSwordNativeAutoPickup\.sdk\RE-UE4SS
.\build\Build-Release.ps1
```

The native build uses the pinned UE4SS toolchain already maintained beside
DragonSwordNativeAutoPickup. It produces `build-native/main.dll`. The release
build stages it as `dlls/main.dll` and creates
`dist/DragonSwordWorldRadarObjectState-v0.5.0-dev2-native-presence.zip`.

## Runtime acceptance

Before replacing the production radar, verify all of the following in game:

1. Install-generated `treasure-actors.tsv` contains the expected count and 11
   exact classes.
2. F7 performs one `F7_SAVE_SYNC_PERF` operation and never performs another
   SQL snapshot during that activation.
3. Compact position remains smooth and Motion Bridge writes fall to heartbeat,
   setting, status, or map-geometry changes only.
4. Opening a nearby treasure hides it immediately and it stays hidden after
   the F7 SQL worker completes.
5. F8 stops native position/object counters; the next F7 increments the native
   activation once and performs one new save reconciliation.
6. Dungeon exit, teleport, channel change, death, and repeated large-map entry
   do not publish events from an older epoch and do not crash.
7. Compare F7 and F8 frametime/FPS over equal routes. No performance claim is
   accepted from static gates alone.

Default pre-release settings keep `debug_logging=true`,
`diagnostic_verbose=false`, `high_resolution_timer=true`, and the visible
world-map presentation interval at 8 ms.
