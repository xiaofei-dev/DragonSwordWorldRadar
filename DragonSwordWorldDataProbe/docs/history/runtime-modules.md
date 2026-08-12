# Runtime Module Inventory

## Active research module

### `assault_target_presence_pair`

- Source: `scripts/modules/assault_target_presence_pair.lua`
- Status: active diagnostic research.
- Targets: PlaceID 104 / CID 143 and PlaceID 109 / CID 148.
- Method: `FindAllOf` for exactly `DsMon_Skeleton_leader_Named02_C` and `DsMon_Hound_Named02_C`, followed by UIDName, CID, and static-coordinate identity checks.
- Environment data: `DGameSingleton.TimeOfDay`, `LoginStartTimeOfDay`, environment weather state, custom-time scalars, and teleport state.
- Reports: `assault-special-spawn-v4-current.tsv` and `assault-special-spawn-v4-history.tsv`.
- Boundary: absence means only `not_observed_in_loaded_object_set`.

## Assault modules

| Module | Purpose | Historical status |
|---|---|---|
| `assault_runtime_table` | Capture runtime UnexpectedMission Place, Kind, condition, schema, access, function, and active-layer evidence. | Preserved; not in the active order. |
| `assault_condition_transition` | Poll the known `CUnexpectedMissionInStandAlone` query in bounded batches. | Archived after the bool was confirmed as player-in-zone trigger context rather than global availability. |
| `assault_condition_correlation` | Correlate archived condition observations with known target metadata. | Preserved analysis support. |
| `assault_environment_snapshot` | Capture bounded environment properties discovered during Assault research. | Preserved; superseded by the current focused sampler. |
| `assault_quest_trigger_correlation` | Inventory known Quest, Trigger, registration, and subsystem properties/functions without invoking unknown functions. | Preserved experimental evidence; not the primary availability route. |

## Treasure modules

| Module | Purpose | Historical status |
|---|---|---|
| `treasure_runtime_discovery` | Discover Treasure-related owner classes and function names. | Archived discovery. |
| `treasure_candidate_validator` | Validate explicit opened and unopened ground-truth candidates. | Fail-closed; disabled unless both truth sets are supplied. |
| `treasure_actor_state_chain` | Inventory TreasureBox, Prop, and interaction state signatures. | Preserved diagnostic implementation. |
| `treasure_blueprint_catalog` | Read `DPropDataTable` TreasureBox metadata and correlate generated classes. | Preserved static/runtime bridge research. |
| `treasure_actor_snapshot` | Match targeted `DsAnimationProp` observations to the static catalog. | Superseded diagnostic route. |
| `treasure_state_hooks` | Observe explicitly selected Treasure/Prop interaction calls without invoking them. | Historical only; lifecycle hooks remain prohibited. |
| `treasure_real_actor_proximity` | Use nearby static points to enumerate only required exact TreasureBox classes and track presence, repeated absence, and reappearance. | Archived and disabled because continuous Lua enumeration did not meet the performance target. |

## Mole module

`mole_completion_state` uses the known read-only `CIsClearMiniGameInStandAlone` helper with a canary followed by a bounded bulk pass. Mole research is complete and frozen; the module is not in the active order.

## Generic framework modules

- `healthcheck`: framework scheduling and file-write health.
- `class_presence`: configured exact-class presence checks.
- `property_snapshot`: configured scalar property reads.
- `bounded_object_snapshot`: bounded snapshots with explicit class and field lists.
- `ui_snapshot`: bounded UI object snapshots.
- `targeted_hook`: explicit allowlisted observation hooks; `ReceiveDestroyed` and `ReceiveEndPlay` are denied.

These generic modules are extension primitives. Empty configuration is intentional and must remain fail-closed.
