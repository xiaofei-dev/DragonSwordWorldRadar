# Production Migration Record

DragonSwordWorldDataProbe is a research and collection project. DragonSwordWorldRadar owns production behavior. A clean observation is not automatically a production acceptance decision.

## Boss

Status: production accepted and frozen.

- Static identity and coordinates come from the nine-boss generated catalog.
- Dynamic availability comes from `tb_actor_respawn`.
- The production Radar applies the verified Boss reset rule and caches save snapshots.
- DataProbe must not replace or expand this path without new explicit scope.

## Treasure

Status: static identity and save truth are production accepted; Actor inference is archived research.

- The Radar generates 1,693 Treasure records from current game data.
- Opened state comes from SQLCipher `tb_treasure_box` category bit fields.
- DataProbe established exact TreasureBox generated classes and a bounded proximity presence/absence method.
- Continuous Lua Actor enumeration caused unacceptable performance impact, so the proximity module was archived rather than migrated into production.

## Assault

Status: research collection is closed and the bounded production model is ready for implementation. Gameplay validation remains required after implementation.

Accepted evidence already ported into the Radar research branch:

- The existing save snapshot query includes all 40 Assault target CIDs in addition to the nine Boss IDs.
- `TreasureSaveState` logs the first observed and changed `tb_actor_respawn` state for each Assault CID.
- A controlled test confirmed that completing CID 175 changed `rowPresent=false` to a row with an exact death time and respawn type 2.

Production model:

```text
40-target static identity
+ persisted tb_actor_respawn death/respawn state
+ CID 143 inferred RevealCycle 10001 schedule (23:00-06:00)
= bounded renderable Assault availability
```

`tb_actor_respawn` is confirmed by a controlled CID 175 completion and supplies the dynamic defeated/respawn path. An absent row does not independently prove special spawn eligibility, so CID 143 additionally uses the confirmed row-10001 schedule. The CID-to-row mapping must retain an inferred evidence label because its direct generator key was not captured.

Weather is not part of the production model. The recorded weather scalars did not change across the CID 143 reveal transition, their visible-weather mapping is unknown, and no target-specific weather condition was found.

Recommended production acceptance test:

- ordinary target visible when no active `tb_actor_respawn` suppression exists;
- completed target suppressed according to the existing save-state policy;
- CID 143 hidden before 23:00, visible after 23:00, and hidden after 06:00;
- no runtime Actor or `MonsterSpawnBase` enumeration.

## Rejected migrations

- `CUnexpectedMissionInStandAlone` polling: confirmed as player-in-zone trigger context, not global availability.
- Global Character or Actor scanning: cannot cover unloaded regions and violates the performance boundary.
- Treasure lifecycle hooks: `ReceiveDestroyed` and `ReceiveEndPlay` caused native crashes and are prohibited.
- Unknown function invocation or environment container enumeration: prohibited by the fail-closed policy.
