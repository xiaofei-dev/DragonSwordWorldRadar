# Performance baselines and current map-lifecycle repair

## Current repaired successor: 0.4.0-dev9-performance1.3-mapinstant-hiddenhost1

### Regression identified after performance1.1

The later `new` source changed the Overlay from a real small radar window into a game-client-sized transparent layered window that remained alive in radar mode. It also added a world-map UObject probe every 96 ms alongside the 24 ms motion scheduler, three-sample map-exit confirmation, and a 250 ms stale re-entry block. Those changes increased composition and game-thread work and matched the reported persistent frame loss and black edge/tear during map loads.

### Repair

- Radar mode again uses an actual small top-right layered window.
- World-map entry is detected by the existing 250 ms static update; there is no separate always-on 96 ms map probe.
- The 24 ms map transform producer exists only while the map is active.
- The Overlay is hidden before changing geometry, uses `SWP_NOCOPYBITS`, repaints the newly sized hidden surface synchronously, and appears without activation in the same timer cycle. No fixed reveal delay remains.
- The first missing active-map read immediately clears map state, publishes radar motion, and restarts the radar producer.
- Active map transforms are reused by the static publisher instead of read twice.
- Normal mode does not calculate or write periodic Lua/Overlay performance diagnostics.
- Debug configuration is parsed once at Overlay startup; Timer ticks do not poll `config.lua`.
- Boss cooldowns use the verified fixed daily 09:00 local reset; the Overlay performs no runtime PAK enumeration or rescan.
- Game-time Lua no longer creates a shell process. A resident hidden WScript watcher receives a newline-free digits-only request stamp, validates it, and starts the exact-PID PowerShell host with window style 0.
- Expected startup save-key readiness is Debug-only, and known save-key RVAs are tried before a delayed one-time worker-thread executable signature fallback.

### Debug measurements

Debug mode produces two performance views:

- Lua producer: sampling/write rates, queue delay, player/map read time, build/write/total duration, and 50/100/250 ms stall counts.
- WinForms Overlay: timer cadence and lateness, paint cadence and gaps, refresh/paint duration, invalidation counts, process CPU, working set, visibility, geometry, and window pixel area.

`overlayPaintFps` measures WinForms paint frequency only. It is not the game's Present FPS. External frame-time capture is still required for an authoritative before/after game-performance comparison.

### Validation status

The repair passed static semantic, syntax, inventory, JSON/XML, binary-hash, and package checks in a Linux preparation environment. Windows PowerShell 5.1 `Add-Type` compilation and in-game measurement were not available. Expected performance improvements are therefore architectural, not measured claims. `Install.cmd` must print `OVERLAY_COMPILE_OK`, followed by an in-game baseline/mod-on/map-transition test.

## Historical accepted baseline: 0.4.0-dev9-performance1.1

This version was the first accepted performance baseline after the dev8 reliability refactor.

### Implemented hot-path changes

- 24 ms motion sampling with cumulative visual thresholds.
- One-second liveness heartbeat while visually idle.
- File timestamp/length gates before opening alternating bridge slots.
- Adaptive Overlay timer: 24/50/75/125 ms.
- Visual-state comparison before `Invalidate()`.
- Retained and batched treasure paths.
- Reused game process identity.

### Historical in-game diagnostic result

The project owner's diagnostic run showed normal installation/compilation, exit code 0, no Lua or motion/static write failures, no invalid/partial/stale bridge errors, and correct transitions through active, world-idle, radar-idle, and disabled timer states. Idle minimap heartbeats were suppressed without paints. The remaining primary hot path was active world-map projection and painting of roughly 1,100 visible treasure markers.

The diagnostics archive used for that review is not bundled into the source tree. Its SHA-256 is recorded in `metadata/source-snapshot.json`.
