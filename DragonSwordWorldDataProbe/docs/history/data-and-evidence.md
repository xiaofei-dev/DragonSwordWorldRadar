# Data and Evidence Inventory

## Assault data

- `reference/assault-runtime-targets/assault-targets.tsv` and `.json`: 40 exact Place, Kind, CID, UID, UIDName, coordinate, respawn, and Section joins.
- `UnexpectedMissionPlaceData.tsv` and `UnexpectedMissionKindData.tsv`: preserved source rows for mission definitions.
- `assault-condition-evidence.json`: exact `MONSTER_ALIVE` linkage evidence.
- `respawn-cycle-105-proven.json`: cycle 105 is statically confirmed as `DAILY`; its exact clock reset boundary remains unspecified.
- `reveal-cycle-static.json`: deterministic extraction provenance for all five RevealCycle rows and the explicitly inferred CID 143 mapping.
- `reference/assault-support/xml/`: SectionMonster, SpawnMonsterGroup, ActorSpawnCondition, NPCSpawnCondition, MonsterCharacter, ActorPosition, and related extracted tables.

Confirmed current facts:

- All 40 missions use `DEFEAT_MONSTER` and `MONSTER_ALIVE`.
- Each condition names one unique target UIDName.
- All 40 targets use RespawnCycleID 105 and `ServerDeathCheck=true`.
- All 40 direct `SpawnConditionID` values are zero.
- Raw SectionMonster GroupID is nonzero for 38 targets and zero only for CID 143 and CID 148.
- A controlled completion created a `tb_actor_respawn` row for CID 175 with the observed death time and respawn type 2.
- CID 143 was absent at 22:51:13 and present at 23:01:46 while the recorded weather state remained unchanged.
- `MonsterSpawnBase.TableKey_RevealCycle` and `RevealCycleData.RevealIngametime/HideIngametime` are the primary direct-condition candidates.
- A controlled CID 175 completion created a `tb_actor_respawn` row with the observed death time and respawn type 2, confirming the production death/respawn state source.
- The unique `/RevealCycleData.xml` entry is deterministically decoded from the 12-byte XOR record at encoded offset 11188. The validated payload is 754 bytes with SHA-256 `F323F0A09DA370CF9B22F0589376F18D969261B562938376085EC6CCD706CF87`.
- RevealCycle row 10001 is statically confirmed as a cemetery-skeleton schedule from hour 23 through hour 6.
- CID 143 to row 10001 remains a strong inference from identity plus the observed 23:00 reveal boundary; no direct `TableKey_RevealCycle` binding was captured.

Unknown current facts:

- The exact DAILY reset boundary for Assault targets. This is not a blocker for initial production integration using the persisted `tb_actor_respawn` row state.
- The direct static generator-asset binding from CID 143 to RevealCycle row 10001.
- Whether environment weather enums are global or region-local across every teleport.
- The runtime hide transition at 06:00 for CID 143. The hide hour is statically confirmed, but the transition was not directly observed.

## Treasure data

- `reference/treasure-real-actors/`: 1,692 exact Section-to-Prop joins, one unresolved unlock-like record, eleven generated TreasureBox classes, and canonical 64-bit UID strings.
- `reference/treasure/`: database and static mapping evidence retained from earlier phases.
- Historical runtime reports preserve exact-class presence, absence confirmation, and reappearance observations.

The encrypted save database `tb_treasure_box` remains authoritative ground truth. Actor presence is diagnostic evidence only and is not production accepted as a replacement.

## Boss and Mole data

- `metadata/world-boss-catalog.json`: frozen nine-boss catalog used as research provenance.
- `metadata/boss-monitoring-status.json`: ownership and freeze status.
- Mole static and state evidence remains in the restored external modules and legacy snapshots.

## Evidence locations

- `runtime/reports/`: current and historical runtime reports.
- `runtime/logs/`: structured probe observations and changes.
- `runtime/runs/`: isolated external-module runs.
- `metadata/known-findings.json`: concise current findings.
- `metadata/method-matrix.json`: accepted, diagnostic, experimental, archived, and prohibited methods.
- `metadata/preservation-policy.json`: preservation contract.
- `archive/legacy-snapshots/`: non-executable historical implementations.

Runtime files are evidence artifacts and are not part of the source manifest.
