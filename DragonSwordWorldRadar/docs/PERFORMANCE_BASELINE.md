# Performance baseline

## Current candidate: 0.4.0-dev47-minimapdiagnostics1

Packaged dev38 is the behavioral feature baseline: Assault remains force-isolated and clock remains disconnected. Dev30 remains the owner-accepted smoothness reference.

Packaged dev30 (`0.4.0-dev30-referencecache1`) remains the owner-accepted smoothness baseline ("excellent experience"). This candidate preserves dev43's bounded epoch-owned world-map candidates, dev42's install-bound save-owner RVA, the single 250 ms fresh-player scalar sample, and 50 ms compact presentation, while using 8 ms only for the visible expanded map. Static comparison does not prove runtime acceptance; repeated travel remains the final gate.

Assault remains fully implemented but a missing-default-true isolation gate overrides even legacy `show_assaults=true`. In this A/B state there is no Assault compact/world traversal or draw, no catalog or availability maintenance refresh, and no Assault-only save diagnostic processing. Generated 40-record data and the re-enable path remain intact.

The Mole/Fly completion layer is unchanged from dev38. Normal player UObject sampling moves entirely to the 250 ms control callback; the 50 ms presentation path uses scalar history only.

Pending runtime targets are: game 99th-percentile frame-time regression below 2% against clean 1.8; Overlay CPU increase below one percentage point; combined Mole/world-status paint below 0.5 ms average and 1 ms p99; and no visible marker judder at 20 Hz. Overlay repaint FPS is not game present FPS; use PresentMon or CapFrameX.

The retained dev33 correctness baseline includes packaged-dev15 treasure visibility/index ownership, compact map 100, current-frame expanded-map ownership, nearest-query buffer semantics, direct Boss/Mole/Assault vector rendering, and no retained bitmap/layer render cache. Dev44 keeps compact production/presentation capped at 50 ms and uses 8 ms only for the visible expanded map; window-state APIs remain sampled at 250 ms with immediate mode-transition checks. Matching the supplied 1.6.1 object policy, Engine is resolved on cache miss while the current Controller and Pawn are traversed only for the shared 250 ms position sample, so no Pawn wrapper survives world teardown. World-map wrappers are additionally hard-capped and epoch/token-owned. Pawn/root loss performs zero UObject work for three seconds and releases one normal player-location sample; every failure rearms the complete cooldown. Stable active paths perform no recurring UObject-array scan, and no FPS or gameplay-success claim is made before in-game acceptance.

The accepted data rules are intentionally outside that runtime-performance rollback: fixed chest 10220122 remains present and its obsolete ignore is removed during upgrades; duplicate/offset 11003 remains ignored in favor of authoritative 14016.

The generated Mole catalog maps all 34 Fly records to their same-ID `DT_MiniGame_G5` reward treasure. The external save snapshot naturally preserves completion across restarts; raw reward bits bypass treasure display overrides such as the intentional 11003 duplicate suppression.

Assault reuses the existing `tb_actor_respawn` snapshot and shared Radar DAILY policy. The observed recovery boundary is 09:00 Korea Standard Time (00:00 UTC), independent of the player's local time zone. Save metadata is checked every two seconds only while F7 is active, and a changed slot must remain stable for four seconds before the low-priority SQLCipher worker runs. Parsed `.db` and `.bak` results are cached independently by exact session fingerprint, so an unchanged sibling is not copied or decrypted again. No additional cross-restart cache or reader is introduced.

The owner-accepted smoothness baseline for this candidate is packaged dev30. Earlier 1.7/1.8 architecture remains historical context, while dev30 runtime observation controls performance acceptance.

### Retained baseline behavior

- 50 ms minimap position sampling in dev24.
- 50 ms active world-map sampling in dev24, stopped immediately when the map closes.
- 250 ms low-frequency control sampling.
- 50 ms active Overlay polling in dev24.
- Radius-derived half-pixel XY publication threshold with a 20-unit floor; 10-unit Z threshold.
- Cached Engine with reference-style current Controller/Pawn traversal; no Pawn wrapper survives across samples.
- Treasure selection and save filtering in C#.
- One-time hidden world-map prepaint without a fixed delay.
- No world-status rendering in expanded world-map mode and no full-screen redraw triggered by time changes.

### Scalar motion boundary

- The 250 ms control callback is the sole current-Pawn full-chain sampling site, targeting approximately four reads per second.
- Compact and visible large-map 50 ms loops reuse the same numeric player sample; only the visible large map keeps its transform reads.
- Overlay prediction uses two protocol-v5 scalar samples. Epoch/enable/mode changes, timestamp regression, source gaps, and teleport/outlier distance reset history; prediction clamps at 250 ms and freezes after 500 ms stale age.

### 1.8 work removed

- Lua Static JSON construction and one-second heartbeat writes.
- Overlay 200 ms Static file checks.
- `JavaScriptSerializer` and `System.Web.Extensions`.
- Static/Motion generation matching and fallback-state merging.
- Lua serialization of the fixed nine-Boss catalog.
- The obsolete `boss_tracker.lua` runtime module.

### Expected effect

The expected improvement is about one-third fewer moving producer, bridge-consumer, invalidation, paint, and layered-window composition opportunities than the previous 33 ms candidate, plus lower save/catalog metadata polling. No fixed FPS gain is asserted from static validation alone. In-game comparison against 1.8 using the same location, route, camera, and F7/F8 state remains authoritative.

### Remaining major cost and safety tradeoff

The Overlay is still a topmost layered WinForms/GDI+ window composed by DWM. If a substantial static FPS gap remains between enabled and F8-disabled states, further work should profile or replace the presentation path rather than alter Lua timing without evidence.

Dev30 retained a Pawn wrapper and refreshed it at 4 Hz, then reused it for 20 Hz location calls. The later safety correction traverses cached Engine through current Controller/Pawn at every 20 Hz sample so no Pawn/Controller wrapper crosses a travel boundary. Debug counters now separate current-root traversal time from `K2_GetActorLocation`. If Assault isolation is insufficient, the next safe A/B is lower-rate game-thread scalar position capture plus Overlay interpolation. Cross-frame Pawn/Controller retention is not an acceptable speed optimization without explicit lifetime proof.

### Secondary A/B and evidence

- `high_resolution_timer = false` uses the Windows default timer resolution; `true` requests `timeBeginPeriod(1)`. Restart the game between samples.
- `OVERLAY_LAYER_PERF` reports separate treasure, Boss, Mole/Fly, and world-status calls/counts/average/max milliseconds.
- `OVERLAY_WORK_PERF` reports the observed window-visibility sampling rate; the target is approximately 4 Hz outside forced mode transitions.
- `SAVE_REFRESH_PERF` reports snapshot copy, one-time key setup, treasure query, Boss/Assault query, database reads/cache hits, and total elapsed milliseconds.

### Validation

The source snapshot includes protocol positive/negative tests, C# source compilation gates for Windows PowerShell 5.1, Lua/source semantic checks, package mapping, manifest hashing, ZIP integrity checks, and bundled binary hashes. Windows compilation and in-game frame-time testing remain required.
