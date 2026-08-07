# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar framework for **DragonSword Awakening**. Version `0.4.0-dev9-performance1.3-mapinstant-hiddenhost1` keeps the treasure and nine-world-boss behavior of the accepted `performance1.1` baseline while removing the later always-fullscreen world-map regression and separating normal-use diagnostics from development diagnostics.

## Map lifecycle and performance

- The compact motion protocol remains exactly 21 fields. Minimap movement is sampled every 24 ms without reading world-map UObjects.
- World-map entry is detected by the 250 ms static-state update. Only while the map is active does a dedicated 24 ms transform producer run.
- Closing the map is accepted on the first missing active-map read. The map producer terminates, stale map transforms are cleared, and a radar motion frame is published without the former three-sample exit delay or 250 ms stale re-entry block.
- Radar mode uses an actual small top-right layered window. It no longer keeps a transparent game-client-sized surface alive behind the radar.
- On every radar/world-map transition, the Overlay is hidden before resizing. The resize discards stale backing bits, the hidden surface is repainted synchronously, and the correct geometry is shown immediately without activation. There is no timed reveal delay.
- The active world-map transform already sampled by the motion producer is reused by the static publisher instead of traversing the same UObject chain twice.
- Compact motion files are written only after a visible delta or a 1000 ms liveness heartbeat. Static marker/configuration state remains double-buffered at 250 ms.
- Overlay refresh remains adaptive: 24 ms during visual motion, 50 ms for an idle world map, 75 ms for an idle radar, 125 ms while disabled, and 500 ms while the game is backgrounded.
- Heartbeat-only motion frames and visually equivalent static states update liveness without invalidating or repainting the form.
- Non-nearest treasure markers remain grouped into retained type-specific `GraphicsPath` batches. Game lifetime, geometry, and save polling reuse the tracked game PID.
- Boss cooldown calculation uses the verified fixed daily 09:00 local reset. The Overlay performs no runtime PAK enumeration or rule rescanning.

The bridge schema, generated-data schema, save-table interpretation, marker colors, F7/F8 controls, height behavior, and Boss filtering semantics are unchanged.

## Use and debug logs

Configure logging in `scripts/config.lua`:

```lua
use_logging = true,
debug_logging = false,
diagnostic_perf_interval_seconds = 5,
```

Normal play should keep `debug_logging = false`. Normal-use logs contain startup/shutdown, warnings, errors, and key state changes only; periodic performance counters and detailed save/geometry messages are not produced.

| Purpose | Path |
|---|---|
| Lua normal-use log | `runtime/logs/DragonSwordWorldRadar.Lua.Use.log` |
| Lua debug/performance log | `runtime/logs/DragonSwordWorldRadar.Lua.Debug.log` |
| Overlay normal-use/error log | `runtime/logs/DragonSwordWorldRadar.Overlay.Use.log` |
| Overlay debug/performance log | `runtime/logs/DragonSwordWorldRadar.Overlay.Debug.log` |

Debug mode records producer queue delay, update duration, 50/100/250 ms stall counts, sampling/write rates, Overlay timer and paint gaps, normalized Overlay/game-process CPU, working set, visibility, window geometry, and `MAP_SURFACE_PREPARED` timing for each hidden map/radar resize. `overlayPaintFps` is the WinForms Overlay paint rate, **not** the game's Present FPS. Use an external frame-rate tool for authoritative game FPS comparison.

`Collect-Diagnostics.cmd` collects both log sets, bridge snapshots, metadata, generated data, UE4SS log tail, process data, and source hashes.

## Implemented layers

### Treasures

- Minimap and world-map markers generated from the local game PAK.
- XYZ nearest-treasure selection, semantic type colors, height pointer, overrides, aliases, and SQLCipher save filtering.
- Player-height reference offset is `-150`.

### World bosses

- Nine fixed world-boss locations generated from local game data.
- One enlarged marker modeled on the game's field-boss icon.
- Availability is derived from `tb_actor_respawn`, not streamed `Character` objects.
- Save snapshots include active `.db`/`.bak` files and WAL/SHM/journal sidecars.
- RespawnCycle rule 106 uses the verified daily 09:00 local reset directly; no runtime PAK scan is performed.

No `FindAllOf("Character")` scan or experimental HP/death hook is used.

## Reliability

- Installation compiles the exact complete Overlay source set under Windows PowerShell 5.1. A compile failure stops installation; the authoritative success marker is `OVERLAY_COMPILE_OK`.
- Lua game-thread queue and callback failures always release their pending gates, allowing the next producer iteration to recover.
- Static and motion bridges use alternating files and sequence validation. Invalid or partial frames never replace the last complete published frame.
- Save, catalog, visibility-index, Boss, diagnostics, timer, renderer, game-window, and game-lifetime failures are isolated and rate-limited.
- Catalog, override, and per-map visibility replacements publish only after complete consistency checks.
- A transient game-process API failure does not start the shutdown countdown for an already tracked PID.
- `Install.cmd` starts and validates a resident hidden WScript watcher and registers it for the current user’s next sign-in. Game-time Lua writes only a validated digits-only `runtime\launch.request` stamp with no trailing newline; it does not invoke a shell. Debug configuration is read once when the Overlay process starts; changing `debug_logging` requires restarting that Overlay process or the game.
- Runtime and patch-deployment backup folders are excluded from the source-only unexpected-EXE scan; the only bundled source tool remains the declared `ooz.exe` with SHA-256 verification.

## Installation

1. Install UE4SS.
2. Extract the complete release folder to `Mods/DragonSwordWorldRadar`.
3. Close the game and run `Install.cmd`. This recreates the hidden user Startup watcher required for shell-free game launch.
4. Confirm the installer prints `OVERLAY_COMPILE_OK` and `INSTALL_COMPLETE`.
5. Start the game normally.
6. Press **F7** to enable and **F8** to disable.

Do not run `Install.cmd` from inside a ZIP preview. The installer preserves a valid `scripts/config.lua`, `data/treasure_overrides.txt`, and unrelated entries in `mods.txt`.

## Validation status

This repaired source package received static source, JSON/XML, Lua syntax, delimiter, semantic-invariant, binary-hash, and package-integrity checks in a Linux preparation environment. Windows PowerShell 5.1 `Add-Type` compilation and in-game FPS/transition validation were not executable there. Run `Install.cmd` and require `OVERLAY_COMPILE_OK` before treating a deployment as valid.

## Game updates

Installation stores a fingerprint of the game executable and generated-data PAK in `metadata/install-state.json`. If the game changes, the watcher requests reinstallation instead of running stale datasets.

## Apply a patch, build, and deploy

Place one `.patch` or `.diff` file in the repository root and run:

```text
Apply-Patch-And-Deploy.cmd
```

The workflow requires a clean Git work tree, confirms the current and target versions, runs all validation gates, builds the release, lets you select `DSClient-Win64-Shipping.exe`, deploys the Mod, and runs the normal installer. Patch changes remain staged for review and commit. Logs are written under `runtime/patch-deploy`.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.
