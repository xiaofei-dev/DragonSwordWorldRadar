# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar for **DragonSword Awakening**. This repository snapshot contains the complete development source for version `0.4.0-dev74-processdispatchguard1-localcapfix1` in the original repository layout.

This optimization candidate keeps the proven 250 ms fresh-current-Pawn sample and 50 ms Lua scalar-flush loop, but changes the compact Overlay consumer to a 33 ms in-memory prediction/presentation path. A compact-only file notification marks the changed Motion Bridge slot dirty; parsing remains serialized on the UI thread, healthy notification loss is covered by a 250 ms dual-slot scan, and an unavailable watcher falls back to 50 ms polling. Activation, compact control, the isolated clock, and the visible expanded map share exactly one stable ProcessEvent callback and one in-flight scalar slot; independent task producers coalesce instead of submitting overlapping UE4SS actions. One accepted callback that does not enter within three seconds permanently poisons that route for the current game session and disables WorldRadar without retry or EngineTick fallback. At most two hidden current-epoch map candidates are visibility-tested at 250 ms; a failed active read drops every wrapper and arms one bounded rescan on the next control sample. Confirmed task exceptions or five seconds of missing control-heartbeat progress retain the bounded automatic F8-to-F7 restart only while the ProcessEvent route remains healthy. F5/F6 runtime-attribution controls are registered only when `debug_logging = true`, which is the current acceptance default. Static validation does not constitute gameplay acceptance.

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
& .\build\Test-ReleasePackage.ps1
```

The build creates `dist/DragonSwordWorldRadar-v0.4.0-dev74-processdispatchguard1-localcapfix1.zip`. Windows PowerShell 5.1 `Add-Type` is the authoritative compiler path because installation uses the same compiler. `Build-Release.ps1` also runs the independent ZIP/manifest/path/byte-identity audit.

---

## 1.8 single-bridge design

The dev18 active path retains the dev17 cadence contract:

- 250 ms fresh current-Controller/Pawn position sampling; the 50 ms compact Lua loop only flushes a pending scalar or heartbeat and performs no UObject read.
- Once-per-game-frame world-map transform production with 8 ms scalar bridge/Overlay presentation only while the expanded map is visible; closing it immediately pauses the game-thread action, restores the normal 33 ms compact presentation cadence, and releases the temporary 1 ms Windows timer-resolution request.
- 250 ms low-frequency control sampling through a cached Engine root, plus one `DLayerMiniMap` scale sample per second. Only retained top-level UObject roots use `IsValid()`; nested minimap property wrappers are read inside a protected block so a usable `LayerMap` cannot trigger a one-hertz global lookup loop.
- 33 ms active compact Overlay prediction/presentation, with bridge files consumed only on dirty-slot notification or a bounded fallback scan; existing lower-frequency idle/background modes remain.
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
鈹溾攢 fixed protocol-v6 scalar parser
鈹溾攢 local treasure catalog
鈹溾攢 local nine-boss catalog
鈹斺攢 save/Boss availability filtering
```

### Motion protocol v6

Each alternating Motion slot is one fixed 38-field ASCII record. Protocol v6 retains the world epoch and scalar sample timestamp for UObject-free Overlay prediction and adds one strict normal/no-paint/no-motion diagnostic enum. It contains:

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
- Compact player scalars are sampled at 250 ms and flushed by the unchanged 50 ms Lua loop, while the Overlay presents bounded in-memory prediction at 33 ms. Compact bridge records are read only when a slot is dirty, every 250 ms as a healthy safety scan, or every 50 ms if notifications are unavailable. All game-thread work shares one stable ProcessEvent callback and one in-flight scalar slot; the visible expanded world map requests its transform at 8 ms but coalesces behind activation, control, and clock work rather than creating an independent native queue entry. A three-second missed callback poisons ProcessEvent for the session with zero retries and no EngineTick fallback. The latest world-map scalar snapshot is presented at 8 ms while reusing the same numeric player sample. The retained widget set is capped at two and bound to the current lifecycle epoch/candidate token, and transition reset drops all wrappers. Compact player deltas below half a projected radar pixel are suppressed before bridge writes. The dev15 compact radar selects the nearest unopened treasure set once per second from the save-filtered map-100 index. Expanded world-map rendering continues to use each frame's `WorldMap.mapId`; this selector is not the save-open check.

Installation resolves the current executable's save-owner pointer through the existing PE pattern while the executable/PAK fingerprint is locked. The generated `data/generated/save_owner_pointer.cfg` contains only schema, fingerprint, EXE length, RVA, and provenance—never the SQLCipher key. Runtime recomputes the exact fingerprint before using it, then retains known-RVA and one delayed pattern-scan fallback paths for update compatibility.
- Boss and Mole markers use direct retained-geometry vector drawing on every paint, exactly matching dev15 behavior.
- Assault and the isolated one-shot world clock follow the single authoritative `scripts/config.lua`; neither adds recurring UObject scans or an additional bridge.
- Lua samples one fresh current Pawn per 250 ms control callback. The 33 ms Overlay predicts from two scalar samples, accumulates movement below the existing half-pixel threshold instead of repainting it, clamps at 250 ms, and freezes after 500 ms stale age without UE access.
- `show_world_status` defaults true. F8 hides it with every other Mod feature; a failed one-shot baseline remains hidden until the next F7.

### Local static data ownership

- The Overlay loads `data/generated/treasures.lua` directly and applies save filtering locally.
- The Overlay validates `data/generated/bosses.lua` and `data/generated/assaults.lua` once at startup, combines them into one 49-record world-encounter catalog, and applies one `tb_actor_respawn` availability pipeline locally.
- The compatibility wrappers `scripts/treasures.lua` and `scripts/bosses.lua` remain in the source package, but normal 1.8 runtime code does not require either catalog in Lua.
- The dev15 runtime rollback retains the accepted data rules: fixed chest `10220122` has no default ignore and upgrades remove only its obsolete ignore/explanation. Duplicate/offset record `11003` remains ignored, with `14016` documented as authoritative.

### Failure behavior

- A Motion frame older than 2500 ms is treated as stale; the Overlay hides instead of continuing to draw obsolete coordinates.

`high_resolution_timer = true` is the default Overlay timer mode and requests `timeBeginPeriod(1)` for the Overlay process. Set it to `false` and restart the game to use the Windows default outside the expanded-map-only timer scope. The Use log records both the requested and active state. Detailed debug logs report treasure, unified encounter, Mole/Fly, and status-layer costs separately.
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
- F7-gated save refresh uses one non-harmonic 19-second change-check interval. Each tick compares the active slot and key with the last successfully published fingerprint; an unchanged source exits without copying or opening SQLCipher, while a change queues one managed-below-normal, Windows-background-mode Treasure-rich snapshot. Every successful or failed worker starts a fresh 19-second completion cooldown, preventing an expired check deadline from immediately queuing catch-up SQL after a slow query. A change observed while the worker is busy or cooling is retained and revalidated at the next eligible check. The copy is published only when its source fingerprint is identical before and after copying. Zero-only categories are not materialized, and the research-only encounter-task table is queried only when debug logging is enabled. Independent `.db`/`.bak` fingerprint caches still eliminate duplicate sibling reads. F8 performs no new save work.
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

The current acceptance build defaults `debug_logging = true` so F5/F6 attribution, Overlay performance counters, save timings, and the diagnostic-only encounter-task table remain available. Set it to `false` after final acceptance to remove that diagnostic work.
Normal-use lifecycle logging remains enabled. Set it to `true` only for a
short detailed diagnostic capture, then restart the game. Keep
`diagnostic_verbose = false`; it enables dense in-map marker labels.

| Purpose | Path |
|---|---|
| Lua normal-use log | `runtime/logs/DragonSwordWorldRadar.Lua.Use.log` |
| Lua debug/performance log | `runtime/logs/DragonSwordWorldRadar.Lua.Debug.log` |
| Overlay normal-use/error log | `runtime/logs/DragonSwordWorldRadar.Overlay.Use.log` |
| Overlay debug/performance log | `runtime/logs/DragonSwordWorldRadar.Overlay.Debug.log` |

Overlay debug output reports the single Bridge's read rate, complete frames, redraw-producing frames, suppressed frames, timer gaps, paint gaps, CPU, and working set. `SAVE_REFRESH_PERF` separates copy, key, treasure-query, Boss/Assault-query, database-read/cache-hit, request scope, executed treasure-query count, completion cooldown, and total time. The large treasure/filter detail block is built only when the save, catalog, or filtered-index version changes. `overlayPaintFps` is the WinForms paint rate, not the game's Present FPS.

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

5. Start the game normally. Use **F7** for normal Radar and **F8** for the complete-off baseline. When `debug_logging = true`, **F5** adds the no-paint diagnostic and **F6** adds the frozen-motion diagnostic.

`scripts/config.lua` is the sole configuration file used by Lua, the Overlay, installer validation, diagnostics, and local deployment preservation. The installer also preserves `data/treasure_overrides.txt` and unrelated `mods.txt` entries. The watcher host checks `mods.txt` once at startup, and the Overlay loads treasure overrides once during construction; runtime edits to either require a game restart.

## Validation status

The release preparation validates Lua syntax, disabled/radar/world protocol-v6 records, all three diagnostic enum values, scalar prediction bounds, protocol positive and negative cases, C# lexical structure and Windows PowerShell 5.1 CodeDOM compatibility patterns, JSON/XML structure, PowerShell delimiter structure, catalog shape, source invariants, manifest hashes, ZIP CRC, path safety, duplicate entries, extracted-byte equality, and bundled binary hashes.

Windows PowerShell 5.1 `Add-Type` compilation passes for the current source and is also enforced by `Install.cmd`. Same-route in-game testing remains the authoritative behavior and performance gate. No specific FPS increase is claimed from build-time validation alone.

## Game updates

Installation locks the pre-generation game fingerprint into `datasets.json` and generated Assault records, verifies that the post-generation identity is identical, and then records it in `metadata/install-state.json`. If the game changes, the watcher requests reinstallation rather than using stale generated data.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.
