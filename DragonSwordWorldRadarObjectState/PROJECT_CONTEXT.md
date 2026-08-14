# DragonSwordWorldRadarObjectState Project Context

## Scope

This directory is the experimental native-object-state successor to
`../DragonSwordWorldRadar`. The production directory is frozen and must not be
modified by work performed here. Never modify DragonSwordNativeAutoPickup,
DragonSwordWorldDataProbe, or other Mods as part of this project.

Version: `0.5.0-dev2-native-presence`

Mod folder: `DragonSwordWorldRadarObjectState`

Deployment, game launch/termination, injection, commit, and push require
explicit user authorization. Preserve unrelated dirty work in the parent Git
repository.

## Approved objective

Remove periodic save-state collection and move straightforward high-frequency
state work to C++ without adding theoretical or compensating overhead:

- native 33 ms fresh player coordinates;
- lifecycle-safe treasure disappearance tracking;
- install-generated exact-class disappearance tracking for 9 bosses and 40 Assault targets;
- fixed-size numeric shared memory;
- exactly one save reconciliation per F7 activation;
- no runtime save fingerprint, copy, or SQL scheduler;
- no compact player-motion filesystem writes;
- F8 stops native collection and invalidates pending save publication.

The external Overlay and existing Lua world-map geometry remain for this
iteration. Moving drawing into PostRender/Canvas is a separate architecture
stage.

## Safety invariants

- No UObjectArray enumeration.
- One exact-class `FindAllOf` catch-up call site; every class is scanned at most
  once per F7 activation and only after a nearby catalog point is reached.
- No UObject pointer crosses callbacks or IPC. Observed actors use weak index
  and serial identity only.
- Travel increments the epoch, clears observations, and publishes invalid
  position until the cooldown and fresh chain read succeed.
- Only `Destroyed` EndPlay near the player can produce a completion event.
- F7 SQL failure is terminal for that activation; there is no automatic retry
  loop.
- Current-activation native deltas win over a returning baseline snapshot.
- Missing temporary map geometry is not an object-state error.

## Evidence boundary

The exact treasure class catalog is generated from the user's installed game
data. Current reference evidence resolves 1692 records across 11 classes, with
one known unmatched record.

Native encounter completion is enabled only for IDs 104, 109, and 120 because
their generated class, identity, and position are jointly evidenced. The
remaining 46 encounter records retain F7 save-baseline behavior.

## Acceptance boundary

Static and build gates prove source structure, managed compilation, C++ core
logic, UE4SS ABI compilation, install-time joins, protocol decoding, and
package integrity. They do not prove in-game smoothness, correct actor EndPlay
semantics, or travel stability. Runtime A/B evidence remains mandatory before
this project may replace DragonSwordWorldRadar.
