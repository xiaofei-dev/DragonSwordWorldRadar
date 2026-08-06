# Performance baseline: 0.4.0-dev9-performance1.1

This version is the first accepted performance baseline after the dev8 reliability refactor.

## Implemented hot-path changes

- 24 ms motion sampling with cumulative visual thresholds.
- One-second liveness heartbeat while visually idle.
- File timestamp/length gates before opening alternating bridge slots.
- Adaptive Overlay timer: 24/50/75/125 ms.
- Visual-state comparison before `Invalidate()`.
- Retained and batched treasure paths.
- Reused game process identity.

## In-game diagnostic result

The project owner's diagnostic run showed normal installation/compilation, exit code 0, no Lua or motion/static write failures, no invalid/partial/stale bridge errors, and correct transitions through active, world-idle, radar-idle, and disabled timer states. Idle minimap heartbeats were suppressed without paints. The remaining primary hot path is active world-map projection and painting of roughly 1,100 visible treasure markers.

The diagnostics archive used for that review is not bundled into the source tree. Its SHA-256 is recorded in `metadata/source-snapshot.json`.
