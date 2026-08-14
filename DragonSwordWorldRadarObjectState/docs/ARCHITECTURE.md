# Architecture

## Components

### Native UE4SS provider

`src/native/main.cpp` owns fresh player coordinates, F7/F8 native activation,
travel epochs, actor lifecycle observation, bounded catch-up discovery, and the
shared-memory publisher. `include/dswros/object_state.hpp` contains the
engine-independent matching and disappearance policy.

The provider uses EngineTick for a 33 ms fresh current
Engine/Viewport/GameInstance/LocalPlayer/Controller/Pawn chain read. It never
retains any object returned by that chain. BeginPlay and EndPlay callbacks are
executed on the game thread. Actor observations retain only `FWeakObjectPtr`
index/serial values, a catalog ID, a catalog position, activation, and epoch.

### Shared-memory bridge

The per-process mapping is
`Local\DragonSwordWorldRadarObjectState.NativeState.<gamePid>`. Protocol 1 is a
5248-byte packed structure protected by a 32-bit seqlock. The header contains
flags, activation, epoch, position, monotonic sample identity, and counters. A
128-entry ring contains numeric treasure-opened and encounter-defeated events.

`NativeStateBridgeReader.cs` retries mapping attachment at most once per second,
accepts a frame only when both seqlock reads match and are even, and detects
event-ring overflow. No steady-state process enumeration or allocation is
required for coordinate reads.

### Lua control and map geometry

Lua retains proven lifecycle controls, settings, status, and UMG world-map
geometry. Protocol-v6 alternating files remain because the external Overlay
still needs these values. Compact player movement is excluded from the file
bridge change predicate; native shared memory is authoritative for movement.

### Overlay

The WinForms Overlay reads native state before consuming a new Lua frame. It
overlays native coordinates on the Lua control frame and feeds the established
bounded predictor. Native lifecycle events update immutable visibility and
encounter dictionaries immediately.

`TreasureSaveState` queues one worker per native F7 activation. There is no
time-based change detector, fingerprint scheduler, or completion cooldown. The
request is consumed before process/key/database work begins, so every failure
is bounded to one attempt. A later native event is merged while publishing the
worker result, eliminating stale-snapshot resurrection.

### Installer

The installer extracts the section and blueprint treasure tables from the same
installed PAK generation. `TreasureActorCatalogGenerator` joins Section CID to
Prop ID, derives the exact generated class name, validates the 11 supported
classes, and writes `data/generated/treasure-actors.tsv`. Extracted XML is
temporary and is not shipped.

## Lifecycle

1. F7 increments native activation and epoch, clears weak observations, and
   publishes enabled-with-invalid-position.
2. After a 750 ms stability delay, the native provider samples a fresh player
   chain and begins proximity-triggered class catch-up.
3. Overlay observes the activation and consumes one save reconciliation
   request.
4. BeginPlay/catch-up associates exact class and position with catalog IDs.
5. Nearby `Destroyed` EndPlay publishes a numeric completion event.
6. Travel begins by incrementing epoch, clearing observations, and invalidating
   position. Work resumes only after the 1500 ms post-transition cooldown.
7. F8 clears observations, increments epoch, disables position publication,
   and invalidates any save worker result.

## Deliberate limitations

- Drawing is still external and therefore still has transparent-window/DWM
  cost.
- The existing Lua control chain still samples at 250 ms for established map
  and lifecycle logic, although it no longer supplies compact movement.
- Installation joins all 9 boss and 40 Assault CIDs to exact current-game
  generated blueprint classes. Runtime completion requires positive nearby
  observation followed by two consecutive missing probes.
- Exact-class catch-up still invokes UE4SS `FindAllOf` once per nearby class and
  activation. Runtime timings must confirm this produces no visible hitch.
