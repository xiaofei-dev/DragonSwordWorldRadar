# Architecture

## Runtime processes

DragonSwordWorldRadar uses UE4SS Lua as the game-side producer, a resident hidden WScript watcher, a game-bound hidden Windows PowerShell 5.1 host, and an in-memory compiled WinForms Overlay. No custom radar executable is built or distributed.

```text
UE4SS Lua
  -> alternating static JSON slots (250 ms)
  -> alternating compact motion slots (24 ms sampling; visual-delta writes)
  -> resident hidden WScript watcher / game-bound hidden host
  -> in-memory WinForms Overlay
```

## Data and state boundaries

```text
TreasureDataProvider -> data/generated/treasures.lua
                     -> SQLCipher opened-state filter
                     -> per-map visibility index
                     -> minimap/world-map renderer

BossDataProvider     -> data/generated/bosses.lua
                     -> SQLCipher tb_actor_respawn state
                     -> availability deadline cache
                     -> minimap/world-map renderer
```

Treasure and Boss save state are read from consistent copies of active `.db`/`.bak` files and available WAL/SHM/journal sidecars. The production runtime does not enumerate `Character` objects and does not install experimental gameplay hooks.

## Scheduler and map lifecycle

- Radar motion sampling: 24 ms, with no world-map UObject read in that loop.
- World-map entry detection: the existing 250 ms static-state producer.
- Active world-map transform sampling: 24 ms only while the map is open.
- Map exit: the first missing active-map read clears the map transform, publishes radar motion, and terminates the world-map loop.
- Motion publication: only after a visible delta or a 1000 ms heartbeat.
- Static state publication: 250 ms.
- Overlay polling: 24 ms active, 50 ms idle world map, 75 ms idle radar, 125 ms disabled, and 500 ms while backgrounded.
- Heartbeat-only and visually equivalent states update liveness without invalidating the window.
- Failed Lua game-thread queues/callbacks clear their pending gates so later iterations can recover.

## Window lifecycle

Radar and world-map modes use different actual window geometries:

- Radar: a small top-right layered window.
- World map: a game-client-sized layered window only while the map is active.

The Overlay hides before either geometry change, resizes with `SWP_NOCOPYBITS`, synchronously repaints the new hidden surface, and then uses `SW_SHOWNOACTIVATE` in the same Overlay timer cycle. There is no timed warm-up. This avoids copying stale layered-window pixels while still avoiding an invisible full-client composition surface during ordinary radar use.

## Rendering

World-map treasures are projected once per paint into a reusable buffer with projected-pixel deduplication. Non-nearest treasure markers are grouped into four retained `GraphicsPath` batches. Boss markers, nearest treasure, labels, and height indicators retain independent rendering paths.

## Diagnostics

Normal mode writes only low-volume Use logs. Detailed producer timing, bridge rates, timer/paint pacing, CPU/memory, save/Boss state, and geometry are computed and written only when `debug_logging = true`.

```text
runtime/logs/
  DragonSwordWorldRadar.Lua.Use.log
  DragonSwordWorldRadar.Lua.Debug.log
  DragonSwordWorldRadar.Overlay.Use.log
  DragonSwordWorldRadar.Overlay.Debug.log
  watcher / host / installer operational logs
```

`overlayPaintFps` is an Overlay paint-rate metric, not the game's Present FPS.

## Runtime directories

```text
runtime/
  bridge/       transient Lua-to-Overlay state
  logs/         installer, watcher, host, Lua, and Overlay logs
  diagnostics/  collected snapshots when diagnostics are requested
  launch.request  # digits-only session stamp; no trailing newline
  reinstall-required.json
```
