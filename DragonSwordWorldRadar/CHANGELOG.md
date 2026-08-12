# Changelog

## 0.4.0-dev74-minigamecatalog2

- Kept the established 250 ms control loop alive during bounded runtime recovery instead of replacing its active UE4SS Lua callback.
- Expanded the install-generated reward mini-game catalog to 83 records: 33 ordinary-world Fly, 40 Mole, and 10 Wave.
- Added allocation-free code-native hammer and wave markers for Mole and Wave activities.
- Bound Wave 13008 to the current PAK's exact mislabelled reward tuple `save_id=13008`, `DT_MiniGame_G5_13009` without introducing a general offset rule.

## 0.4.0-dev72-runtimewatchdog1

- Added bounded automatic F8-to-F7 lifecycle recovery for confirmed asynchronous Lua failures and a confirmed stalled 250 ms control loop. The existing compact or expanded-map presentation loop observes only scalar control-heartbeat progress once per second and requires five consecutive stale observations before recovery.
- Kept temporary expanded-map read loss on the dev71 candidate-only recovery path; missing map state does not arm whole-runtime recovery. The watchdog performs no UObject, bridge, drawing, game-thread queue, or file work and adds no fourth `LoopAsync` registration.
- Automatic recovery invalidates all pending tokens and UObject wrappers, publishes disabled state, registers one replacement control loop, and reuses the four-sample F7 stability gate. Three consecutive automatic failures fail closed instead of creating an unbounded restart loop.

## 0.4.0-dev71-worldmaprecovery1

- Replaced the two-second expanded-map rediscovery gap after an active read failure with one next-control-sample bounded rescan. The failed candidate wrappers are still dropped immediately, the candidate token is advanced, and no UObject survives the failure.
- Added a 250 ms visibility probe for at most two already-bounded hidden current-epoch candidates, allowing the first map opening and later reopenings to be detected without recurring UObject enumeration.
- Preserved the visible 8 ms expanded-map path, retired-identity protection, epoch/token travel invalidation, F8 suspension, and compact scheduling. Added structural gates that reject direct enumeration or wrapper retention in the recovery block.

## 0.4.0-dev70-clocklogfix1

- Corrected the debug-only `WORLD_TIME_TASK_PERF.failure` field so a successful one-shot clock capture records `none` instead of the misleading string `true`.
- Preserved thrown `pcall` errors and normal capture-failure reasons without changing clock capture, scheduling, publication, or normal-play execution.
- Added source and scheduling gates that reject the ambiguous Lua truthiness expression. All dev69 marker, control, cadence, bridge, and lifecycle behavior remains unchanged.

## 0.4.0-dev69-heightpairdebug1

- Extended the existing nearest-treasure height indicator to the two closest visible treasures in one allocation-free selection pass. The closest marker remains enlarged and labeled; the second-closest marker keeps the normal diameter and receives only its height indicator.
- Applied the same two-marker rule to the expanded map while preserving projected-pixel deduplication; an exactly overlapping second marker remains suppressed instead of producing indistinguishable duplicate geometry.
- Restricted F5 no-paint and F6 frozen-motion A/B hotkey registration to `debug_logging = true`. F7 and F8 remain the only registered normal-play controls, while the protocol and diagnostic implementations remain available for bounded debugging.
- Added deterministic nearest-pair and debug-hotkey guard tests without adding runtime queries, catalogs, timers, bridge traffic, or allocations.

## 0.4.0-dev68-moleoverridefix1

- Removed the obsolete stock `ignore 11003` treasure override after gameplay identification confirmed that the apparent overlap belongs to the valid MiniGame/Fly reward path.
- Added an exact installer migration that removes only the old `11003` ignore and its two stock explanation lines while preserving unrelated ignores and user-authored aliases.
- Added source and scheduling gates that prohibit the obsolete default rule. All dev67 performance scheduling and runtime behavior remain unchanged.

## 0.4.0-dev67-compactingest1

- Changed only compact Overlay prediction/presentation from 50 ms to 33 ms while retaining 250 ms fresh current-Pawn sampling, the 50 ms scalar-only Lua flush loop, the 1000 ms heartbeat, and the visible expanded-map 8 ms path.
- Replaced unchanged compact bridge polling with one slot-dirty `FileSystemWatcher` gate. Callbacks only coalesce a dirty bit; fixed-buffer parsing remains serialized on the UI thread, healthy notification loss is covered by a 250 ms dual-slot scan, and unavailable notifications fall back to 50 ms polling.
- Added coalesced slow-mode UI wakes for F7/F8/mode/epoch frames, disabled compact notifications during world-map presentation, added epoch-only visual invalidation, bounded partial-record retry, deterministic reader/gate tests, independent package byte/path/manifest auditing, and source-manifest build-output exclusions.
- Retired the dev30 cadence comparison as a performance acceptance gate. Runtime acceptance is now based on same-session F5/F6/F7/F8 and frame-time evidence; static/build/package gates do not claim a gameplay result.

## 0.4.0-dev66-inlineab1

- Added one-session compact-radar attribution controls: F5 suppresses custom Overlay painting while retaining the active producer/bridge/control/maintenance chain; F6 freezes the rendered scalar snapshot, skips prediction and motion-triggered repaint, and reduces control-only bridge reads to 250 ms; F7 restores the existing normal path; F8 remains complete-off.
- Upgraded the sole scalar Motion Bridge to protocol v6 with one strict three-value diagnostic enum so mixed source/Overlay packages fail closed.
- Added protocol, hotkey, no-paint, frozen-snapshot, timer, source-verifier, and package gates. Runtime smoothness attribution remains pending owner A/B testing.

## 0.4.0-dev65-hotpathopt1

- Removed normal-mode Lua F7 trace activation and eager trace-field table allocation while retaining the complete bounded trace when debug logging is explicitly enabled.
- Removed disabled Overlay performance bookkeeping, repeated per-marker UTC reads, interface-enumerator allocation in paint/query loops, unchanged encounter-availability rebuilds, and sub-half-pixel prediction invalidations without changing any feature, cadence, or marker precision threshold.
- Kept complete save snapshots and moved their worker into balanced Windows background mode; zero-only treasure categories are no longer materialized, and the diagnostic-only encounter-task table is queried only in debug mode.
- Preserved 250 ms fresh current-Pawn sampling, 50 ms compact presentation, 8 ms visible expanded-map presentation, direct vector rendering, all configured layers, and every fail-closed travel invariant.

## 0.4.0-dev64-debugoffab1

- Disabled detailed file diagnostics by default for a no-feature-loss runtime performance A/B. Normal-use lifecycle logging remains enabled.
- Preserved every dev63 radar layer, 50 ms compact presentation, 250 ms current-Pawn sampling, 8 ms visible world-map path, 45-second complete save-snapshot window, and fail-closed lifecycle boundary.
- Kept `diagnostic_verbose=false`; detailed diagnostics can still be enabled temporarily through the single authoritative `scripts/config.lua` followed by a game restart.

## 0.4.0-dev63-coldreadcoalesce1

- Coalesced continuous save fingerprints into one latest stable complete snapshot per 45-second window. The dev62 log opened a database on all 69 refreshes because `Slot1.bak` changed roughly every 20-30 seconds; the same 24-minute span is now structurally bounded to about 33 cold refreshes while two-second metadata observation remains enabled.
- Changed each permitted cache miss to build one Treasure-rich strict-superset snapshot for Treasure, Boss, Assault, and task state together. An unchanged window is served entirely from the independent `.db`/`.bak` caches.
- Retained immediate first-load, four-second stable-copy, below-normal worker, F8 invalidation, and request-scoped fail-closed publication boundaries. Added explicit requested-versus-executed Treasure-query diagnostics plus executable and structural scheduling gates.

## 0.4.0-dev62-treasuredeadlinefix1

- Kept treasure-rich cache entries compatible with encounter-only reads while making snapshot publication request-scoped: a narrow read reuses Boss/Assault and task data but never publishes cached treasure bits as a fresh treasure result.
- Prevented save-triggered encounter refreshes from indefinitely extending the 45-second treasure deadline. Only a request that explicitly includes Treasure and returns opened data may advance the next Treasure query time.
- Added executable regression coverage for narrow rich-cache reuse, retained encounter publication, and full Treasure publication.

## 0.4.0-dev61-savecacheconfig1

- Replaced the path-only save snapshot cache with query-shape-aware entries. Treasure-rich snapshots satisfy encounter-only reads, while the 45-second treasure pass and save-triggered encounter pass no longer evict each other for an unchanged `.db` or `.bak` sibling.
- Preserved exact database fingerprints, independent `.db`/`.bak` merging, the four-second change debounce, the 45-second treasure cadence, BelowNormal workers, and fail-closed publication; the optimization removes redundant SQLCipher work without weakening freshness or consistency.
- Restored visible expanded-map Lua transform production and Overlay presentation from the diagnostic 4 ms cadence to 8 ms. Compact presentation and player UObject sampling remain 50 ms and 250 ms respectively.
- Replaced the installed `config.default.lua` plus generated `config.lua` pair with one authoritative `scripts/config.lua`; runtime, Overlay, installer validation, diagnostics, and local deployment preservation now share that file.

## 0.4.0-dev60-minimapcachefix1

- Fixed the one-hertz `DLayerMiniMap` cache miss loop: only the retained top-level widget is checked with UObject `IsValid()`, while `LayerMap`, `MapOverlay`, and scale are read inside the existing protected access block.
- Missing or throwing nested-property access still fails closed and clears the top-level cache, but a usable nested UE4SS property wrapper is no longer rejected solely because its incompatible `IsValid()` result is false.
- Added regression gates that prohibit nested `LayerMap`/`MapOverlay` UObject validation while preserving one cache-miss-only `FindFirstOf("DLayerMiniMap")` site and phase timing diagnostics.

## 0.4.0-dev59-ue4ssroot1

- Standardized the runtime root on `DS/Binaries/Win64/ue4ss` and the Mod root on `DS/Binaries/Win64/ue4ss/Mods/DragonSwordWorldRadar`.
- Updated the Lua fallback resolver, installer Oodle fallback, local deployment helper, and documentation without changing F7 runtime scheduling or rendering behavior.

## 0.4.0-dev58-taskstateperf1

- Corrected the shared daily Boss/Assault recovery boundary from the player's local 09:00 to 09:00 Korea Standard Time (00:00 UTC), so availability is independent of the host time zone and daylight-saving rules.
- Added bounded, change-only diagnostics for the current save's `tb_unexpected_switch_week` rows, preserving `.db` and `.bak` provenance so the single-player encounter-task selection semantics can be established before production filtering changes.
- Changed inactive world-map detection from a 250 ms UObject validation path to notification-driven wake-up with a 2 second fallback probe. Visible expanded-map presentation remains at 4 ms.
- Split save work so the 49-record Boss/Assault query and task-table diagnostic remain on the fast save-change path while the full treasure query runs at most once per 45 seconds. No new game thread or UObject access was added.

## 0.4.0-dev57-stableactivation1

- Added a four-sample, 250 ms F7 stability gate that reads only the current player location before releasing map, clock, encounter, save, and fast-presentation work.
- Added per-encounter respawn diagnostics with actor ID, respawn type, destroy time, next availability, and hidden state.

## 0.4.0-dev56-assault60pct1

- Reduced Assault marker diameter to 60% of the corresponding world-boss marker in both minimap and world-map rendering.

## 0.4.0-dev55-worldmap4ms1

- Changed only the visible expanded-map Lua transform loop and Overlay presentation timer from 8 ms to a diagnostic 4 ms cadence for runtime A/B telemetry. Compact mode remains 50 ms and player UObject sampling remains 250 ms.

## 0.4.0-dev54-worldmapreopen1

- Added an explicit normal world-map close boundary that drops only retained widget candidates, increments their token, and preserves scalar map dimensions. Hidden prior-session widgets can no longer fill the bounded two-candidate cap and block later map opens.
- Reduced Assault marker diameter independently to roughly 78-80 percent of Boss marker size in compact and expanded-map modes.

## 0.4.0-dev53-stablecallback1

- Replaced request-specific compact-control game-thread closures with one stable Lua callback function and scalar request/epoch fields. The 250 ms sampling cadence and all UObject reads are unchanged.
- Retained the bounded F7 trace and enabled Assault reproduction path. Existing encounter lifecycle, save-state, condition, and draw diagnostics provide the staged attribution boundary without adding UObject work.

## 0.4.0-dev52-f7crashtrace1

- Added a six-second, 220-record, file-only F7 crash trace around existing asynchronous and UObject boundaries; it records scalar stage names only and performs no additional object query.
- Enabled Assault in this diagnostic deployment so the known F7 crash can be reproduced and attributed from the last durable trace record. No runtime behavior fix is claimed.

## 0.4.0-dev51-assaultfailsafe1

- Made `scripts/config.lua` the authoritative runtime Assault switch for both Lua and the Overlay. `config.default.lua` is only the installation template.
- Defaulted and upgrade-migrated `show_assaults=false` after the repeatable F7 native-crash A/B result; the generated 40-record catalog remains installed for further diagnosis.

## 0.4.0-dev50-unifiedencounter1

- Replaced the independent Boss and Assault runtime paths with one immutable 49-record world-encounter catalog, one `tb_actor_respawn` target set, one availability tracker, and one render traversal per surface.
- Moved all Boss/Assault catalog loading and save-filter configuration to Overlay startup. F7 no longer loads Assault data, changes the SQL target signature, resets the snapshot cache, or emits Assault-specific Lua lifecycle work.
- Preserved install-time current-PAK validation for nine Bosses and forty Assault targets. Optional encounter conditions remain record-owned and fail closed; the current build generates one fingerprint-pinned `world_time_window` condition, while unsupported weather semantics remain disabled.

## 0.4.0-dev49-assaultrestore1

- Re-enabled the install-generated 40-target Assault layer by default while preserving the dev48 one-shot clock, protocol-v5 scalar bridge, transition safety, and direct retained-vector rendering.
- Replaced the compiled Assault CID list in save SQL and diagnostics with the exact validated current-PAK catalog; the normalized target signature is part of the in-memory snapshot-cache identity and changes fail closed.
- Added rate-limited catalog, save-filter, availability-cache, respawn/time-gate, selection/draw-cost, F7/F8, epoch, and travel diagnostics. Time gates consume only the cached protocol-v5 local-clock scalar and perform no additional game-time UObject read.

## 0.4.0-dev48-clockrestore1

- Restored the compact game clock through the isolated one-shot provider: F7 waits for eight valid 250 ms player-context samples, reads `DGameSingleton.TimeOfDay` once, discards the wrapper, and advances the scalar locally at 60 game seconds per real second.
- Kept clock access outside the normal Pawn/map control task and added no polling loop, retry, recalibration, weather query, retained UObject, or transition-time lookup.
- F8 cancels clock work with all other Mod work. World travel hides the bridge temporarily but preserves a successfully captured numeric baseline.
- Preserved the dev47 minimap diagnostics and all current transition/performance behavior unchanged.

## 0.4.0-dev47-minimapdiagnostics1

- Added debug-only, phase-separated timing for the unchanged one-hertz
  `DLayerMiniMap` scale path. Logs now distinguish cache validation,
  `FindFirstOf`, child-object validation, scale access, result, and total time.

## 0.4.0-dev46-debugseparation1

- Separated file-based performance logging from in-map marker labels.
  `debug_logging=true` now records diagnostics without drawing coordinate or
  identity text; only `diagnostic_verbose=true` enables those visual labels.

## 0.4.0-dev45-debugbaseline1

- Enabled file-based debug diagnostics by default for the remaining
  pre-release performance tests. In-map verbose marker labels remain disabled
  by default and debug logging must be disabled again for the final release.

## 0.4.0-dev44-worldmapepochbound120hz1

- Retained dev43's epoch-bound two-candidate world-map ownership and tokenized loop/callback lifecycle.
- Restored visible expanded-map transform and Overlay presentation to 8 ms without changing the 250 ms player full-chain sampler or 50 ms compact cadence.
- Restored idempotent world-map-only `timeBeginPeriod(1)` acquisition and balanced release on map exit, suppression, F8, or form close; the startup-wide user option is never double-acquired.

## 0.4.0-dev43-worldmapepochbound1

- Replaced the unbounded retained world-map widget list with a two-candidate lifecycle-epoch/token-owned set. Reset drops all wrappers, retires bounded scalar identities, and prevents later scans from promoting previously retained old-world widgets merely because UE GC still reports them valid.
- Added unique compact/world LoopAsync serial ownership plus request tokens for queued control/world callbacks. Old instances exit on token mismatch and cannot clear or mutate a newer pending gate; logical compact/world ownership transitions directly with no overlap.
- Returned visible world-map production/presentation from 8 ms to 16 ms and removed dev42's transient mode-scoped timer-resolution request. The existing user-config startup A/B remains unchanged.
- Preserved dev42's fingerprint-bound install-generated save-owner RVA and one-shot initial snapshot optimization. Structural accumulation is removed, but attribution of the reported progressive slowdown remains a runtime hypothesis pending repeated-travel acceptance.

## 0.4.0-dev42-installrva-worldmap120hz1

- Added install-time exact-EXE owner-pointer pattern resolution, bound only to the locked pre/post game fingerprint and persisted as RVA/provenance without the SQLCipher key. Runtime validates the current EXE+PAK fingerprint before trying the generated RVA, then retains known RVAs and one delayed pattern fallback.
- Allowed only the first save snapshot to bypass the four-second change debounce because the worker still requires a stable source fingerprint before/after its consistent copy; later save changes retain the debounce.
- Raised only the visible expanded-map transform and Overlay presentation from 16 ms to 8 ms. The same WinForms timer is reused, compact mode remains 50 ms, and player UObject sampling remains one fresh 250 ms scalar read.
- Added idempotent, mode-scoped `timeBeginPeriod(1)` acquisition and balanced release on world-map exit/disable/close, without double-acquiring when the existing startup-wide option is enabled.

## 0.4.0-dev41-worldmap60hz1

- Raised only the visible expanded-world-map transform producer and Overlay presentation timer from 50 ms to 16 ms for lower dragging latency.
- Kept the dev40 safety boundary: one fresh Engine-to-current-Pawn numeric sample per 250 ms, no player reread from the world-map loop, no retained Pawn or Controller wrapper, bounded scalar prediction, forced Assault isolation, and zero clock work.
- Added bounded debug aggregates for world-map producer rate, world-map presentation rate, and deterministic mode-transition count; the existing single timers change cadence in place and stop/restore on map exit.

## 0.4.0-dev40-scalarmotion1

- Based on user-tested dev38: Assault remains force-isolated and production clock access remains disconnected.
- Removed the compact 50 ms full player-root traversal. One fresh, non-retained player sample now occurs in the 250 ms control callback; compact and large-map loops reuse the numeric sample.
- Protocol v5 adds world epoch and sample timestamp. The 50 ms Overlay predicts from two scalar samples, resets on lifecycle/outliers, clamps at 250 ms, and freezes after 500 ms stale age.
- Added full-chain sample-rate and prediction age/clamp/reset/stale diagnostics. In-game acceptance remains pending.

## 0.4.0-dev39-assaultgateclockwall1

- Based exactly on the separately packaged dev38 forced Assault-isolation stage.
- Added one serialized clock-baseline attempt per F7 after eight valid 250 ms context samples. F7 only arms state and the capture task replaces one normal control sample for isolation.
- Added DataProbe-style protected singleton validity and scalar field reads, immediate wrapper release, no retry/recalibration, and stale-token rejection before native access.
- Changed local advancement to nonnegative `os.time()` wall elapsed at 60x, so delayed/skipped callbacks do not slow the clock. Static tests do not prove native crash safety.

## 0.4.0-dev38-assaultisolation2

- Made packaged dev30 the explicit owner-accepted performance/smoothness baseline while retaining the later fresh-current-Pawn, no-cross-frame-wrapper travel-safety correction.
- Added a missing-default-true Assault isolation gate in both Lua and Overlay, so an older preserved `show_assaults=true` cannot reactivate catalog/availability maintenance, traversal, drawing, or Assault-only save diagnostics.
- Retained dev37's zero-production-clock behavior to isolate the owner’s first performance test from clock stability.
- Added debug-only split timing for current-root traversal versus `K2_GetActorLocation`. Static validation does not claim dev30 performance parity.

## 0.4.0-dev37-clockrollback1

- Removed the production `world_environment` import and every arm, refresh, cancellation, and travel callback after the delayed F7 `DGameSingleton.TimeOfDay` read reproduced an uncatchable native UE4SS crash.
- Forced world status fail-closed even when an older preserved user configuration still requests it. Marker, save-state, debounce, snapshot-cache, worker-priority, and F8 suspension optimizations from dev36 remain intact.
- Added source and scheduling gates that fail the build if production reconnects the unsafe provider.

## 0.4.0-dev36-saveclock1

- Suspended save metadata and SQLCipher work behind the F7/F8 master gate; F8 invalidates queued publications and performs no new save refresh.
- Added a four-second stable-change debounce and two-second metadata cadence before save snapshot work.
- Added per-database fingerprint caching so an unchanged `.db` or `.bak` reuses its parsed snapshot instead of being copied and decrypted again.
- Applied the SQLCipher key once per isolated temporary snapshot connection and moved refresh workers to best-effort below-normal priority.
- Added debug-only copy, key, treasure-query, Boss/Assault-query, cache-hit, and total save-refresh timings.
- Restored the clock with one delayed `DGameSingleton.TimeOfDay` baseline read per F7 activation. No UObject is retained and no retry or recalibration read occurs; local wall time advances the cached value at the confirmed 60x rate.
- Routed the conditioned Assault visibility rule through the same cached/extrapolated protocol-v4 time value.

## 0.4.0-dev35-clockfailclosed1

- Removed the production `DGameSingleton.TimeOfDay` provider after an F7 startup sample reproduced an uncatchable native UE4SS C++ exception immediately after the otherwise successful player/minimap reads.
- World-status protocol fields remain wire-compatible but fail closed as unavailable; the clock is hidden until a non-UObject provider exists.
- Preserves the dev34 unified Assault marker design and the dev33 reference-style Pawn-loss cooldown unchanged.

## 0.4.0-dev34-assaultstyle1

- Unified Assault markers with the Boss marker visual system: equal map and minimap sizing, retained diamond shadow/backing/inner border, and allocation-free gold crossed swords for distinct Assault semantics.
- Retains the validated dev33 world-transition recovery behavior unchanged.

## 0.4.0-dev33-recoveryrollback1

- Removed the dev26-dev32 dedicated world-identity recovery callback after an F8 A/B isolated the repeated dungeon crash to the active Radar lifecycle.
- Restored the proven 1.6.1 cooldown policy: three seconds with zero UObject work, followed by one normal player-location sample; every failed sample rearms the complete cooldown.
- Removed recovery-time `Pawn:GetWorld()` / `World:GetFullName()` reads and the two-sample identity state while preserving world epochs, stale-callback rejection, fail-closed bridge output, and current-Pawn sampling.
- Kept cross-frame Pawn caching disabled because its performance benefit is not independently measured and it would add a separate stale-wrapper risk.

## 0.4.0-dev32-assault1

- Added 40 install-generated Assault markers on both map surfaces using shared `tb_actor_respawn` state.
- Added generic generated time-window conditions from a versioned fingerprint-pinned inference-policy input, with exact fresh target/cycle resolution, confirmed-cycle/inferred-binding provenance, and fail-closed unavailable-time behavior; weather remains excluded.
- Bound every generated dataset and Assault catalog record to the pre-generation game fingerprint and reject installation if the post-generation identity changes.
- Added a retained warm-orange crossed-swords diamond vector layer with independent diagnostics and no bitmap cache.
- Preserved protocol v4, one bridge/save reader, existing cadence, and the then-current dev31 travel lifecycle.

## 0.4.0-dev25-secondary1

- Reduced foreground, visibility, and minimized-window API sampling to 250 ms while keeping cached visibility decisions on every Overlay tick; F7/F8 and radar/world transitions force an immediate fresh sample.
- Added the startup-only `high_resolution_timer` A/B setting. It defaults to `false`; `true` requests `timeBeginPeriod(1)` and retains balanced `timeEndPeriod(1)` cleanup.
- Added debug-only per-layer draw calls, rendered marker counts, average time, and maximum time for treasures, Bosses, and Mole/Fly markers alongside the existing world-status and total-paint evidence.
- Added debug evidence for the actual window-visibility sampling rate and corrected world-map Mole timing to record once per layer pass instead of once per marker.
- Preserved dev24's 50 ms motion/paint cadence, half-pixel compact-motion threshold, one-second maintenance/save checks, reward-bit Mole/Fly completion, and all dev21 map/index correctness behavior.

## 0.4.0-dev24-motion20hz1

- Capped the Lua radar/world-map producers and active Overlay consumer at a coherent 50 ms (20 Hz) cadence to reduce moving bridge, paint, and layered-window composition work.
- Suppressed compact player motion smaller than half a projected radar pixel, using a radius-derived threshold with the accepted 20-unit safety floor; no world-map transform or treasure-index cache was added.
- Reduced save-slot metadata checks and Overlay maintenance to one hertz, and stopped filesystem metadata polling for installer-generated treasure/Boss catalogs after their first validated load.
- Added debug-only visual, invalidate, paint, refresh-duty, paint-duty, and composed-pixel-rate evidence for runtime A/B evaluation.
- Preserved the dev21/dev23 map ownership, save-filtered treasure index, direct vector rendering, 34 reward-bit Mole/Fly filter, F7/F8 master gate, and one-second nearest-treasure selection.

## 0.4.0-dev23-molereward1

- Mapped all 34 Fly activities `11001-11034` to their exact same-ID `DT_MiniGame_G5` reward treasure records.
- Reused the existing external `tb_treasure_box.OPENED_BIT_FIELD` snapshot to hide a Fly marker after its reward is claimed, without any game UObject query.
- Added a raw save-bit path isolated from treasure ignore/alias overrides so the treasure-specific `11003` ignore rule cannot hide Fly `11003` incorrectly.
- Added fail-closed startup, full 34-bit visibility, first/last claimed reward, and `11003` override-bypass regression coverage.

## 0.4.0-dev22-molequerysafe1

- Disabled `DETUtil.CIsClearMiniGameInStandAlone` resolution and invocation after three reproducible native UE4SS crashes immediately following F7 activation across quest/world lifecycle transitions.
- Kept the validated 34-record Mole/Fly catalog and publishes all markers while completion state is unavailable; treasure, Boss, map, clock, protocol, and rendering behavior are unchanged.
- Added negative structural validation preventing the unsafe UObject query path from returning unnoticed.

## 0.4.0-dev21-dev15rollback1

- Restored the packaged dev15 runtime behavior for treasure visibility indexing, compact map ownership, nearest-query buffering, Overlay diagnostics, native window interop, and Boss/Mole vector rendering.
- Removed all dev16 composition-region behavior and all dev18-dev19 retained-map, map-key, and radar-map debug behavior.
- Retained only the approved 33 ms Lua/Overlay active cadence, 1000 ms nearest selection, and whole-mod F7/F8 clock lifecycle.
- Moved the transparent 24/15-reference-font time/phase group into a six-pixel-gapped 52-pixel strip below the 360-pixel compact minimap square.
- Preserved the accepted data rules across the rollback: fixed chest 10220122 is not ignored and its obsolete ignore is removed on upgrade, while duplicate/offset record 11003 remains ignored in favor of authoritative 14016.

## 0.4.0-dev20-vectorrenderrollback1

- Removed the dev17 Boss/Mole bitmap marker caches, cache blit paths, runtime vector toggle, and cache-specific diagnostics completely.
- Restored direct retained-geometry vector rendering on every Boss and Mole marker paint, matching the packaged dev16 behavior.
- Added executable vector-renderer smoke coverage and negative structural gates preventing the removed cache symbols from returning.

## 0.4.0-dev19-treasurecorrectness1

- Restored fail-closed treasure visibility until the first valid save snapshot, removing the temporary full-catalog layer and its already-open markers.
- Restored the supported compact radar to map 100 and removed generation-retained world-map state that could survive teleports or region changes; expanded world-map rendering remains keyed to the current frame map ID.
- Added map ID to the nearest-treasure query-cache key so a source-map change always rebuilds before the normal one-second interval.
- Added executable regressions for unknown-save hiding, ready-save filtering, map-key invalidation/reset, fixed compact ownership, and unchanged world-map map ownership.

## 0.4.0-dev18-masterab5-startupfailopen1

- Changed startup treasure visibility from an empty unknown-save index to the complete catalog grouped by map, preventing blank compact/world-map treasure views while the first save snapshot is pending.
- Rebuilds atomically as soon as the first save snapshot becomes available or its version advances, filtering confirmed-open IDs. Already-open boxes may briefly appear during startup.
- Added executable unknown-save/full-catalog and loaded-save/opened-filter regression coverage while preserving generation-scoped compact map retention.

## 0.4.0-dev18-masterab4-mapretention1

- Fixed compact treasure selection to use the most recently observed valid world-map map ID instead of hard-coded map 100, with a safe pre-observation default and generation-scoped reset that preserves F8/F7 state.
- Added bounded debug-only radar map evidence for treasure candidates/selection and Mole catalog/mask/range counts.
- Confirmed no Mole map-split defect in current data: all 34 generated records are map 100, world-map rendering already filters by frame map ID, and compact rendering correctly evaluates all records by mask and distance.

## 0.4.0-dev18-masterab3

- Corrected installer completion guidance and default configuration comments to state that F7 enables the complete configured mod including world status and F8 disables every mod feature for FPS comparison.
- Added source gates that reject the obsolete marker-only F7/F8 installer claim.

## 0.4.0-dev18-masterab2

- Reduced hidden master-disabled Overlay bridge polling from 125 ms to 500 ms while retaining the one-second game-lifetime shutdown check.
- Skipped periodic window geometry placement while disabled; the first transition back to radar/world mode still forces immediate placement.

## 0.4.0-dev18-masterab1

- Restored F7/F8 as a whole-mod runtime A/B gate: F7 enables configured markers and world status; F8 hides everything and stops radar/world-map producers, Mole/Fly queries, and world-time work after one disabled lifecycle publication.
- Preserved configured layer choices without rewriting `config.lua`; context recovery cannot bypass the F8 master-disabled state.
- Removed the active status-after-F8 control and Overlay presentation paths while retaining one process, one window, and protocol-v4 bridge.
- Closed cross-restart Mole/Fly persistence by design. Each launch performs the bounded fail-closed initial scan, and the existing session permanently stops completion calls after all 34 are complete.

## 0.4.0-dev17-moving33-markercache1

- Changed Lua radar/world-map motion producers and active Overlay polling from 24 ms to 33 ms without adding adaptive logic.
- Changed nearest unopened radar treasure selection from 250 ms to 1000 ms; selection continues to consume only the save-filtered active index and keeps the 80-point cap.
- Added retained DPI/scale-keyed Boss and Mole bitmap caches with allocation-free blits, automatic vector fallback, and `EVENTRADAR_VECTOR_MARKERS=1` rollback.
- Added cache blit/vector/rebuild/time diagnostics. Runtime FPS benefit remains an in-game A/B gate.
- Removed the obsolete stock `ignore 11003` rule and its comments. User override files remain owned and preserved.
- Did not add unsafe cross-restart Mole completion persistence: the Lua completion producer has no stable save/account identity, while the Overlay-only database stem is not a proven account identity and is not available through protocol v4.
- Recorded the future Boss/Assault permanence contract: persist next-due time only when keyed to a proven save identity; do not query before due after restart.

## 0.4.0-dev16-compositionregion1

- Limited compact Win32 composition exposure to the union of the complete right-hand radar square and a padded left status rectangle; status-only mode excludes the radar region and world-map mode restores the full rectangle.
- Applied region changes only on presented-mode or geometry/scale transitions, with correct transferred HRGN ownership and rectangular fail-open behavior.
- Added change-only debug region mode, approximate visible-pixel area, and ratio evidence explicitly labeled as not equivalent to DWM GPU savings.
- Preserved all dev15 producer cadence, adaptive behavior, marker caching, treasure selection, Mole scheduling, world-time scheduling, bridge, process, and window counts. Runtime FPS effect remains unvalidated.

- Preserved the bounded five-per-callback initial Mole/Fly snapshot and replaced four-per-second steady polling with a paced full sweep of unfinished IDs across one minute, never exceeding one UObject query per 250 ms callback.
- Added gameplay-context suspension, permanent all-complete shutdown, and bounded 10/30/60/120/300-second failure backoff while preserving the fail-closed mask.
- Replaced one-hertz `TimeOfDay` reads with local 60x extrapolation accumulated from the established 250 ms control callback and source recalibration once per in-game hour. This avoids portable-Lua `os.clock`/wall-clock assumptions; the 60x rate remains runtime-gated.
- Suppressed exact-seconds-only bridge writes, repaired Mole debug diagnostics, added aggregate layer timing, cached the debug Fly font, and moved research-only Assault detail to debug.
- Made world time automatic and independent of F7/F8 using the existing 250 ms control path; status-only mode never starts 24 ms motion production.
- Extended the compact transparent overlay 170 reference pixels left, kept the original 360-pixel minimap square fixed on the right, and placed a clamped 150-pixel horizontal status group 12 pixels left of the minimap circle.
- Increased reference time/phase fonts to 24/15 pixels and retained full-screen world-map omission.
- Fixed context-loss and `mods.txt=0` lifecycle frames to clear published time/weather availability before serialization, preventing stale status display.
- Centralized the F7 marker predicate as treasures, bosses, or Moles; world status and treasure-only height/type decorations cannot start 24 ms motion production.

## 0.4.0-dev14-timephase1

- Removed user-facing `W/BT` values and stopped querying the invariant weather manager scalars.
- Enlarged the transparent clock and circular icon while keeping the group against the extreme top-right edge.
- Added retained phase-specific visuals and labels for `MORNING` (06:00), `AFTERNOON` (12:00), `EVENING` (18:00), and `NIGHT` (21:00), with night continuing until 06:00.
- Reduced world-status sampling from three scalar properties to one cached `DGameSingleton.TimeOfDay` read per second.
- Added exact boundary tests for all four time phases.

## 0.4.0-dev13-molecatalogfix1

- Fixed the production Overlay catalog parser to recognize Lua Boolean and `nil` literals, matching the owner-tested standalone Mole parser.
- Restored parsing of generated `has_z = true` fields so all 34 validated Fly records can load instead of being skipped at runtime.
- Added a 34-record parser-contract regression test covering unique IDs, contiguous mask bits, and Boolean `has_z` values.
- Added source verification markers that reject removal of the shared Boolean-literal parsing contract.

## 0.4.0-dev12-worldstatus-layout1

- Removed the opaque rectangular world-status background and border so only the dial, weather glyph, and outlined text remain visible.
- Moved the circular day/night dial to the extreme top-right and placed the time/weather text to its left, reducing overlap with the circular radar map.
- Kept the same cached one-hertz sampler, protocol-v4 record, minute-level redraw filtering, and retained rendering resources.

## 0.4.0-dev11-worldstatus-singlebridge1

- Added a compact time and weather-status panel to the upper-right of the radar window; it is not drawn over the expanded world map.
- Added read-only, cached scalar sampling of `DGameSingleton.TimeOfDay`, `DsEnvironmentManager.CurrentWeatherState`, and `CurrentWeatherBTState` at one sample per second inside the existing 250 ms control callback.
- Kept startup time fail-closed until a nonzero clock value has been observed, after which midnight zero is accepted.
- Displayed weather as neutral raw `W` and `BT` values because the current evidence does not yet prove semantic labels such as sunny or rainy.
- Extended the sole Motion/control bridge to protocol v4 with 35 fields; legacy and malformed records remain rejected.
- Added retained drawing resources and visual-delta filtering so unchanged status values do not create extra redraw work.

## 0.4.0-dev10-molefly-singlebridge1

- Integrated the owner-tested 34-record `MiniGame_Fly` layer into the production Radar without adding a Host, watcher, Overlay, scheduler, or bridge.
- Added install-pinned Fly catalog extraction with exact classification, stable IDs, contiguous mask bits, section/map indexing, and fail-closed shape validation.
- Added bounded read-only completion checks through `DETUtil.CIsClearMiniGameInStandAlone`: five initial reads and one unfinished read per existing 250 ms callback.
- Extended the single Motion/control bridge to protocol v3 with 29 fields for `showMoles` and the 34-bit visibility mask; older records are rejected.
- Added a retained, code-native winged upward-arrow renderer for minimap and world-map Fly markers.
- Preserved treasure, Boss, save-key startup waiting/recovery, F7/F8 lifecycle, and accepted 24 ms active cadence.

- Rebased the performance work on the accepted 1.7 Stable Core cadence: 24 ms minimap/world-map motion, 250 ms control sampling, 24 ms active Overlay polling, and 20/10 XY/Z thresholds.
- Removed the active Static JSON Bridge, its one-second Lua heartbeat write, the Overlay's 200 ms Static file poll, `JavaScriptSerializer`, and static/motion fallback merging.
- Introduced explicit Motion protocol v2 with 27 strictly validated fields, alternating slots, generation/sequence ordering, and leading/trailing sequence equality.
- Moved fixed treasure and nine-Boss catalog ownership to the Overlay; Lua publishes only live UObject/control state.
- Added a 2500 ms Motion stale timeout that hides obsolete output and automatically restores on the next valid frame.
- Kept the effective immediate world-map transition: hidden no-copy resize, one synchronous prepaint, then immediate reveal; closing the map immediately restores the small radar.
- Installer upgrades remove only the three obsolete 1.7 source files that folder overwrite cannot delete.
- Preserved the save-startup visibility gate, buffered snapshot flush, delayed full-EXE key scan, and exact process-bound watcher lifecycle.

## 0.4.0-dev9-performance1.4-30hz1

- Synchronized the Lua minimap/world-map motion producers and the active Overlay consumer at 33 ms (~30 Hz); the prior source-only 33 ms Overlay edit had left the runtime producer at 24 ms.
- Kept static state at 250 ms and retained 50/75/125/500 ms idle, disabled, and background Overlay intervals.
- Removed the obsolete default `ignore 10220122` rule because the current game data exposes that chest again.
- Installer upgrades now remove only the exact deprecated `ignore 10220122` rule from a preserved `data\treasure_overrides.txt`; other user overrides are preserved.
- Session-overlap audit confirmed one resident hidden WScript watcher, one game-bound host, mutex-protected duplicate host startup, and clean host/Overlay teardown when the game exits. No extra runtime watcher process is created by F7/F8 toggles.
- No bridge schema, Boss rule, marker style, save schema, or world-map lifecycle change.

## 0.4.0-dev9-performance1.3-mapinstant-hiddenhost1

- Removed the 1000 ms world-map reveal delay and all associated warm-up state; map entry and exit now restore the Overlay in the same timer cycle.
- Kept hide-before-resize, added `SWP_NOCOPYBITS`, and synchronously repaints the newly sized hidden surface before `SW_SHOWNOACTIVATE`. A paint-sequence check skips the second full-screen invalidate only when hidden prepaint actually completed. Debug mode records `MAP_SURFACE_PREPARED` timing. This is a zero-timer attempt to suppress stale layered-window pixels without retaining a full-screen surface outside map mode.
- Restored the proven resident hidden WScript launcher used by the stable baseline. Game-time Lua now writes only `runtime\launch.request` and no longer invokes `cmd.exe`, `wscript.exe`, or PowerShell, eliminating the transient console-window path.
- The installer recreates and validates the hidden user Startup watcher; the game-bound PowerShell host remains hidden and exits with the exact game process.
- Classified the expected pre-login save-key-not-ready state as Debug-only instead of writing a normal-use stack trace.
- The diagnostic game executable is 162,551,704 bytes; normal startup no longer reads it before the known RVAs have had 30 seconds to resolve.
- Launch request stamps are digits-only and newline-free; WScript validates them before command construction, fixing the observed broken WatcherHost log lines and reducing parameter-injection surface.
- Changed save-key discovery to try cached/current/legacy RVAs first. A full game-EXE signature scan is now a one-time worker-thread fallback after 30 seconds; transient owner/key-not-ready states neither trigger it early nor disable it, and the Overlay UI thread is never blocked by that scan.
- No bridge fields, treasure/Boss semantics, save schema, marker style, F7/F8 behavior, or active map-producer intervals changed.

## 0.4.0-dev9-performance1.2-mapfix-debug1-cleanup1

- Removed Overlay runtime polling for `debug_logging`/`diagnostic_verbose`; the setting is parsed once at process startup, so normal Timer ticks and debug-log guards perform no config-file stat/read work.
- Removed runtime Boss RespawnCycle PAK enumeration, binary-to-text decoding, regex parsing, the 30-second debug rescan path, and the associated locks/state.
- Preserved Boss availability semantics with the verified fixed rule-106 schedule: daily reset at 09:00 local time.
- Removed the save-worker call that existed only to refresh the optional Boss rule scanner.
- Added source-verifier assertions that reject reintroduction of Debug hot-reload polling and runtime Boss PAK scanning.
- No bridge schema, save-table interpretation, marker behavior, F7/F8 behavior, or map lifecycle logic changed.

## 0.4.0-dev9-performance1.2-mapfix-debug1

- Removed the always-full-client radar window introduced after `performance1.1`; radar mode again uses an actual small top-right layered window.
- Changed world-map lifecycle handling to hide the Overlay before resizing, keep it hidden for 1000 ms after every map entry, and restore the small radar immediately on the first confirmed map-close read.
- Removed the 96 ms always-on map UObject detector, three-miss exit confirmation, and 250 ms stale re-entry block from the 24 ms radar motion path.
- Restricted 24 ms world-map UObject sampling to the period in which the world map is actually active, and reused the active transform in the 250 ms static publisher.
- Made Lua game-thread queue/callback gates recover after scheduling or callback exceptions instead of remaining permanently pending.
- Split Lua and Overlay output into low-volume `Use` logs and opt-in `Debug` logs. Normal mode no longer computes or writes periodic performance diagnostics.
- Expanded debug diagnostics with producer queue/update latency, 50/100/250 ms stall counts, motion sample/write rates, Overlay timer/paint gaps, normalized process CPU, working set, visibility, and geometry. `overlayPaintFps` is explicitly labeled as an Overlay metric rather than game Present FPS.
- Reduced optional Boss respawn-rule PAK discovery to one scan per Overlay process in normal mode; periodic rescans remain available only in debug mode. PAK enumeration failures now preserve the daily 09:00 fallback and cannot abort save-state refresh.
- Added transient game-process and game-window failure recovery, log-session rotation, and exact source-verifier rules for the repaired map lifecycle.
- Hardened Overlay launch retry and serialized, before/after-consistent debug-config reads across UI/save worker threads; the normal timer checks for debug-mode changes only once per second.
- Removed the no-op Boss tracker call from the 250 ms static-state build.
- Excluded generated `runtime`/patch-deployment backups from the source-only unexpected-EXE scan while retaining SHA-256 enforcement for the single bundled `ooz.exe`.
- Static validation was completed in Linux. Windows PowerShell 5.1 `Add-Type` compilation and in-game FPS/transition validation remain mandatory deployment checks; `Install.cmd` must print `OVERLAY_COMPILE_OK`.

## 0.4.0-dev9-performance1.1

- Reduced compact-motion bridge writes through cumulative visual-delta filtering while retaining a one-second liveness heartbeat.
- Changed active minimap and world-map sampling to 24 ms without changing the fixed 21-field bridge protocol.
- Added adaptive Overlay polling at 24/50/75/125 ms for active, world-idle, radar-idle, and disabled states.
- Prevented unchanged bridge slots, heartbeat-only motion frames, and visually equivalent static frames from causing repeated reads, parsing, invalidation, or paint.
- Batched non-nearest treasure markers into retained type-specific `GraphicsPath` instances.
- Reused a validated game process ID across geometry, lifetime, and save polling.
- Added scheduler/write-suppression counters to Lua and Overlay diagnostics.
- Fixed the Windows PowerShell 5.1 CodeDOM local-variable shadowing error in `MotionVisualSnapshot.Update`.
- In-game diagnostics confirmed normal operation, adaptive timer transitions, zero bridge/write failures, and materially lower idle/active work.

## 0.4.0-dev8-refactor2

- Fixed the refactor1 Windows PowerShell 5.1 `Add-Type` compiler blocker.
- Added mandatory real-compiler and regression-harness gates to release builds.
- Added SHA-256 package integrity validation and exact immutable-tree checks before installation.
- Made datasets, preserved config, overrides, metadata, `mods.txt`, and startup shortcut one staged, rollback-protected transaction; watcher readiness is the commit point.
- Isolated treasure and boss save-state failures, retained last-known-good snapshots, and added bounded partial-module retry.
- Added bounded bridge/catalog reads, PAK/decoder guards, canonical ZIP validation, and per-record rendering isolation.
- No intended feature, marker-style, bridge-protocol, schema, hotkey, or sampling-interval change.

## 0.4.0-dev7-stable6

- Frozen dev7 stability baseline after in-game validation of F7/F8, minimap and world-map double buffering.
- Restored the complete original dev7 rendering implementation before applying narrowly scoped visibility and marker changes.
- Overlay remains active after F7 until F8; foreground, overlap, and minimized-window visibility polling are removed.
- Host and watcher processes use a temporary working directory rather than holding the Mod folder as their current directory.
- Main Radar contains no experimental Boss, treasure, or sudden-mission collection hooks.
- Treasure and Boss marker colors share `RadarMarkerStyle`; Boss uses the accepted warm-red visual.
- Treasure outlines use the shared dark outline. World-map paths use `FillMode.Winding` and projected-pixel deduplication.
- Installer validates/repairs `config.lua` and compiles the exact complete Overlay source set before installation succeeds.

## 0.4.0-dev5

- World-map refresh now runs at 8 ms while dragging or zooming and returns to 16 ms after 250 ms of stable input.

- Reduced radar motion sampling from 16 ms to 32 ms while retaining the 16 ms world-map producer.
- Producer loops now terminate completely while disabled and restart only on F7.
- World-boss Character discovery runs at most every 5 seconds and only near a known spawn.
- Installer now changes only the DragonSwordWorldRadar entry in mods.txt.

- Added a local-PAK `BossDataProvider` that generates exactly nine world-boss records.
- Added independent UE4SS world-boss runtime tracking and bridge payloads.
- Added a larger unified marker modeled on the game's field-boss map icon to the minimap and world map.
- Added death/disappearance hiding and actor-respawn restoration for previously observed bosses.
- Added independent `show_bosses` and `show_treasures` rendering controls.
- Changed the treasure player-height reference offset from `-120` to `-150`.
- Fixed release packaging to include the validated bundled `tools/ooz.exe`.
- Updated source verification to allow only the declared `ooz.exe` tool binary.

## 0.3.2c

- Established the standalone DragonSwordWorldRadar repository and release layout.
- Removed the installation dependency on DragonSwordTreasureMap 1.6.1.
- Added one-click local treasure-data extraction and generation.
- Added a reusable `IDataProvider` pipeline for future radar layers.
- Added game-version fingerprinting and launch-time reinstall prompts.
- Preserved the completed treasure layer.

## 0.4.0-dev31-freshpawn1

- Removed the cross-frame Pawn cache after a dungeon transition crashed in UE4SS at the exact `player_access_failed` boundary.
- Matched the supplied 1.6.1 player path: cache Engine only, then resolve the current Controller and Pawn for every position sample.
- Added regression gates that reject any retained `player_pawn` wrapper while preserving zero `FindFirstOf`/`FindAllOf` calls in the stable position path.
- Preserved delayed two-sample World recovery, direct rendering, and existing 50/250/1000 ms scheduling.

## 0.4.0-dev30-referencecache1

- Replaced repeated stable-path root acquisition with the supplied 1.6.1 cached Engine -> Controller -> Pawn policy; Engine lookup now occurs only on cache miss/recovery.
- Restored the supplied 1.6.1 cached `DLayerMiniMap` scale reader; the widget is re-found only after absence/invalidity.
- Completely removed the custom task/cutscene HUD visibility scan and `surface_hidden` state machine, including `IsVisible`, `DPanelMain`, and `Overlay_Minimap` access.
- Kept world-epoch fail-closed recovery, F7/F8 master control, expanded-map handling, and 50/250/1000 ms scheduling without adding object enumeration.

## 0.4.0-dev29-pawnvisibility1

- Removed the dev26-dev28 UE4SS `LoadMap` pre/post hooks after two identical startup crashes and restored the stable 1.6.1 Pawn-loss plus delayed-widget-rescan lifecycle.
- Retained world epochs and queued-callback rejection, but recovery now begins after root/Pawn loss, waits three seconds, and requires two matching fresh World identities.
- Replaced the 250 ms `DPanelMain`/`Overlay_Minimap`/`DLayerMiniMap` visibility chain with one freshly acquired and immediately validated `DLayerMiniMap` sample per second.
- Kept hidden-HUD bridge shutdown and expanded-world-map independence without retaining a compact HUD widget wrapper or adding a scheduler.

## 0.4.0-dev28-hudvisibility1

- Added a fail-closed compact-surface gate using the existing 250 ms control callback and current `DPanelMain`, `Overlay_Minimap`, and `DLayerMiniMap` visibility.
- Hidden cutscene/menu HUD now publishes one `surface_hidden` bridge frame, hides the Overlay, context-disables world-time/Mole work, and stops the compact 50 ms producer until the HUD returns.
- Kept expanded world-map detection and rendering independent from compact minimap visibility, including a fail-closed map-close handoff.
- Added executable/static HUD-surface scheduling contracts without adding a fourth Lua scheduler or changing protocol v4.

## 0.4.0-dev27-transition2

- Independently reviewed the dev26 transition lifecycle against the repeated UE4SS access-violation evidence and the installed UE4SS LoadMap API.
- Removed unused retained Engine/PlayerController wrappers and stopped retaining the one-hertz minimap widget and low-frequency world-time singleton across UI/world rebuilds.
- Replaced the large-map per-property shared-generation lookup with a Lua-only lifecycle-epoch guard while preserving one generation check at each queued callback boundary.
- Strengthened scheduling tests to reject reintroduced low-frequency UObject retention and high-frequency shared-generation lookups.

## 0.4.0-dev26-transition1

- Added a fail-closed LoadMap pre/post lifecycle that purges every retained UE4SS UObject reference before world destruction and resumes only after two stable fresh-root samples from the new world.
- Added per-world epochs to reject stale queued control, radar-motion, world-map-motion, recovery, provider, bridge, and diagnostic work after transitions.
- Made F7 defer safely during loading/incomplete world roots and made F8 cancel deferred activation while invalidating all queued work immediately.
- Added concise transition enter, purge, post-load, and resume diagnostics plus executable/static lifecycle regression contracts.
