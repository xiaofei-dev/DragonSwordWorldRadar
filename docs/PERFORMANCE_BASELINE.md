# Performance baseline

## Current candidate: 0.4.0-dev9-performance1.8-singlebridge1

The accepted runtime baseline is 1.7 Stable Core. Version 1.8 keeps its motion cadence and removes the remaining redundant Static Bridge.

### Retained baseline behavior

- 24 ms minimap position sampling.
- 24 ms active world-map sampling, stopped immediately when the map closes.
- 250 ms low-frequency control sampling.
- 24 ms active Overlay polling.
- 20 XY and 10 Z publication thresholds.
- Cached Pawn fast path.
- Treasure selection and save filtering in C#.
- One-time hidden world-map prepaint without a fixed delay.

### 1.8 work removed

- Lua Static JSON construction and one-second heartbeat writes.
- Overlay 200 ms Static file checks.
- `JavaScriptSerializer` and `System.Web.Extensions`.
- Static/Motion generation matching and fallback-state merging.
- Lua serialization of the fixed nine-Boss catalog.
- The obsolete `boss_tracker.lua` runtime module.

### Expected effect

The expected improvement is lower periodic IPC/file/parser work and simpler failure behavior, especially reduced micro-stutter risk. No fixed FPS gain is asserted from static validation alone. In-game comparison against 1.7 using the same location, route, and F7/F8 state remains authoritative.

### Remaining major cost

The Overlay is still a topmost layered WinForms/GDI+ window composed by DWM. If a substantial static FPS gap remains between enabled and F8-disabled states, further work should profile or replace the presentation path rather than alter Lua timing without evidence.

### Validation

The source snapshot includes protocol positive/negative tests, C# source compilation gates for Windows PowerShell 5.1, Lua/source semantic checks, package mapping, manifest hashing, ZIP integrity checks, and bundled binary hashes. Windows compilation and in-game frame-time testing remain required.
