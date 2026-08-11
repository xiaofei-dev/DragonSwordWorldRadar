# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar for **DragonSword Awakening**. This repository snapshot contains the complete development source for version `0.4.0-dev59-ue4ssroot1` in the original repository layout.

The Assault layer is generated from the current game PAK, validates exactly 40 map-100 targets, reuses `tb_actor_respawn`, and interprets generic per-target time conditions. Its versioned inference-policy input is locked to the exact game fingerprint and resolved against freshly extracted target/cycle tables. It adds no runtime actor scan, save reader, bridge channel, hook, or bitmap cache.

## Repository layout

```text
src/ue4ss/          UE4SS Lua producer
src/overlay/        C# WinForms Overlay
src/installer/      installer entry point and C# installer core
src/host/           hidden watcher and game-bound host
src/tools/          diagnostics collector
resources/          release-time defaults
build/              source verification, compile tests, and release packaging
docs/               architecture and performance notes
vendor/              required local build/runtime dependencies
```

The installable package uses a different layout (`scripts/`, `host/`, `installer/`, and `src/overlay/`). `build/Build-Release.ps1` performs that source-to-release mapping. Do not develop directly inside a deployed Mod folder.

## Build and validation

Run from Windows PowerShell 5.1:

```powershell
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
```

The build creates `dist/DragonSwordWorldRadar-v0.4.0-dev59-ue4ssroot1.zip`. Windows PowerShell 5.1 `Add-Type` is the authoritative compiler path because installation uses the same compiler.

---

## 1.8 single-bridge design

The dev18 active path retains the dev17 cadence contract:

- 50 ms minimap position sampling.
- Diagnostic 4 ms world-map transform production and Overlay presentation only while the expanded map is visible; closing it immediately restores the normal 50 ms compact cadence and releases the temporary 1 ms Windows timer-resolution request.
- 250 ms low-frequency control sampling through a cached Engine root, plus one `DLayerMiniMap` scale sample per second; both objects are resolved only on cache miss/invalidity, matching the supplied 1.6.1 reference.
- 50 ms active Overlay timer, with the existing lower-frequency idle/background modes.
- 250 ms foreground/minimize/visibility sampling with immediate sampling on F7/F8 and radar/world transitions.
- 20 XY and 10 Z movement thresholds.
- Exactly the existing three textual `LoopAsync` registrations; no new worker loop, prime-number staggering, or ThreadPool polling chain.
- Reference-style current Controller/Pawn sampling without retaining Pawn wrappers, C# treasure selection, save-filtered local catalog, normal buffered save-snapshot flush, delayed full-EXE key scan, and one-time hidden world-map prepaint.

The old runtime path was:

```text
UE4SS Lua
鈹溾攢 radar_motion_a/b.dat     (high-frequency movement)
鈹斺攢 radar_state_a/b.json     (low-frequency duplicate state)

Overlay
鈹溾攢 fixed-record parser
鈹斺攢 JSON parser + static/motion fallback merge
```

The 1.8 path is:

```text
UE4SS Lua
鈹斺攢 radar_motion_a/b.dat     (all live control and motion data)

Overlay
鈹溾攢 fixed protocol-v5 scalar parser
鈹溾攢 local treasure catalog
鈹溾攢 local nine-boss catalog
鈹斺攢 save/Boss availability filtering
```

### Motion protocol v4

Each alternating Motion slot is one fixed 37-field ASCII record. Protocol v5 adds world epoch and scalar sample timestamp for UObject-free Overlay prediction. It contains:

- leading and trailing sequence numbers;
- explicit protocol version and Lua generation;
- enabled state and `radar` / `world` / `disabled` mode;
- height, treasure-type, treasure-layer, Boss-layer, Mole/Fly-layer, and world-status display switches;
- the Mole/Fly unfinished mask and unavailable world-status/weather fields, plus world epoch and sample timestamp;
- text scale;
- player XYZ, Z validity, and radar radius;
- all world-map transform fields required while the map is open.

The explicit protocol version prevents an old 21-field record or partially upgraded installation from being interpreted with shifted fields. The existing two-slot sequence validation still rejects partial rewrites.

### Work removed from the runtime

1.8 removes:

- the Lua Static JSON builder and one-second heartbeat write;
- Static Bridge Boss serialization;
- `radar_state_a.json`, `radar_state_b.json`, and the legacy single state file as active IPC;
- the Overlay's 200 ms Static file poll;
- `JavaScriptSerializer` and the `System.Web.Extensions` dependency;
- Static/Motion generation matching, fallback-state merging, and duplicate redraw comparison;
- the Lua `boss_tracker.lua` runtime module.

The 250 ms Lua callback reads only the low-frequency UObject/control state needed to refresh current Pawn context, detect expanded-map mode, and update minimap radius. It does not query parent-HUD/minimap visibility, build JSON, or write a second bridge. Expanded world-map rendering remains independent.

### World time and phase status

- Each F7 activation schedules one `DGameSingleton.TimeOfDay` scalar read only after eight valid 250 ms player-context samples. The wrapper is discarded immediately, with no retry or recalibration read during that activation.
- A failed baseline read leaves time unavailable until the next F7. A successful read advances locally at sixty game seconds per real second without recalibration. Weather remains unavailable and is never queried.
- F8 hides all mod visuals and stops marker motion, Mole/Fly work, save refresh, and clock work for clean FPS A/B comparison without rewriting configuration.
- The compact window is 360×400 reference pixels. Its smaller 138-pixel time/phase group starts six pixels above the minimap square's lower edge and remains clamped inside the transparent window.
- The dev16 Win32 composition-region experiment is removed; compact and expanded modes use the dev15 rectangular layered-window behavior.
- Compact active movement uses a coherent 20 Hz cadence (50 ms) in Lua and the Overlay. The visible expanded world map alone uses an 8 ms transform/presentation cadence while reusing the same 250 ms numeric player sample. Its retained widget set is capped at two and bound to the current lifecycle epoch/candidate token; transition reset drops all wrappers. Compact player deltas below half a projected radar pixel are suppressed before bridge writes. The dev15 compact radar selects the nearest unopened treasure set once per second from the save-filtered map-100 index. Expanded world-map rendering continues to use each frame's `WorldMap.mapId`; this selector is not the save-open check.

Installation resolves the current executable's save-owner pointer through the existing PE pattern while the executable/PAK fingerprint is locked. The generated `data/generated/save_owner_pointer.cfg` contains only schema, fingerprint, EXE length, RVA, and provenance—never the SQLCipher key. Runtime recomputes the exact fingerprint before using it, then retains known-RVA and one delayed pattern-scan fallback paths for update compatibility.
- Boss and Mole markers use direct retained-geometry vector drawing on every paint, exactly matching dev15 behavior.
- Dev40 returns to the user-tested dev38 feature boundary: Assault is force-isolated and production clock access is disconnected.
- Lua samples one fresh current Pawn per 250 ms control callback. The 50 ms Overlay predicts from two scalar samples, clamps at 250 ms, and freezes after 500 ms stale age without UE access.
- `show_world_status` defaults true. F8 hides it with every other Mod feature; a failed one-shot baseline remains hidden until the next F7.

### Local static data ownership

- The Overlay loads `data/generated/treasures.lua` directly and applies save filtering locally.
- The Overlay validates `data/generated/bosses.lua` and `data/generated/assaults.lua` once at startup, combines them into one 49-record world-encounter catalog, and applies one `tb_actor_respawn` availability pipeline locally.
- The compatibility wrappers `scripts/treasures.lua` and `scripts/bosses.lua` remain in the source package, but normal 1.8 runtime code does not require either catalog in Lua.
- The dev15 runtime rollback retains the accepted data rules: fixed chest `10220122` has no default ignore and upgrades remove only its obsolete ignore/explanation. Duplicate/offset record `11003` remains ignored, with `14016` documented as authoritative.

### Failure behavior

- A Motion frame older than 2500 ms is treated as stale; the Overlay hides instead of continuing to draw obsolete coordinates.

`high_resolution_timer = false` is the default Overlay timer mode. Set it to `true` in `scripts/config.lua` and restart the game to request `timeBeginPeriod(1)` for an A/B comparison. The Use log records both the requested and active state. Detailed debug logs report treasure, unified encounter, Mole/Fly, and status-layer costs separately.
- New valid Motion data restores the Overlay automatically.
- Legacy Static files are deleted only as upgrade/session cleanup; they are never read or written as active state.
- Catalog, save, renderer, timer, process, and geometry failures remain isolated and rate-limited.

## World-map behavior

World and dungeon transitions fail closed. The Lua producer publishes disabled
state while loading, drops every retained UObject root and queued old-world
callback, performs zero UObject work for three seconds, and then permits one
normal current-player location sample. A failed sample restarts the complete
cooldown; a valid sample resumes map and provider work without calling
`Pawn:GetWorld()` or `World:GetFullName()`. F8 cancels deferred activation.

The accepted immediate-map behavior remains:

```text
hide Overlay
鈫?resize to the game client with SWP_NOCOPYBITS
鈫?one hidden Invalidate() + Update()
鈫?reveal immediately
```

There is no fixed one-second reveal delay and no recurring warm-up render loop. The first failed active-map sample stops full-map production and restores the small radar mode.

## Implemented layers

### Mole/Fly mini-games

- Exactly 34 `MiniGame_Fly` starts (`11001-11034`) generated from the current local PAK during installation.
- Classification is pinned to notice title `109208` and description `109202`; position priority is `NPC_Start`, then `Teleport_Start`, then `Fly_Linked`.
- Completion is read-only and fail-closed through the existing external save snapshot. Fly `11001-11034` maps directly to reward treasure `DT_MiniGame_G5_11001-11034`; claiming the same-ID reward hides that marker. No game UObject completion query is performed.
- The marker is a code-native winged upward arrow drawn by the existing WinForms Overlay on both radar and world map.
- Set `show_moles = false` in `scripts/config.lua` to disable this layer without changing the existing F7/F8 lifecycle.

### Treasures

- Minimap and world-map markers generated locally from the game PAK.
- SQLCipher save filtering, overrides, aliases, type colors, nearest marker, and height indicator.
- F7-gated two-second save metadata checks, a four-second stable-change debounce, one SQLCipher key setup per isolated temporary snapshot connection, below-normal refresh workers, and independent in-memory `.db`/`.bak` fingerprint caches. F8 performs no new save work.
- Player-height comparison offset: `-150`.
- Until the first complete save snapshot is available, treasure visibility fails closed and no treasure points are published. The first valid snapshot immediately rebuilds the index and publishes only records not confirmed open.

### World bosses

- Exactly nine fixed locations generated locally from game data.
- Availability comes from `tb_actor_respawn`; no streamed `Character` scan or HP hook is used.
- Daily Boss and Assault recovery uses the observed shared reset boundary at 09:00 Korea Standard Time (00:00 UTC), independent of the player's local time zone.
- Boss coordinates are loaded by the Overlay; Lua no longer serializes the same fixed records every second.

### Assault encounters

- Exactly forty map-100 targets are generated and identity-validated from the current local PAK during installation.
- Bosses and Assaults share one startup-fixed 49-ID save query, one respawn-availability tracker, and one render traversal. F7 performs no encounter catalog load, filter reconfiguration, or snapshot-cache reset.
- Optional conditions are attached to encounter records. The current build has one fingerprint-pinned `world_time_window`; unknown or unavailable condition sources fail closed. Weather remains excluded until a target-specific mapping and trustworthy runtime scalar are established.

## Logging and diagnostics

Configure logging in `scripts/config.lua`:

```lua
use_logging = true,
debug_logging = true,
diagnostic_perf_interval_seconds = 5,
```

Pre-release builds keep `debug_logging = true` so performance and lifecycle
tests remain comparable. Keep `diagnostic_verbose = false`; it enables dense
in-map marker labels and is intended only for short visual-diagnostics runs.
The final release should restore `debug_logging = false` after acceptance.

| Purpose | Path |
|---|---|
| Lua normal-use log | `runtime/logs/DragonSwordWorldRadar.Lua.Use.log` |
| Lua debug/performance log | `runtime/logs/DragonSwordWorldRadar.Lua.Debug.log` |
| Overlay normal-use/error log | `runtime/logs/DragonSwordWorldRadar.Overlay.Use.log` |
| Overlay debug/performance log | `runtime/logs/DragonSwordWorldRadar.Overlay.Debug.log` |

Overlay debug output reports the single Bridge's read rate, complete frames, redraw-producing frames, suppressed frames, timer gaps, paint gaps, CPU, and working set. `SAVE_REFRESH_PERF` separates copy, key, treasure-query, Boss/Assault-query, database-read/cache-hit, and total time. `overlayPaintFps` is the WinForms paint rate, not the game's Present FPS.

`Collect-Diagnostics.cmd` captures both Motion slots, logs, generated data, metadata, process state, UE4SS log tail, and relevant source hashes. It no longer collects active Static Bridge files.

## Installation

1. Close the game and all existing radar processes.
2. Extract the complete folder to `ue4ss/Mods/DragonSwordWorldRadar`.
3. Run `Install.cmd` outside a ZIP preview.
4. Require these markers:

```text
OVERLAY_COMPILE_OK
INSTALLER_COMPILE_OK
SESSION_WATCHER_READY
INSTALL_COMPLETE
```

5. Start the game normally. Press **F7** to enable and **F8** to disable.

The installer preserves a valid `scripts/config.lua`, `data/treasure_overrides.txt`, and unrelated `mods.txt` entries. It also removes the three exact 1.7 source files retired by the single-Bridge architecture, so a normal full-folder overwrite followed by `Install.cmd` does not retain duplicate C# models or the old Lua Boss tracker.

## Validation status

The release preparation validates Lua syntax, disabled/radar/world protocol-v5 records, scalar prediction bounds, protocol positive and negative cases, C# lexical structure and Windows PowerShell 5.1 CodeDOM compatibility patterns, JSON/XML structure, PowerShell delimiter structure, catalog shape, source invariants, manifest hashes, ZIP CRC, path safety, duplicate entries, extracted-byte equality, and bundled binary hashes.

Windows PowerShell 5.1 `Add-Type` compilation passes for the current source and is also enforced by `Install.cmd`. Same-route in-game testing remains the authoritative behavior and performance gate. No specific FPS increase is claimed from build-time validation alone.

## Game updates

Installation locks the pre-generation game fingerprint into `datasets.json` and generated Assault records, verifies that the post-generation identity is identical, and then records it in `metadata/install-state.json`. If the game changes, the watcher requests reinstallation rather than using stale generated data.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.
