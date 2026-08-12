# External Module Inventory

External modules run through `tools/ModuleHost.ps1`. Heavy work is normally deferred until the game exits.

## Enabled post-exit modules

| Module | Output or purpose |
|---|---|
| `static_inventory` | Game layout and build fingerprint. |
| `transition_capture_analysis` | Summaries of preserved Treasure transitions and Assault bool transitions. |
| `assault_respawn_cycle_fast` | Exact extraction of `RespawnCycleData`, especially cycle 105. |
| `assault_reveal_cycle_fast` | Exact extraction of `RevealCycleData`, including reveal and hide game-time windows. |
| `assault_quest_condition_analysis` | ObjectDump evidence for Quest, Trigger, weather, DaySwitch, and Respawn relationships. |

## Manual or disabled research modules

| Module | Historical role | Current status |
|---|---|---|
| `pak_static` | Proven PakReaderCore and Oodle static extraction. | Manual verification only. |
| `assault_catalog` | Join UnexpectedMission Place, Kind, SectionMonster, and support tables. | Completed and preserved. |
| `assault_kind_place_fast` | Exact two-file Kind/Place recovery fallback. | Manual only. |
| `pak_decoder_evidence` | Preserve decoder offsets, raw bytes, failures, and implementation evidence. | Superseded by integrated extraction evidence. |
| `save_snapshot` | Copy DB, WAL, SHM, BAK, and SAV files after exit without decryption. | Optional and disabled by default. |
| `object_dump` | Extract targeted lines from an existing UE4SS ObjectDump. | Optional and disabled by default. |
| `mole_game_discovery` | Static Mole ID and PAK discovery. | Frozen. |
| `mole_state_discovery` | Mole completion-state analysis. | Frozen. |
| `treasure_discovery` | Broad Treasure and ObjectDump discovery. | Archived. |
| `treasure_blueprint_analysis` | Treasure Blueprint class and signature analysis. | Archived. |
| `treasure_static_fast` | Exact Treasure static extraction. | Archived. |
| `treasure_static_catalog` | Build the static Treasure identity catalog. | Archived. |

## Execution guarantees

- Module definitions are authoritative in `config/modules.json`.
- Trigger eligibility, timeout, ordering, and risk labels are explicit.
- A named mutex prevents overlapping ModuleHost runs.
- Each run has isolated context, progress, stdout, stderr, and `result.json` files.
- Failed or partial extraction remains failed or partial; it is never promoted silently.
