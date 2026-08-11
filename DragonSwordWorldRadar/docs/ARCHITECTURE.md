# Architecture

Bosses and Assault targets use one world-encounter runtime. Installation generates nine Bosses and forty current-PAK Assault identities, resolves fingerprint-pinned optional conditions against fresh source tables, and preserves provenance. Overlay startup validates both generated datasets, fixes one 49-ID `tb_actor_respawn` target set, and creates one availability cache. F7 changes only presentation/runtime enablement; it never loads a catalog, reconfigures SQL, or resets the save snapshot cache.

## Runtime processes

### World-transition safety boundary

The UE4SS producer intentionally does not register `LoadMap` hooks. Travel is
detected through temporary current-Pawn/root loss. A missing or failed player
sample increments a world epoch, stops provider work, invalidates queued
callbacks, clears retained Engine/minimap/world-map/provider handles, publishes
a disabled bridge frame, and starts a three-second cooldown containing zero
UObject work. Cooldown expiry releases exactly one normal cached-Engine/current-
Controller/current-Pawn location sample. Failure rearms the complete cooldown;
success resumes map/provider work. Recovery never calls `Pawn:GetWorld()` or
`World:GetFullName()` and has no dedicated 250 ms recovery callback. Every
queued control, compact-motion, and expanded-map-motion callback captures its
scheduling epoch and rejects itself before UObject access if that epoch is
stale. F8 cancels deferred activation and advances the same boundary.

The supplied 1.6.1 reference object policy is preserved literally: Engine is
resolved once and retained, but every sample reaches the current Controller and
current Pawn through that cached root. A Pawn wrapper is never retained across
frames or world teardown. `DLayerMiniMap` is resolved only when its cached
wrapper is absent or invalid. Stable 250 ms control and 50 ms motion samples
therefore perform no UObject-array lookup. No World-identity traversal is used
for recovery. All retained runtime wrappers are cleared at the fail-closed
boundary, and world-map/provider access stays suspended until a normal player-
location sample succeeds.

The one-hertz minimap operation reads only `MapOverlay` scale for radius. It
does not call `IsVisible()`, traverse `DPanelMain`/`Overlay_Minimap`, or attempt
to infer task/cutscene HUD state. Compact presentation consequently follows the
same player-context/master-enable and external foreground/minimize behavior as
the reference mod. Expanded-world-map detection remains independent.

DragonSwordWorldRadar uses UE4SS Lua as the game-side producer, a hidden per-session WScript watcher, a game-bound hidden Windows PowerShell 5.1 host, and an in-memory compiled WinForms Overlay. No custom radar executable is built or distributed.

```text
UE4SS Lua
  -> alternating protocol-v5 scalar Motion/control slots
  -> hidden WScript watcher / exact-PID PowerShell host
  -> in-memory WinForms Overlay
```

## Single live IPC channel

`runtime/bridge/radar_motion_a.dat` and `radar_motion_b.dat` are the only active Lua-to-Overlay state channel. Each compact ASCII record contains 37 fields: protocol version, generation, world epoch, numeric sample timestamp, sequence integrity, enabled/mode state, layer controls, Mole/Fly visibility, game-time status, reserved unavailable weather slots, text scale, player coordinates, radar radius, and active world-map transform data.

Compact treasure selection uses the packaged-dev15 map-100 index and original query-buffer ownership; no world-map-derived session map or post-dev15 map-key field exists. Expanded world-map treasure rendering remains independently keyed by the current frame's `WorldMap.mapId`.

The old `radar_state*.json` files are cleanup-only. They are never read or written as active state. This removes duplicate state ownership, JSON serialization/parsing, the 200 ms Static poll, and static/motion fallback merging.

Protocol v3 adds only two fields to the sole Motion/control record: the Mole/Fly display switch and a 34-bit unfinished-record mask. Install-generated coordinates remain local Overlay data, so no marker catalog crosses the bridge and no second channel exists.

Protocol v4 adds six fields to the same record: the world-status display switch, time availability and normalized seconds, plus three weather slots retained as unavailable/zero for wire compatibility. Legacy records are rejected instead of being interpreted with shifted fields.

The installer shape-validates exactly 34 Fly records, stable IDs/mask bits, and same-ID `DT_MiniGame_G5` reward mappings. `WorldMoleCatalog` indexes them once by map/section. The Overlay filters their mask only when the existing external save snapshot or catalog version changes. No Mole completion UObject is resolved or called; raw reward bits bypass treasure-only ignore/alias rules.

Dev40 uses the user-tested dev38 feature boundary: Assault is force-isolated and production `main.lua` does not import or call `world_environment`. Player motion is sampled through one fresh Engine-to-current-Pawn traversal in the 250 ms control task. Protocol v5 publishes only epoch, timestamp, and scalar coordinates; the 50 ms Overlay predicts from two samples without UE access. Visible large-map transforms reuse the same scalar player position.

F7 enables configured marker and world-status layers. F8 cancels the clock, suspends save refresh, publishes one fail-closed disabled lifecycle record, hides the Overlay, and lets the 250 ms control loop plus both 50 ms producers terminate. Mole/Fly work is context-disabled before publication.

The high-frequency radar predicate contains only actual marker layers: treasures, bosses, Assaults, or Moles. Height and treasure-type controls decorate treasures and do not independently activate motion. Player/world context loss clears published time before a disabled lifecycle frame but retains the F7 master state so valid context can recover. F8 retains the disabled master state across context changes. Complete `mods.txt=0` remains a full shutdown.

Compact mode restores the dev15 rectangular click-through/no-activate window without Win32 HRGN composition APIs. Its scaled width equals the 360-reference-pixel minimap square; its height is 400 reference pixels. The compact 138-pixel status group begins six pixels above the minimap square's lower edge, using only the otherwise transparent circular-map corner before extending into a 40-pixel lower strip. Expanded world-map mode still uses the complete game-client rectangle and omits status rendering.

## Data ownership

```text
TreasureDataProvider -> data/generated/treasures.lua
                     -> Overlay catalog + save visibility index
                     -> minimap/world-map renderer

BossDataProvider     -> data/generated/bosses.lua (exactly 9)
                     -> Overlay catalog + tb_actor_respawn availability
                     -> minimap/world-map renderer

MoleDataProvider     -> data/generated/moles.lua (exactly 34)
                     -> Overlay catalog + reward-save-bit visibility mask
                     -> minimap/world-map renderer

isolated scalar world time -> status renderer fail-closed until captured
                            -> protocol-v5 unavailable time fields
                            -> radar-only status and Assault condition
```

Lua no longer serializes fixed treasure or Boss catalogs. It publishes only live UObject/control data that the external Overlay cannot obtain itself.

## Scheduler

- Radar position sampling: 50 ms.
- Active world-map transform production and Overlay presentation: 8 ms, only while the map is visible; both reuse the shared 250 ms numeric player sample and stop immediately on exit. A balanced mode-scoped timer-resolution request is held only while the expanded map is visible and is not double-acquired when the startup-wide user option is enabled.
- World-map widget ownership is lifecycle bounded: at most two wrappers, each tagged with capture epoch and candidate token. Reset retires their scalar identities, drops every wrapper, and increments the token. A single resume scan examines results transiently, prefers newest visible candidates, and cannot assign a retired identity to a new epoch. `NotifyOnNewObject` is direct evidence for admitting a current-epoch widget. This removes structural unbounded retention; whether it is the complete runtime cause of progressive post-travel slowdown remains unconfirmed until in-game acceptance.
- Low-frequency Pawn/map-mode control sampling: 250 ms; fresh minimap radius sampling: 1000 ms; zero production game-time reads.
- Motion heartbeat: 1000 ms.
- Overlay active timer: 50 ms; world-idle/radar-idle/disabled/background modes are 50/75/125/500 ms.
- Foreground/minimize/visibility API sampling: 250 ms; F7/F8 and radar/world mode transitions expire the cached sample immediately.
- Overlay maintenance: 1000 ms. F7-gated save-slot metadata checks: 2000 ms. The first snapshot may queue immediately because its worker still verifies an unchanged source fingerprint before/after copying; later changes retain the four-second debounce. Installer-generated treasure, Boss, and Mole catalogs stop filesystem polling after the first validated load.
- Motion stale timeout: 2500 ms.
- XY publication threshold: half a compact projected pixel with a 20-unit floor (approximately 36.8 town / 66.2 field units); Z threshold: 10 game units.
- The textual Lua `LoopAsync` registration count remains three; no added pseudo-worker loop or prime-number staggering.

## Window lifecycle

Radar mode uses a real small top-right layered window. World-map mode uses a client-sized layered window only while the map is active. On entry the Overlay hides, resizes with `SWP_NOCOPYBITS`, performs one hidden `Invalidate() + Update()` prepaint, and reveals immediately. There is no fixed reveal delay or recurring warm-up. The first missing active-map sample stops full-map production and restores radar mode.

## Save and availability state

Treasure and Boss state are read from consistent snapshots of the active `.db`/`.bak` and available WAL/SHM/journal sidecars. Treasure visibility fails closed before the first complete save snapshot. Installation resolves the save-owner pointer RVA from the exact current PE image while the EXE/PAK fingerprint is locked before and after generation. Runtime accepts the generated RVA only after recomputing the same current fingerprint; the generated file contains no SQLCipher key. Known RVAs and one delayed pattern scan remain compatibility fallbacks. A later changed slot must remain stable for four seconds; the first snapshot skips that delay because the background worker still rejects a source that changes across its copy. The reader runs at best-effort below-normal priority, applies the SQLCipher key once per isolated temporary snapshot connection, and reuses the parsed result of an unchanged sibling database by exact fingerprint. This is an in-memory session cache only. The first valid snapshot or later save-version change atomically rebuilds the map-grouped index and removes confirmed-open records. Daily Boss and Assault recovery uses the observed shared 09:00 Korea Standard Time boundary (00:00 UTC), independent of the player's local time zone. Production code does not enumerate streamed `Character` objects or install gameplay hooks.

The dev15 runtime rollback retains the accepted data interpretation: fixed chest 10220122 is absent from default ignores and installation removes only its exact obsolete ignore/explanation. Duplicate/offset record 11003 remains ignored, with 14016 authoritative; unrelated user rules remain untouched.

## Diagnostics

Normal mode writes low-volume Use logs, including whether the restart-scoped `high_resolution_timer` request was requested and activated. Detailed producer, Bridge, timer, paint, CPU, memory, save, geometry, work-duty, composed-pixel-rate, window-sampling, and per-layer treasure/Boss/Mole/status metrics are computed only when `debug_logging = true`. `SAVE_REFRESH_PERF` separates copy, key, treasure query, Boss/Assault query, cache hits, and total time. `overlayPaintFps` is the WinForms paint rate, not the game's Present FPS.
