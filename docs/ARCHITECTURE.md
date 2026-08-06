# Architecture

## Runtime processes

DragonSwordWorldRadar uses UE4SS Lua as the game-side producer, a hidden WScript watcher, a transient Windows PowerShell 5.1 host, and an in-memory compiled WinForms overlay. No custom radar executable is built or distributed.

```text
UE4SS Lua
  -> alternating static JSON slots (250 ms)
  -> alternating compact motion slots (24 ms sampling; visual-delta writes)
  -> hidden watcher / transient host
  -> in-memory WinForms overlay
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

## Scheduler

- UE4SS motion sampling: 24 ms in minimap and world-map modes.
- Motion publication: only after a visible delta or a 1000 ms heartbeat.
- Static state publication: 250 ms.
- Overlay polling: 24 ms active, 50 ms idle world map, 75 ms idle minimap, 125 ms disabled.
- Heartbeat-only and visually equivalent states update liveness without invalidating the window.

## Rendering

World-map treasures are projected once per paint into a reusable buffer with projected-pixel deduplication. Non-nearest treasure markers are grouped into four retained `GraphicsPath` batches. Boss markers, nearest treasure, labels, and height indicators retain independent rendering paths.

## Runtime directories

```text
runtime/
  bridge/       transient Lua-to-overlay state
  logs/         installer, watcher, host, Lua, and overlay logs
  diagnostics/  collected snapshots when diagnostics are requested
  launch.request
  reinstall-required.json
```
