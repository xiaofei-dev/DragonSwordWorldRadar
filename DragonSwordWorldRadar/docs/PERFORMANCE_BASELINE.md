# Performance baseline

## Current candidate: 0.4.0-dev74-minigamecatalog2

Dev66 supplied the attribution evidence for the owner's continuous compact-radar micro-stutter: F5 (no custom paint) felt approximately the same as F7, F6 (frozen motion and 250 ms Overlay control cadence) was substantially smoother, and F8 remained best. That result points away from marker drawing itself and toward work repeated by the active compact consumer. It does not isolate one instruction or prove a final frame-time result.

Dev72 preserves the dev71/dev70/dev69/dev67 evidence-driven performance architecture without changing compact cadence. It retains every visible layer, the 250 ms fresh-current-Pawn sample, the 50 ms scalar-only Lua flush loop, the 8 ms visible expanded-map path, epoch/token travel safety, and direct-vector rendering. Compact Overlay presentation remains 33 ms after removing unchanged bridge-file probes from that cadence. The two nearest treasure candidates are retained in the existing selection pass; the second adds only one normal vector marker and optional height arrow. F5/F6 hotkeys remain absent from normal play and are registered only for an explicit debug capture.

Expanded-map recovery now visibility-tests at most two already-bounded hidden candidates on the existing 250 ms control sample. A missing active sample stops the 8 ms world loop, drops every retained wrapper, advances the candidate token, and arms exactly one bounded current-epoch rescan on the next control sample. This removes the previous two-second rediscovery gap without adding an active-loop scan, bridge message, drawing pass, timer, or unbounded container. Runtime testing remains required to prove the three observed display-loss modes are resolved.

Dev72 also reuses the already-running compact or expanded-map presentation loop as a pure-scalar control-heartbeat observer. It compares progress once per second and requires five consecutive misses before a confirmed automatic F8-to-F7 lifecycle restart; a globally stalled game cannot advance the observer and therefore cannot create a false timeout. Explicit caught asynchronous exceptions arm the same bounded path. No fourth scheduler loop, UObject read, bridge operation, file access, drawing pass, or game-thread request was added. Temporary missing map state remains on the candidate-only dev71 path and cannot arm this restart.

One compact-only `FileSystemWatcher` marks slot A or B dirty. Its callback performs no read, parse, frame allocation, background task, or presentation mutation. The existing reusable fixed-buffer reader remains serialized on the UI thread and consumes only dirty slots. A healthy watcher receives a 250 ms dual-slot safety scan; setup or runtime failure restores 50 ms polling. F6, idle, and disabled states receive one coalesced UI wake for bridge transitions. World-map mode disables notifications and preserves the existing forced 8 ms dual-slot path.

Save snapshots keep the same complete 45-second publication contract. Their worker additionally enters balanced Windows background mode, ignores zero-only treasure categories, and skips the diagnostic-only `tb_unexpected_switch_week` query when debug logging is disabled. These changes do not remove or delay Treasure, Boss, Assault, Mole/Fly, world-status, compact-radar, or expanded-map output. They are source-verified optimizations; only an in-game A/B can establish the frame-time effect.

Static comparison does not prove runtime acceptance. Same-session F7/F8 frame-time, repeated travel, F7/F8 response latency, and clean shutdown are the final gates.

Assault is enabled by the single authoritative `scripts/config.lua` unless the user explicitly disables it. Its forty records share the immutable 49-record encounter catalog and save filter with the nine Bosses. The isolated one-shot world clock is likewise configuration-controlled and performs no recurring UObject read.

The Mole/Fly completion layer is unchanged from dev38. Normal player UObject sampling remains entirely in the 250 ms control callback; the 33 ms presentation path uses scalar history only.

Pending runtime targets are: materially lower F7 frame-time spikes and CPU duty than dev66 on the same route; no regression in F8, F6, world-map, or travel behavior; compact bridge metadata probes near dirty-write frequency plus the four-per-second safety scan rather than visual-tick frequency; and visibly smoother marker movement at the 33 ms presentation cadence. Overlay repaint FPS is not game present FPS; use PresentMon or CapFrameX.

The retained correctness boundary includes packaged-dev15 treasure visibility/index ownership, compact map 100, current-frame expanded-map ownership, nearest-query buffer semantics, direct Boss/Mole/Assault vector rendering, and no retained bitmap/layer render cache. The visible expanded map alone uses 8 ms; window-state APIs remain sampled at 250 ms with immediate mode-transition checks. Matching the supplied 1.6.1 object policy, Engine is resolved on cache miss while the current Controller and Pawn are traversed only for the shared 250 ms position sample, so no Pawn wrapper survives world teardown. World-map wrappers are additionally hard-capped and epoch/token-owned. Pawn/root loss performs zero UObject work for three seconds and releases one normal player-location sample; every failure rearms the complete cooldown. Stable active paths perform no recurring UObject-array scan, and no FPS or gameplay-success claim is made before in-game acceptance.

The accepted data rules are intentionally outside that runtime-performance rollback: fixed chest 10220122 remains present and its obsolete ignore is removed during upgrades; duplicate/offset 11003 remains ignored in favor of authoritative 14016.

The generated Mole catalog maps all 34 Fly records to their same-ID `DT_MiniGame_G5` reward treasure. The external save snapshot naturally preserves completion across restarts; raw reward bits bypass treasure display overrides such as the intentional 11003 duplicate suppression.

Assault reuses the existing `tb_actor_respawn` snapshot and shared Radar DAILY policy. The observed recovery boundary is 09:00 Korea Standard Time (00:00 UTC), independent of the player's local time zone. Save metadata is checked every two seconds only while F7 is active, and a changed slot must remain stable for four seconds before the low-priority SQLCipher worker runs. Continuous changes replace the pending fingerprint but cannot queue a second complete read inside the shared 45-second window. Parsed `.db` and `.bak` results are cached independently by exact session fingerprint; each cache miss builds one Treasure-rich entry for all layers. No additional cross-restart cache or reader is introduced.

The dev62 A/B log showed no growing Lua loop/process count and only about 4 MB of working-set growth before a plateau. Enabled-only save reads nevertheless had repeated cold tails, including 1.1-second and 8.46-second samples. All 69 recorded refreshes opened at least one database because the game rewrote `Slot1.bak` roughly every 20-30 seconds; there were zero cache-only refreshes. The first query on a fresh SQLCipher snapshot connection carried almost all elapsed cost, regardless of whether it was Treasure or Boss. Dev63 therefore preserves the cheap two-second metadata observation but coalesces intermediate fingerprints into one latest stable complete snapshot per 45-second window. Against the recorded 24-minute enabled span, the structural upper bound falls from 69 observed cold refreshes to about 33. This does not claim that the background read was the sole cause of game-frame stalls; runtime A/B remains required.

### Retained behavior

- 250 ms fresh current-player position sampling.
- 50 ms compact scalar-only flush loop with no UObject read.
- 8 ms active world-map transform production/presentation, stopped immediately when the map closes.
- 250 ms low-frequency control sampling.
- 33 ms active compact Overlay prediction/presentation, with event-driven bridge ingestion.
- Radius-derived half-pixel XY publication threshold with a 20-unit floor; 10-unit Z threshold.
- Cached Engine with reference-style current Controller/Pawn traversal; no Pawn wrapper survives across samples.
- Treasure selection and save filtering in C#.
- One-time hidden world-map prepaint without a fixed delay.
- No world-status rendering in expanded world-map mode and no full-screen redraw triggered by time changes.

### Scalar motion boundary

- The 250 ms control callback is the sole current-Pawn full-chain sampling site, targeting approximately four reads per second.
- Compact and visible large-map loops reuse the same numeric player sample; only the visible large map keeps its transform reads.
- Overlay prediction uses two protocol-v6 scalar samples. Epoch/enable/mode changes, timestamp regression, source gaps, and teleport/outlier distance reset history; prediction clamps at 250 ms and freezes after 500 ms stale age. F6 clears predictor history and holds one copied scalar snapshot until another control mode is observed.

### 1.8 work removed

- Lua Static JSON construction and one-second heartbeat writes.
- Overlay 200 ms Static file checks.
- `JavaScriptSerializer` and `System.Web.Extensions`.
- Static/Motion generation matching and fallback-state merging.
- Lua serialization of the fixed nine-Boss catalog.
- The obsolete `boss_tracker.lua` runtime module.

### Expected effect

The expected improvement is removal of unchanged compact bridge metadata probes from the visual cadence while increasing only bounded in-memory prediction from 20 Hz to roughly 30 Hz. Compact bridge reads should follow actual writes (about one per second stationary and at most about four per second while moving) plus a four-per-second safety scan. No fixed FPS gain is asserted from static validation alone. Same-session F7/F8 comparison using the same location, route, camera, and layer state remains authoritative.

### Remaining major cost and safety tradeoff

The Overlay is still a topmost layered WinForms/GDI+ window composed by DWM. If a substantial static FPS gap remains between enabled and F8-disabled states, further work should profile or replace the presentation path rather than alter Lua timing without evidence.

The current producer traverses cached Engine through the current Controller/Pawn once per 250 ms sample and retains only numeric scalars. Raising that UObject cadence, retaining Pawn/Controller wrappers, or moving bridge file writes into the game-thread callback is not an acceptable smoothness optimization. If a substantial F7-to-F8 gap remains, measure the layered-window/DWM presentation cost before changing safe producer timing.

### Secondary A/B and evidence

- `high_resolution_timer = false` uses the Windows default timer resolution; `true` requests `timeBeginPeriod(1)`. Restart the game between samples.
- `OVERLAY_LAYER_PERF` reports separate treasure, Boss, Mole/Fly, and world-status calls/counts/average/max milliseconds.
- `OVERLAY_WORK_PERF` reports the observed window-visibility sampling rate; the target is approximately 4 Hz outside forced mode transitions.
- `SAVE_REFRESH_PERF` reports snapshot copy, one-time key setup, Treasure query, Boss/Assault query, database reads/cache hits, whether Treasure publication was requested, how many Treasure queries actually ran, and total elapsed milliseconds.

### Validation

The source snapshot includes protocol positive/negative tests, C# source compilation gates for Windows PowerShell 5.1, Lua/source semantic checks, package mapping, manifest hashing, ZIP integrity checks, and bundled binary hashes. Windows compilation and in-game frame-time testing remain required.
