# DragonSwordWorldDataProbe Project Context

## Purpose of this project

DragonSwordWorldDataProbe is a modular research and diagnostics framework for DragonSword Awakening. It gathers build-pinned static data, bounded runtime observations, and post-exit evidence used to understand treasures, Assault/UnexpectedMission content, and other game systems.

It is not the production Radar. Findings from this project must be validated before they are promoted into `DragonSwordWorldRadar` behavior.

Use this file as the first orientation document when continuing the project in a new chat.

## Current identity and research focus

- Framework version: `1.0.42`
- State schema: `142`
- Package lineage: `DragonSwordWorldDataProbe-1.0.42-assault-time-weather-teleport.zip`
- `enabled.txt`: the runtime framework runs only when this contains `1`
- Active Lua probe order in `scripts/config.lua`:
  1. `assault_target_presence_pair`

Treasure research is archived as of 1.0.38. The exact catalog, implementation, historical reports, and confirmed presence-to-absence observation remain preserved, but the Treasure runtime module is disabled and removed from the automatic probe order. No Treasure Actor enumeration runs in the active configuration.

The Assault trigger sampler remains archived. Version 1.0.40 runs one bounded comparison every 10 seconds between PlaceID 104 (conditioned target) and PlaceID 126 (control target). It enumerates only their two exact generated classes and never performs a global Character or Actor scan. Absence is recorded only as not observed in the loaded object set. Boss research remains frozen and must not be expanded.

## Architecture

The project has two execution layers:

### UE4SS Lua runtime layer

- Entry point: `scripts/main.lua`
- Configuration: `scripts/config.lua`
- Modules: `scripts/modules/`
- Core scheduling, checkpoint, logging, quarantine, and value encoding: `scripts/core/`

Runtime modules execute through a checkpointed probe manager. Incomplete or unsafe steps are recoverable or quarantined rather than silently treated as success.

### External PowerShell analysis layer

- Monitor entry point: `Start-Monitor.cmd` / `tools/Start-Monitor.ps1`
- Module orchestrator: `tools/ModuleHost.ps1`
- Definitions: `config/modules.json`
- Active profile: selected by `config/suite.json`
- Module implementations: `tools/modules/<module-id>/Run.ps1`
- Diagnostics entry point: `Collect-Diagnostics.cmd`

Heavy external collection is deferred until the game exits. The module host uses a named mutex, ordered module definitions, trigger filtering, progress files, isolated run directories, and structured `result.json` outputs.

Supported triggers are `monitor_start`, `game_exit`, `collect`, and `manual`.

## Established findings

- Boss research is complete and unchanged. Production Boss availability is already handled by the Radar using a static catalog plus `tb_actor_respawn`.
- Assault content uses separate UnexpectedMission World, Place, and Kind static tables.
- Forty Assault targets have exact CID and UIDName joins to unique actors in preserved evidence.
- Current Kind rows use `DEFEAT_MONSTER` / `MONSTER_ALIVE`; exact actors use RespawnCycleID 105, `ServerDeathCheck=true`, and `SpawnConditionID=0`.
- RespawnCycleID 105 parses as `DAILY` with `RespawnRealTime=0`; this alone does not prove complete availability semantics.
- Full-map runtime actor scanning cannot represent unloaded world-partition regions.
- The historical `CUnexpectedMissionInStandAlone` false/false observations do not represent a persistent completion flag.
- Treasure database bits remain the authoritative validation ground truth while actor-presence inference remains diagnostic-only.

## Safety and evidence boundaries

The following rules are deliberate and must remain fail-closed:

- Do not mutate game state.
- Do not run a global `Character` or global Actor scan.
- Do not invoke unknown discovered functions.
- Do not automatically call mutation-shaped or event-shaped candidates.
- Do not use `ReceiveDestroyed` or `ReceiveEndPlay`; they caused repeatable native crashes and are not reliable kill signals.
- Do not treat adjacent encoded-entry recovery as valid; it produced false positives.
- Do not claim weather, switch, cooldown, completion, or availability semantics without an observed and reproducible correlation.
- Do not replace the Radar's proven Boss/save tracker with experimental DataProbe code.
- Run manual PAK verification only while the game is closed; it is optional and has a hard timeout.

## Preservation policy

This project intentionally preserves historical research:

- Completed or superseded modules remain disabled or manual-only.
- Dangerous or buggy implementations are retained as non-executable `.disabled.txt` snapshots.
- Build-pinned extracted reference data remains read-only.
- New research routes should normally be added as separate modules rather than replacing earlier evidence.
- Archived modules must never be auto-loaded by `scripts/main.lua` or `ModuleHost.ps1`.

The presence of a module in the tree does not mean it is active, safe, current, or validated. Always check `scripts/config.lua`, `config/suite.json`, the active profile, and `config/modules.json` before describing the execution path.

## Diagnostics workflow

`Collect-Diagnostics.cmd` may be started while the game is running. The expected workflow is:

1. Wait for all game processes to exit.
2. Wait for the automatic monitor to finish, or use the bounded fallback grace path.
3. Run or reuse eligible post-exit modules.
4. Package the report.
5. Verify the resulting ZIP and print its path.

Do not launch the diagnostics command twice. Progress is recorded under `runtime/reports/`, `runtime/monitor/`, and the current `runtime/runs/<run-id>/` directory.

## Important files

- `README.txt`: full version history, usage, and current collection procedure
- `scripts/config.lua`: authoritative active runtime probe order and safety flags
- `scripts/main.lua`: runtime bootstrap and probe-manager integration
- `config/suite.json`: active external profile and monitor policy
- `config/profiles/`: allowed modules and research-stage policy
- `config/modules.json`: external module definitions, ordering, and triggers
- `metadata/release.json`: current release contract and historical change record
- `metadata/validation.json`: explicit gates and prohibited behavior
- `metadata/preservation-policy.json`: append/preserve rules for historical work
- `metadata/known-findings.json`: concise known findings and remaining validation
- `metadata/method-matrix.json`: approved, optional, experimental, and prohibited methods
- `docs/DECISION_LOG.md`: durable research decisions

Some older documentation files contain mojibake from an earlier encoding problem. Prefer the current source, JSON metadata, `README.txt`, and this context file when the corrupted text is ambiguous; do not silently reinterpret damaged text as authoritative evidence.

## Current limitations and next evidence

The project still lacks a production-quality global availability model for Assault events. Weather, switch, task-state, and RespawnCycle evidence are incomplete as a combined semantic model. Treasure actor proximity inference also remains diagnostic and must not replace save-database truth yet.

The most useful next evidence is a controlled before/during/after capture while the user completes one relevant Assault task, followed by the normal wait-for-game-exit diagnostics workflow. Any semantic conclusion should bind the exact game-build fingerprint and preserve the raw reports used to reach it.

## Safe continuation checklist

1. Read `README.txt`, this file, `scripts/config.lua`, and the active external profile.
2. Confirm the exact game-build fingerprint before reusing static or runtime evidence.
3. Inspect current reports before changing a probe.
4. Keep runtime calls bounded, read-only, and explicitly whitelisted.
5. Add new research as a separate module when practical.
6. Preserve earlier modules and artifacts rather than deleting or rewriting them.
7. Distinguish observation, inference, and production-accepted behavior in every summary.
