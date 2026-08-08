# Architecture

## Runtime processes

DragonSwordWorldRadar uses UE4SS Lua as the game-side producer, a hidden per-session WScript watcher, a game-bound hidden Windows PowerShell 5.1 host, and an in-memory compiled WinForms Overlay. No custom radar executable is built or distributed.

```text
UE4SS Lua
  -> alternating protocol-v2 Motion/control slots
  -> hidden WScript watcher / exact-PID PowerShell host
  -> in-memory WinForms Overlay
```

## Single live IPC channel

`runtime/bridge/radar_motion_a.dat` and `radar_motion_b.dat` are the only active Lua-to-Overlay state channel. Each compact ASCII record contains 27 fields: protocol version, generation, sequence integrity, enabled/mode state, layer controls, text scale, player coordinates, radar radius, and active world-map transform data.

The old `radar_state*.json` files are cleanup-only. They are never read or written as active state. This removes duplicate state ownership, JSON serialization/parsing, the 200 ms Static poll, and static/motion fallback merging.

## Data ownership

```text
TreasureDataProvider -> data/generated/treasures.lua
                     -> Overlay catalog + save visibility index
                     -> minimap/world-map renderer

BossDataProvider     -> data/generated/bosses.lua (exactly 9)
                     -> Overlay catalog + tb_actor_respawn availability
                     -> minimap/world-map renderer
```

Lua no longer serializes fixed treasure or Boss catalogs. It publishes only live UObject/control data that the external Overlay cannot obtain itself.

## Scheduler

- Radar position sampling: 24 ms.
- Active world-map transform sampling: 24 ms, only while the map is open.
- Low-frequency Pawn/map-mode/radius control sampling: 250 ms.
- Motion heartbeat: 1000 ms.
- Overlay active timer: 24 ms; idle/disabled/background modes remain 50/75/125/500 ms.
- Motion stale timeout: 2500 ms.
- XY/Z publication thresholds: 20/10 game units.
- The textual Lua `LoopAsync` registration count remains three; no added pseudo-worker loop or prime-number staggering.

## Window lifecycle

Radar mode uses a real small top-right layered window. World-map mode uses a client-sized layered window only while the map is active. On entry the Overlay hides, resizes with `SWP_NOCOPYBITS`, performs one hidden `Invalidate() + Update()` prepaint, and reveals immediately. There is no fixed reveal delay or recurring warm-up. The first missing active-map sample stops full-map production and restores radar mode.

## Save and availability state

Treasure and Boss state are read from consistent snapshots of the active `.db`/`.bak` and available WAL/SHM/journal sidecars. Treasures remain hidden until the first complete save snapshot. Boss cycle 106 uses the verified daily 09:00 local reset. Production code does not enumerate streamed `Character` objects or install gameplay hooks.

## Diagnostics

Normal mode writes low-volume Use logs. Detailed producer, Bridge, timer, paint, CPU, memory, save, and geometry metrics are computed only when `debug_logging = true`. `overlayPaintFps` is the WinForms paint rate, not the game's Present FPS.
