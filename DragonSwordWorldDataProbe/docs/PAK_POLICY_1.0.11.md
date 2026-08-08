# 1.0.11 PAK policy

`pak_static` is no longer a normal collection dependency.

Normal assault path:
1. In-game `assault_runtime_table` snapshots Place/Kind from `DUnexpectedMissionTable`.
2. After game exit, `static_inventory`, `assault_catalog`, and `mole_state_discovery` run.
3. Diagnostics package those results.

Optional PAK verification:
- run `Run-Pak-Verification.cmd` only after the game is closed;
- it has a hard 180-second child-process timeout;
- failure/timeout does not block the production assault route.
