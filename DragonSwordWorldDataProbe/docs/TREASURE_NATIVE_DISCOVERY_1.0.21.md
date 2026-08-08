# Treasure native-status discovery — 1.0.21

## Objective

Find a game-native, read-only query equivalent to:

`treasure save_id -> opened / unopened`

without replacing the current `tb_treasure_box` implementation prematurely.

## Existing ground truth

The current Radar reads:

`tb_treasure_box(CATEGORY, OPENED_BIT_FIELD)`

with:

- `category = save_id // 64`
- `bit = save_id % 64`
- opened when that bit is set.

The bundled static catalog contains 1693 treasure IDs.

## Discovery stages

### Runtime

`treasure_runtime_discovery.lua` inventories `DETUtil` functions and their
parameter/return signatures. It never calls an unknown candidate.

### Post-exit ObjectDump

`treasure_discovery` parses the full `UE4SS_ObjectDump.txt` for Treasure,
TreasureBox, Chest, Opened, Clear, Collect, and InStandAlone functions/classes.

Candidates are ranked using:

- Treasure-specific naming;
- query-shaped prefix (`CIs`, `Is`, `Has`, `Can`, `Check`, `Get`);
- bool return;
- WorldContextObject;
- one stable numeric ID parameter;
- mutation/event penalties.

### Validation evidence

The module also collects, when present:

- current WorldRadar bridge points as one-sided unopened samples;
- WorldRadar debug `newlyOpened` bit deltas as authoritative opened samples.

Absence from the bridge is never treated as opened.

## Acceptance criteria

A native helper is accepted only if:

1. it is a read-only bool query;
2. its ID maps stably to treasure `save_id`;
3. it matches known opened IDs;
4. it matches known unopened IDs;
5. it remains correct after game restart;
6. it does not require opening task/UI pages.

`treasure_candidate_validator.lua` is included but disabled in 1.0.21.
It can only be enabled after an exact candidate and both ground-truth classes
are available.
