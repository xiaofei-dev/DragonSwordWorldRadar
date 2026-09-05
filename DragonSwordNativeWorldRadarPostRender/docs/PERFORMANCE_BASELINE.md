# Performance Baseline

## Evidence boundary

Dev60 remains the historical measurement baseline for the compact dungeon-return
failure, the old encounter enumeration, and other bounded transactions. A later
dev64 diagnostic session confirmed that the two-sample encounter fallback could
misclassify streaming replacement, expanded-map/menu activity, or departure as
a defeat. Those logs motivated the dev65 conservative encounter gate; they do
not accept it. The same dev64 session also proves that clustered treasure opens
were lost only at the interaction boundary and that successful opens refreshed
immediately. The latest dev66 log then exposed exact area-task completion,
treasure receiver, expanded-map first-open, and moving-encounter candidate
defects. Dev67 repairs those mechanisms structurally. The later R3 encounter
log still contains no positive Boss/Assault completion evidence: observed
targets end as departure eviction, while F8/F7 works through one-shot save
reconciliation. R4 addressed that separate defect with an exact optional death
notification route and supplied historical Boss/Assault gameplay proof. That
historical pass does not accept the final 2.1.0 handoff bytes; a fresh
diagnostics-enabled 2.1.0 gameplay and frame-time session remains required. The
user-visible result remains the final acceptance gate.

R7 preserves every R6 renderer schedule and adds two bounded runtime changes:
one startup hash-set membership exclusion for save ID `11230106`, and a repair
to the fixed 49-bit encounter-death handoff. The latter changes only how the
existing 250 ms control service preserves and applies event-published bits; it
adds no tick, poll, scan, query, SQL work, renderer schedule, or recurring work.
Release and installer hardening runs out of process. R8 adds one independent
schema-validated death-process receiver. A schema-valid dynamic event for an
exact catalog task arms the existing ten-second exact-ID witness without a prior
`PROGRESS` requirement; the event is not completion. If exact `END` is not
observed, one immediate positive-only exact-ID save confirmation and at most two
15-second retries may use the existing single below-normal worker only when F7
established a known per-ID `COMPLETE_CNT` baseline. Missing means zero only for
a valid single-owner query; otherwise the baseline stays unknown and no SQL is
queued. Only strict count growth is accepted. Three non-confirming attempts lock
the same task generation until a new F7 or settled repeatable reactivation.
Every attempt skips treasure SQL, and the fixed schedule cannot run periodically.
Exact-artifact 2.1.0 gameplay and frame-time acceptance remain `NOT_VALIDATED`.

The final lifecycle correction changes no cadence. Bird-egg availability now
uses two integer fields on the exact Actor-owned interaction component instead
of Actor hidden state; unknown reads remain in the fixed pool for the next
already-scheduled bounded service pass, and EndPlay retires one exact weak
identity immediately. The exact `TitleMap` owner boundary performs one
edge-triggered disable and state clear through existing transition/world
identity signals. Neither change adds a timer, polling schedule, enumeration,
SQL request, worker, or render-path operation.

The vertical-oval Bird Egg presentation changes only the Slate brush geometry used
by the four already-preallocated pieces. It adds no marker, widget, layer,
allocation, Actor read, timer, service pass, or render update.

The diagnostics-enabled 2026-08-28 session did not implicate Bird Egg discovery
in the reported severe hitch: six candidates produced zero drops and zero
faults, with 52 microseconds cumulative discovery time and a 43-microsecond
maximum. The same session recorded 20 immediate expanded-map atlas rebuilds
while the F6 area-task mode was repeatedly changed; each attach took 81-100 ms.
Version 2.1.0 therefore persists and applies compact changes on each real edge
but holds one expanded-map dirty bit until X or F6 closes the Hub, then performs
at most one bounded rebuild; returning to the opening state performs none. This
is an evidence-backed removal of redundant work, not
a frame-time acceptance result. The two F7 activation reconciliations remained
separate below-normal worker transactions at 760-784 ms total, dominated by
719-746 ms SQL queries.

## Clustered-treasure interaction cost

Dev67 uses only event-edge work at
`DsAnimationProp.NetMultiExecuteInteractProp`: validate the reflected Actor as
the freshly resolved local Pawn, treat the context as the exact treasure Actor,
and validate its reported `ObjectID` against exact class and 3D position.
Version 2.1.0 adds one callback-local alternate identity check for underwater
mounts: exact `Pawn.Rider` pointer equality, same non-null World, a mount-only
treasure class, and the existing eight-metre receiver bound. Only when the ID
is unavailable does completion consult one activation-matched weak identity or
the bounded unique-class 3D fallback. `SetDeathProcess` is the second exact
receiver event. There is no new tick branch, timer, polling, enumeration, SQL
call, allocation queue, filesystem access, or UMG operation. Idle, motion, and
map-render schedules are unchanged. Any rejected non-Pawn event may set one
fixed catalog bit only after an exact nearby receiver resolves. While waiting,
the 16 ms path performs only an armed flag and due-time comparison. At the
15-second due edge, the existing
below-normal worker copies the active save snapshot and queries at most 64
exact treasure categories while skipping encounter and dynamic-quest SQL. A
negative or failed result can schedule one final attempt 285 seconds later;
there is no third attempt, save-file watcher, timer thread, recurring SQL, or
full treasure-table query. This is a structural bound, not measured gameplay
or frame-time acceptance.

## Steady compact path

While compact rendering is visible:

- the current Controller/Pawn is sampled into scalars every 16 ms;
- one preallocated Canvas root receives at most one translation per valid
  sample;
- each of the three independent fixed six-piece height channels may receive at
  most one translation and one rotation, with a 0.15-degree rotation epsilon;
  Treasure, Area Quest, and shared Fly/Mole/Wave guidance retain separate
  targets, all compare against the same numeric `playerZ - 150`, and none
  allocates or reads a UObject on this path;
- marker children receive no per-sample allocation, lookup, layout mutation, or
  translation;
- activity/discovery and minimap context work run on the existing 250 ms
  service, including one encounter-edge scalar comparison rate-limited to 1 Hz,
  while minimap scale is sampled once per second;
- compact catalog selection is rebuilt only on state/radius change, 1,000
  Unreal units of movement, or five seconds.

This is one host, one moving root, and one coordinate-space translation. Older
two-root/two-translation descriptions are obsolete.

## Bird-egg provider cost

Bird eggs use the existing UObject creation listener and fixed storage only.
Exact `Bird_Egg01_C` and `Bird_Egg02_C` events enter a 512-slot weak pool;
duplicates coalesce and overflow drops the new event. The existing 250 ms
control service examines at most eight unresolved candidates per tick. Each
attempt resolves the exact Actor-owned `DInteractableComponent`, reads the two
integer availability values, and copies the Actor position only after both
`InteractableValue=2` and `InteractTypeValue=2` hold. A reflection or ownership
failure remains unknown and waits for another pass of the same bounded service.
Only the nearest fixed set of 16 candidates inside the compact radius receives
exact-state samples on the same 250 ms discovery edge while the category is
enabled. EndPlay removes one matching weak identity immediately. This removes
the separate 100 ms timer and lowers the maximum active-state query rate from
160 to 64 reads per second. The 16 ms render path uses copied numeric positions
and performs no Actor read.

The category is compact-only and independently controlled by F6. It adds no
new polling schedule, global UObject enumeration, SQL or save query,
dynamically growing queue, filesystem polling, expanded-atlas build, or
per-frame UMG allocation. These are structural bounds, not measured frame-time
acceptance.

## Save-owner boundary cost

Entering the exact `TitleMap` performs one hard-stop transaction: detach both
renderers, clear fixed weak/runtime state, cancel activation-owned
reconciliation and confirmation state, and latch activation off. The check
reuses transition end and the existing world-identity control probe. While the
title screen and subsequent load remain inactive, ordinary radar/provider work
stays disabled. No automatic reactivation, retry timer, new world poll,
enumeration, or SQL request is scheduled; one explicit open-world F7 starts the
next activation.

## Area-task cost

Dev55 changes only the colors of the existing compact area-task RoundedBox and
three dot pieces. Widget count, geometry, layout, scan scheduling, and the 16 ms
translation path are unchanged.

Dev67 adds no idle task scan. A real exact quest event or exact task-actor end
event may arm one fixed numeric witness for ten seconds. Its first probe is due
after 750 ms and later probes retain that interval, but all witnessed tasks share
a hard limit of one reflected exact-ID query per engine tick. A deduplicated
fixed 147-entry index queue cannot grow beyond the catalog, and repeated events
cannot extend a live deadline or add another entry. Generic interaction does not
participate. Success or final expiry removes only that entry. The
`MONSTER_ALIVE` positional join still runs only during the existing F7/travel
definition capture. Neither path adds SQL, UObject enumeration, or 16 ms work.

The F6 visibility Hub is transient. While closed, its owner executes one
Boolean branch and returns before Controller lookup or UObject access. While
open, check-box service is bounded to 50 ms; only a real mask transition writes
the configuration and may invalidate one current atlas. The same open-only
sample reads one cursor bit and performs the input transaction only if gameplay
has hidden the cursor. A
250 ms F6 debounce prevents key-repeat allocation bursts. Hiding every compact
category collapses the existing renderer and stops its motion service.
The persisted area-quest `AVAILABLE` / `ALL` choice adds one Boolean branch to
the existing fixed 147-entry selection passes and no new pass, query, timer,
allocation, SQL operation, or UObject access. A real mode edge reuses the same
single compact dirty flag and bounded current-atlas invalidation as a category
change.

The persisted Assault `AVAILABLE` / `ALL` choice similarly adds only one enum
branch inside the existing fixed 49-entry compact and expanded-map selection
passes. `ALL` returns the immutable Assault catalog without reading state,
time, or cooldown for presentation; `AVAILABLE` retains every live gate. The strict paths
used by observation, death confirmation, cooldown application, Boss selection,
and linked area-quest eligibility are unchanged. The mode adds no selection
pass, timer, periodic SQL, provider query, object scan, UObject retention, or
steady allocation. Exact 2.1.0 gameplay and external frame-time acceptance for
this changed artifact remain `NOT_VALIDATED`.

A task refresh is bounded and amortized: one reflected task or prerequisite
query per game frame into fixed staging arrays. Dev47 measured complete
147-task scans at approximately 0.6-1.0 ms total, spread across frames. Refresh
is event-driven by F7, travel, quest callbacks, exact completion, or a displayed
game-hour edge. The hour edge is repeated bounded work, roughly once per real
minute under the 60x local game-clock model; it is not an independent wall-clock
polling timer. One identical refresh also runs on the first valid clock sample
after F7. No task refresh performs SQL or UObject enumeration.

Dev54 no longer waits for a 147-task inference scan before hiding a mapped
completion. The callback publishes one bit in a fixed three-word atomic mask;
the next game tick applies only numeric state and render invalidation for that
exact catalog index. The existing follow-up state scan still waits for one
second of quiet with a hard two-second maximum, holds one bounded request state,
and exists to settle game state. A repeatable task cannot reactivate from that
or any other still-active sample until one current scan first observes
`NONE`/`END` and a later current scan observes `ACCEPTABLE`/`PROGRESS`; `FAIL`
does not arm the transition.

An exact or settled task-state change invalidates a retained expanded atlas but
does not move atlas work onto the 16 ms gameplay sampler. If the exact
attachment is visibly open, the event service may rebuild it once in the
current session; a hidden retained layer is detached and deferred until its
next exact `SetWorldMapImage` edge. The same rule covers direct encounter and
linked-task eligibility changes published by displayed world-hour or two-hour
cooldown-expiry edges.

Treasure-open and linked mini-game reward deltas use an idempotent visibility
transition. Only a real `1 -> 0` change dirties compact selection. A changed
map-100 entry applies the same visible-rebuild/hidden-defer rule instead of
reusing stale pixels. A duplicate runtime ID returns before either generated
catalog is scanned and causes no dirty state, atlas invalidation, or rebuild.

Boss/Assault cooldowns use one process-local map bounded by the 49-entry catalog.
F8/F7 does not clear it, and one-shot save timestamps merge by maximum. The
existing 250 ms control service performs only a rate-limited 1 Hz comparison
against one earliest-expiry scalar. The 49-entry visibility mask is recomputed
only when that scalar expires or the displayed world hour changes. A real mask
or linked-task change performs one compact dirty transition and either rebuilds
one visibly open exact attachment or retires a hidden retained layer until its
next exact `SetWorldMapImage` edge. There is no 49-entry scan, log, atlas
mutation, or extra clock call on the 16 ms motion path.

R4 added no death-state poll, and the R7/R8 handoff repairs add none. The
optional `DsFieldCharacter.NetMulticastNotifyDeath` and schema-validated
`NetMulticastSetDeathProcess(End)` pre-hooks run only on game-owned death events
and perform bounded scalar, weak-identity, exact-class, lifecycle,
availability, visibility, and 100-metre checks. An accepted event sets one bit
in a fixed 49-bit atomic mask. Ordinary consumption in the existing 250 ms
control service requires a valid runtime context but does not recheck the
event's already-proved time window. Applied bits clear individually; an apply
exception retains its bit without erasing unrelated or newly published bits.
F7, disable, travel, and activity-suppression boundaries may settle pending
numeric state without renderer mutation before reset. Only safe live-
GameThread finalization and the TitleMap owner boundary may hard-clear the
mask; true process teardown closes ingress and returns, while UObject-array
shutdown does not clear it.
The callback performs no UObject enumeration, SQL query, atlas build, dynamic
allocation queue, recurring timer, or 16 ms motion work. Missing or failed
optional-hook registration adds no retry path and does not disable the radar.

Dev67 encounter discovery and the dev65 confirmation remain inside that same
250 ms service. Fixed weak slots are walked round-robin; current actor position
is read before the 100-metre player bound, with no more than eight actor-position
queries in one control tick. Each currently observed nearby encounter refreshes
its last trusted position only while the Actor is in range. A later far pooled
position may evict the observation but cannot replace that trusted numeric
position. Four present samples spanning at
least one second arm the observation; forty missing samples spanning at least
ten seconds are required before the fallback can complete. Every sample also
requires the player to remain within 100 metres of the last trusted in-range actor
position in a valid open-world, non-menu context. Reappearance resets missing
evidence, while departure, teleport/context replacement, activity suppression,
invalid player state, or a visible cursor clears the gate. An already observed
`RemovedFromWorld` clears its ended weak UObject immediately and may retain
numeric evidence only for this same ten-second gate; the event itself never
completes an encounter. The strict exact-class `Destroyed` path remains event
driven. Candidate consumption and the final state write on these conservative
fallback routes both recheck current time/cooldown availability; rejected races
clear the fixed processed identity and add no retry loop. An exact death event
already accepted into the R7 mask is not subject to that second time-window
check. This adds no timer, SQL, enumeration, dynamic
queue, catalog-coordinate prefilter, atlas build, or 16 ms work; fresh runtime measurement must still
establish its bounded 250 ms cost and gameplay correctness.

Confirmed activity entry performs one edge-only pass over the currently
observed bounded objects and clears two fixed 49-entry processed-identity
arrays. It runs even when Pawn sampling has already failed and performs no
steady polling, allocation, UObject resolution, SQL, or render-path work.

The single asynchronous F7 save result may retire an atlas that was attached
before reconciliation completed. A hidden retained layer takes that one
event-only detach and waits for its next exact `SetWorldMapImage` edge. If the
exact attachment is visibly open, the same bounded event service may instead
perform one current-session rebuild. Neither path adds steady work.

The task-class identity map is one bounded F7/travel capture. It traverses at
most 4,096 reflected `DynamicQuestTaskList` rows, accepts at most 256 quest IDs
per row, and must finish with exactly 147 unambiguous catalog bindings. Local
maps/sets and copied strings may allocate during that capture; no UObject is
retained and no class-map work occurs in the steady render path. Its
`AREA_QUEST_TASK_CLASS_MAP elapsed_us` cost has no gameplay evidence yet. The
first attempt runs once after the existing activation/travel stability gate;
only failure permits one delayed retry. Success or the second failure stops the
work, so this is not a steady-state timer or polling path. Confirmed suppressed
activities defer pending capture and perform no task-class reflection.

The exact-completion race fence is two fixed 147-entry integer arrays plus two
fixed 147-entry boolean reactivation arrays. A scan copies staging state only
when an event already requested a full transaction; exact completion increments
one integer and clears one boolean. Inactive/current and later active/current
samples update only those fixed entries. This adds no heap allocation, UObject
access, timer, or work to ordinary 16 ms position samples.

## Historical and remaining hitch sources

Dev47 logs measured exact-class `FindAllOf` catch-up at about 28.5-37.8 ms on the
game thread; dev60 still measured Boss/Assault enumeration around 22-32 ms.
Dev53/dev54 removed treasure scheduling. Dev64 removes the remaining encounter
enumeration from production and substitutes one fixed weak slot per immutable
encounter catalog entry. The existing 250 ms service resolves only nearby slots,
so there is no class scan, dynamic queue, or encounter work on the 16 ms path.
Fresh runtime evidence must confirm that no `NEARBY_CLASS_CATCHUP` or production
`FindAllOf` timing event remains. Static structure demonstrates only that the
new R4 path is bounded; it does not establish gameplay correctness or external
frametime acceptance.

The dev60 audit also recorded bounded event transactions around 24-29 ms for
the one-shot DLayerMap lookup, 27-34 ms for clock baseline capture, 22-31 ms for
area-task definition capture, and 45-104 ms for expanded-map open/attachment.
These are not continuous work, but each can still present as an isolated hitch.
Dev64 does not claim to remove those transactions; acceptance must measure their
frequency and confirm that none repeats or accumulates unexpectedly.

Expanded-map attachment is also synchronous on the game thread. Dev67 performs
same-session runtime-delta rebuild only after exact attachment and current layer
visibility are both proven. A hidden retained layer is detached and defers all
rebuild cost until the next exact `SetWorldMapImage` edge. That edge may rearm
the three-attempt readiness budget once for the matching serviced serial,
including when three early `map_id` probes exhausted before any renderer
failure code existed. Duplicate same-layer events cannot create recurring
attachment work. Prior evidence measured approximately 94-97 ms for
build/write/import/attach and 27-37 ms for a
process-local atlas-file cache hit. It occurs only during an explicit attachment
transaction and is not steady-state work, but it can cause one visible hitch.

## Expanded-map steady state

After attachment, the expanded renderer has no per-marker tick, timer, poll,
lookup, rebuild, file write, texture import, or UMG mutation. The exact native
parent supplies pan, zoom, clipping, Z composition, and visibility. F8/F7,
travel, state invalidation, and a new map session can cause bounded validation,
detach, or reattachment work.

## Save and logging

F7 queues one full below-normal SQLCipher reconciliation after the first valid
player sample. Native interaction/disappearance deltas update immediately and
no periodic save query exists. An exact area-quest end witness that remains
inconclusive for ten seconds may queue its fixed catalog bit for an immediate
coalesced exact-ID confirmation and at most two retries spaced 15 seconds apart,
but only with a known exact per-ID F7 `COMPLETE_CNT` baseline. A missing row is
zero only for a valid single-owner query; any other missing/ambiguous ownership
leaves the baseline unknown and queues no SQL. The same single worker accepts
at most one request at a time, every confirmation skips treasure SQL, and only
strict count growth is accepted. Its asynchronous
dynamic-task result only
merges positive completion IDs; it cannot erase a newer native exact/generic
completion. Negative, ambiguous, mismatched, unavailable, or failed results
change no state; after three total attempts that task generation is locked
against repeated-event replenishment. Only new F7 or current `NONE`/`END` then
later `ACCEPTABLE`/`PROGRESS` unlocks a new generation. This
ordering rule adds no recurring query, save poll, dynamic queue, or steady-state
work.
Encounter timestamps likewise merge by maximum into the bounded
process-local map, preventing an older F7 result from reversing a newer native
defeat.

Each SQLite row callback is exception-contained and row-bounded: 4,096 treasure
rows and 65,536 encounter or dynamic-completion rows. Malformed/excessive input
or allocation failure aborts only that one worker attempt. These checks are
linear guards inside an already one-shot worker and add no steady game-thread
work.

Native diagnostics are controlled by one bounded startup-only
`config/diagnostics.ini` document. The strict current form accepts at most 2,048
bytes, one `[diagnostics]` section, and exactly one
`debug_logging=true|false` key; release installs default to false. The exact
legacy one-line `event_log_enabled=true|false` form
remains accepted for preserved older installations. Missing, oversized,
duplicate, unknown, or malformed input also
disables diagnostics. Every disabled diagnostic site returns through one atomic
enabled check before detail formatting, logger locking, directory creation,
rollover, or file I/O. The configuration is not polled or hot reloaded. A test
deployment may explicitly enable it before process startup.

When enabled, diagnostics enqueue capture-time metadata into a fixed 256-record
queue. The producer uses non-blocking `try_lock`; lock contention or saturation
drops and counts the record instead of waiting. A below-normal-priority writer
owns the persistent process-session output stream, drains at most 16 records per
batch, and flushes idle partial batches after 250 ms. One schema-2 session header
is written, and each event receives `seq`, `utc_ms`, and `elapsed_ms` fields.
This changes no event cadence and introduces no game query, polling pass, or
high-resolution timer.

START, shutdown, activation/disable, activity suppression, and fault-class
events request an immediate flush only after the worker dequeues them.
Compact/world-map state changes use the ordinary bounded path. The current file
is capped at 1 MiB and rotates to exactly one previous file. Numeric
`ENGINE_TICK_SLOW` and `ENGINE_TICK_PROFILE` records perform no game-thread
string formatting; writer output adds `logger_dropped` and `logger_truncated`.
Other rare descriptive events still format their already-required scalar
details before enqueue, so enabled diagnostics are lower-interference rather
than literally free. Runtime measurement remains required. The metadata must
not claim an active high-resolution timer because this module does not call
`timeBeginPeriod`.

Visibility configuration is likewise startup-only: one bounded 4 KiB parse of
the readable `[radar]`, `[map]`, and `[modes]` document, with strict legacy
schema 1-4 compatibility. It is never hot-polled. A real F6 selection edge
performs one temporary-file replacement; unchanged open-Hub samples and all
closed-Hub samples perform no configuration file I/O.

The compact and expanded-map layer mailboxes first perform an acquire-only
pending check. Their mutexes are entered only when a create event may be
present; the locked atomic exchange remains the authoritative consume step.
Idle gameplay, including F8, therefore performs no mailbox lock/unlock pair.

Engine-tick recovery is fault-only. It adds no watchdog, timer, poll, or retry
queue. A pending SEH fault is serviced on the next registered engine tick, may
consume one automatic recovery attempt for the entire process, and then returns
without running the ordinary tick body. World-map and Hub runtime-only recovery
are explicit F7/F6 transactions. None of these paths changes clean steady-state
cadence.

## R7 acceptance measurements

- With diagnostics explicitly enabled for a test install, `START` must name
  `2.1.0`, runtime label
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0`, and
  `config_debug_logging=true log_schema=2` in the runtime START summary. The installed
  gameplay-test copy is expected to be enabled; the release package default
  remains `false`.
- Open a two- or three-chest cluster in immediate succession and one
  XY-overlapping different-height set. Require one distinct event and immediate
  state transition per chest without F8/F7, no false neighbor removal, and no
  new recurring work between interactions.
- In a separate clean release-default startup with no pre-existing runtime log,
  require `config/diagnostics.ini` to retain `debug_logging=false` and verify that no
  native log directory, rollover, formatting-only event work, or file I/O is
  created by the disabled path. Re-enable diagnostics before collecting the
  remaining acceptance evidence.
- At the active high-resolution/DPI configuration, verify that title, column
  labels, rows, unavailable marker, and X remain inside their own geometry.
  Text transforms are fixed construction-time calls and add no 50 ms service
  or closed-state work.
- Open F6 at the active resolution and verify the centered framed panel, column
  chips, row rhythm, toggle contrast, and close control. All added work is
  bounded to panel construction on F6 open; closed-state cost remains zero.
- Switch between fullscreen and windowed resolutions without F8/F7. The next
  existing one-hertz minimap-scale sample may issue two scalar viewport queries;
  only a changed size or DPI may add one host scale, position, and layout
  update. The 16 ms motion path, marker children, and clock source remain
  unchanged.
- Expanded-map attach may add a bounded set of cached-geometry reads and two
  coordinate transforms per bounded sample: the player alignment pivot through
  `LocalToAbsolute`, then into the selected native Canvas through
  `AbsoluteToLocal`. Initial readiness remains a separate maximum-three-attempt
  service. Projection scales X/Y world deltas by that witnessed parent's live
  local width/height; `WorldMapUISize` is metadata, not the parent extent. Only
  numeric values/timing cross samples--no sampled UObject wrapper or
  `FGeometry`. There is no centered fallback, desktop-resolution substitution,
  per-frame world-map projection, aspect-ratio poll, or steady timer.
- Verify all expanded-map radar categories remain above game-native icons at
  several zoom levels. A changed native Canvas or same-parent anchor/extent
  reflow must remain within the existing witnessed stable-geometry and bounded
  reconstruction strategy. The five
  deadlines are 100/250/500/1,000/1,250 ms; each due game-thread pass takes one
  observation; overdue deadlines remain due and advance only one observation
  per later pass. For later event tails the first four passes are read-only; only
  the final pass may use the existing witnessed stable-geometry and bounded
  map/zoom reconstruction strategy. The atlas-local correction adds no parent-
  size post-check, growth rejection, or new extent-change allowance. Missing,
  mismatched, or final-unstable geometry follows the established fail-closed or
  defer path. There is no steady-state work.

These statements are static bounded-work properties. Deterministic numeric
viewport tests, including 3840x1600, are unit inputs only and do not accept real
21:9, a 3840x2160 viewport with internal black bars, windowed client geometry,
runtime Slate layout, or external frame-time behavior. Static verification
cannot replace live visual acceptance.
- Measure scene-handoff hide latency. The first failed current-Pawn sample must
  collapse compact rendering through the existing 16 ms sample; no new timer,
  query, enumeration, or retained UObject is allowed.
- Complete two dungeon enter/return cycles without F7/F8. The activity edges may each
  perform one guarded detach/rearm transaction, but steady gameplay must retain
  the existing cadence. Require compact `detached`, then
  a distinct `COMPACT_LAYER_CAPTURE source=create_listener` serial, a new
  attach, fresh geometry, and no `Faulted` state or repeated retry loop after
  each return. `attach_rearmed=true` is required only when the event is consumed
  outside transition and activity suppression; transition-end consumption may
  report `false` because the return edge already began the bounded activation.
- In an ordinary multi-minute session, require no `ENGINE_TICK_FAULT`, renderer
  `Faulted` state, or visibility-Hub fault. Under a controlled engine-tick fault,
  require one `ENGINE_TICK_FAULT` and at most one `ENGINE_TICK_RECOVERED` for the
  process. A later fault must log `ENGINE_TICK_RECOVERY_STOPPED`, remain disabled,
  and produce no recurring recovery work.
- Verify a prior runtime-only world-map fault can recover only on a later
  explicit F7 after clean detach, and a prior runtime-only Hub fault only on a
  later explicit F6. A same-call detach fault or nonzero ABI failure mask must
  remain terminal.
- Open F6 directly from gameplay and require the cursor on the opening frame or
  first 50 ms repair sample. Record no repeated input repair while it stays
  visible and no Hub service after `X`, F6 close, F8, or travel.
- Enable the compact-only `BIRD EGGS` row and approach both exact egg classes.
  Require stable markers, a nearest-active count no greater than 16, no more
  than eight unresolved-candidate queries per 250 ms control tick, and no
  expanded marker. Require availability only for an Actor-owned component with
  `InteractableValue=2` and `InteractTypeValue=2`; an unknown read must remain
  retryable without appearing, and exact EndPlay must remove the matching marker
  immediately. Toggle only that row off/on and verify no other category changes.
  A multi-minute capture must contain no new polling schedule, UObject
  enumeration, or SQL attributable to bird eggs and no accumulating pool or
  queue behavior.
- Activate in save A, produce at least one runtime treasure or encounter delta,
  then return to the exact title screen. Require one `MAIN_MENU_DISABLED` edge,
  both renderers detached, the prior save's mutable runtime state cleared, and
  no automatic reactivation while loading save B. F7 before an open-world
  identity must be rejected; one explicit F7 after save B reaches open world
  must start a fresh activation and one fresh reconciliation. Require no new
  title-screen poll, repeated cleanup loop, enumeration, or SQL before that F7.
- Toggle one category once and require exactly one applied-mask log and config
  replacement. An unchanged open Hub must generate neither writes nor atlas
  rebuilds.
- Record `AREA_QUEST_TASK_CLASS_MAP`; require `ready=true`, `bindings=147`, and
  `ambiguities=0`, and retain its `elapsed_us` value.
- The delivery/cooking task should emit
  `AREA_QUEST_EVENT_TRIGGER progress_precondition=not_required`, then arm one
  ten-second exact-ID witness with a 750 ms probe interval. The event itself
  must not complete the task. Accept either
  `AREA_QUEST_COMPLETION_VERIFIED evidence=exact_task_id_end_state` or an expired
  witness followed by a baseline-relative exact-ID save confirmation. Require a
  known per-ID F7 `COMPLETE_CNT` baseline and strict growth; a missing ID is
  zero only for a valid single-owner query, and an unknown baseline queues no
  SQL. The fallback may
  run immediately and retry twice at 15-second intervals, but must produce no
  fourth request or periodic SQL. After exhaustion, same-generation trigger
  events must remain locked until new F7 or the two-scan repeatable generation
  transition. Final `END` may follow a stale prior `NONE`,
  `FAIL`, `ACCEPTABLE`, or `PROGRESS`, but `NONE` alone must never latch
  completion. Simultaneous witnesses must stay within one reflected exact-ID
  query per engine tick, with no duplicate queue entry or deadline extension.
- Complete a task while a prior scan is still active and verify that the older
  transaction cannot revive it. Verify that a later still-active current scan
  also cannot revive it before an inactive boundary.
- Record the time from completion to icon removal separately from the later
  generic refresh. For a genuinely repeatable task, require current `NONE` or
  `END`, then later current `ACCEPTABLE` or `PROGRESS`, before reappearance;
  confirm `FAIL` never arms reactivation.
- Force one failed task-class capture, press F7 while already active, and verify
  the bounded capture rearm does not clear completion revisions, bits, or
  reactivation state.
- Confirm no treasure, Boss, or Assault class appears in
  `NEARBY_CLASS_CATCHUP`, and no production `FindAllOf` timing event is emitted.
  Require current-actor-distance candidate binding and no more than eight actor
  position queries in any 250 ms control tick.
- Attach the expanded map, return to gameplay, open a map-100 treasure or finish
  a linked mini-game, and reopen the same live map layer. The changed marker must
  be absent from a newly built next-session atlas rather than returning through
  `WORLD_MAP_ATLAS_REUSED reason=same_live_layer`; repeating the same event must
  cause no additional invalidation or rebuild. A hidden retained layer must use
  `action=deferred_until_set_world_map_image`; a visibly open exact layer may use
  `action=bounded_visible_session_rebuild`. Only one serial-matched
  `WORLD_MAP_ATLAS_SET_IMAGE_REARM` may restore the readiness budget for that
  layer.
- Observe a live Boss and Assault for at least one second/four discovery samples.
  Without defeating either target, separately walk beyond 100 metres, teleport,
  open and close the expanded map/menu, and trigger a streamed replacement.
  Require no `ENCOUNTER_RUNTIME_STATE_APPLIED` or two-hour cooldown. Reappearance
  must reset missing evidence instead of completing it.
- Require `ENCOUNTER_DEATH_HOOK_READY`. In a separate capability-boundary test,
  a deliberately unavailable optional hook must report one bounded disabled
  reason without preventing normal radar readiness, but that session cannot
  accept R7 encounter completion. Without F8/F7, defeat one Boss and one Assault and require
  `ENCOUNTER_DEFEATED_NATIVE
  evidence=exact_observed_net_multicast_notify_death` plus
  `ENCOUNTER_RUNTIME_STATE_APPLIED` for each. Separately exercise strict
  exact-class `Destroyed` and the fallback, which requires a full
  forty-sample/ten-second continuous absence while the player remains within
  100 metres of the last trusted in-range actor position in normal open-world
  gameplay. Then
  perform F8/F7 before save persistence; require the cooldown to remain
  effective after reconciliation. Cross one cooldown expiry and one
  time-conditioned Assault hour edge and require at most
  one `RUNTIME_VISIBILITY_EDGE` per real visibility transition, with no 49-entry
  work on the motion path.
- Accept an exact death at the end of a time-conditioned Assault window, then
  let the window change before the next 250 ms service. The pending bit must
  still apply. Invalid ordinary context must defer rather than erase it.
- Publish multiple encounter bits and inject one apply exception. Require
  per-bit success clearing, retention of the failing bit, and no loss of an
  unrelated or concurrently published bit. Exercise F7, disable, travel, and
  activity suppression with pending bits; each may settle numeric state without
  renderer work, while only safe live-GameThread finalization or the TitleMap
  owner boundary may clear the whole mask. True process teardown and UObject-
  array shutdown must not touch it. These cases must add no new recurring metric, service, timer, SQL, scan,
  or 16 ms work.
- Require the fixed encounter slots to detect and retire both a Boss and an
  Assault, including strict `Destroyed` EndPlay recovery when the positive
  BeginPlay observation was unavailable. An observed `RemovedFromWorld` may
  complete only through the full nearby ten-second numeric gate, while
  unobserved removal, menu/cursor state, travel, teleport/context replacement,
  and departure must remain fail closed.
  Record only the existing 250 ms discovery event cost; there must be no
  encounter class-enumeration metric.
- Record expanded-map `atlas_build_us` and `attach_total_us` for cold attach,
  reopen, F8/F7, and post-travel attach.
- Verify the current native log remains at or below 1 MiB, only one previous log
  exists after rotation, ordinary diagnostics are not emitted per frame, and
  critical fault/lifecycle events survive an immediate process exit.
- Confirm no accumulating task scans, SQL work, renderer faults, or stale atlas
  behavior during a multi-minute session.

## 2.1.1 addendum: controller menus and task-height presentation

This addendum is additive. All preceding 2.1.0 and R7 measurements, receipts,
status labels, and ownership remain historical 2.1.0 evidence; none is
relabelled as 2.1.1 evidence.

### Controller-menu suppression cost

- The exact `/Script/DSClient.DLayerMap:SetWorldMapImage` post event latches
  world-map suppression immediately and collapses the existing compact host.
  The implementation does not read controller bindings or input mappings.
- `UWidget::IsVisible` is not polled on the 16 ms coordinate path. It is read
  only while the world-map latch is set or during a bounded activation/travel
  catch-up. An unknown sample preserves the previous state, while a destroyed
  weak layer clears its stale latch.
- `/Script/Engine.GameplayStatics:IsGamePaused` is an optional,
  ABI-validated provider on the existing 250 ms activity probe. It adds no new
  timer, thread, UObject scan, SQL query, filesystem read, growing container,
  or recurring allocation.
- The 16 ms render decision calls the pure boolean
  `compact_render_suppressed` predicate. It reads existing visibility,
  position, cursor, world-map, pause, and activity booleans; it performs no
  reflection, widget lookup, map query, allocation, or log write.
- Menu-state diagnostics are emitted only on known state edges when debug
  logging is enabled, not once per sample or frame.

For this addendum, source review is `PASS` and the complete static source gate
is `PASS`. Real-controller map/pause behavior is `NOT_VALIDATED`, and external
frame-time or hitch capture is `NOT_VALIDATED`. Those two runtime results must
not be inferred from the source/static results.

### Historical Area Quest height and arrow cost

- The 2.1.1 task-height implementation reused fixed marker pieces and added no
  runtime widget or pool allocation. Its single-height data contract and narrow
  alignment threshold are superseded by the 2.2.0 height-band model below and
  must not be used as current acceptance criteria.
- The historical black/white task presentation and marker-Z no-fallback rule
  remain useful architectural evidence only. They do not validate 2.2.0
  direction, placement, appearance, or performance.

## 2.2.1 addendum: world-map ownership isolation

Version 2.2.1 is a fixes-only release. The native world-map icon Canvas is now
a read-only geometry witness: Radar does not add children to it, alter its
desired size, participate in its prepass, or affect its hit-test layout. The
two Mod-owned atlas hosts are independent, hit-test-invisible viewport widgets.
The player anchor and native Canvas geometry are converted through
`LocalToAbsolute` and then into game-viewport local space with
`AbsoluteToLocal`; accepted geometry changes update only the two viewport-host
transforms.

This isolation adds no new timer, poll, per-frame projection route, atlas
rasterization, marker rebuild, or dynamic steady-state container. Same-parent
pan, zoom, DPI, window, aspect-ratio, and controller-layout changes must not
rerasterize, rebuild, reproject, re-add, or reparent an atlas. Invalid or
unstable geometry collapses only the Mod-owned viewport hosts and follows the
existing bounded settle/defer path.

Exact-artifact 2.2.1 source/static gates, Core `2/2`, native build `444/444`,
package validation, Setup `20/20`, Manual `2/2`, payload/layout/clean-target
checks, three-archive byte-identical re-extraction, and rollback-backed developer
deployment passed for DLL
`C21823088E38D2BD1635651981187AB4C01C2FFD0DCD4804CB9FFDB1899FABB9` and
compiled source
`DE0100B2D4DE894FA94C6911AD328F7699C55D50EC21193C688B44F7F2588BA2`.
Gameplay, native-icon stability, click alignment, exit behavior, and external
performance remain `NOT_VALIDATED`. The exact 2.2.0 measurements, hashes, tests,
package checks, and deployment record below are historical evidence only and
must not be relabelled as 2.2.1 evidence.

## Historical 2.2.0 addendum: mini-game height, localization, and responsive F6

This addendum is additive. Every 2.1.1 gate, hash, measurement, and deployment
statement above remains historical 2.1.1 evidence. No `B89F...` result is
relabelled as 2.2.0 evidence.

### Fixed shared-mini-game-height cost boundary

- The mini-game height input is one immutable 83-row map-100 table keyed to
  exact `MiniGame_<kind>_<id>_NPC_Start` identities: 33 Fly, 40 Mole, and 10
  Wave.
- Runtime selection consumes numeric catalog state only. It performs no XML
  parsing, PAK extraction, global object enumeration, SQL request, filesystem
  read, or runtime catalog generation.
- The selected mini-game reuses a fixed preallocated arrow group and publishes only numeric
  transform state. It adds no widget construction or allocation to the 16 ms
  compact motion path.
- The target compares with numeric `playerZ - 150`. The inclusive +/-500
  interval adds only fixed scalar comparisons and hides the triangle. A missing
  or invalid trusted height hides only the shared mini-game arrow. It cannot
  trigger a retry loop or suppress the mini-game marker.

### All-visible Area Quest height cost boundary

- The Area Quest height switch defaults ON with the other two controls. A valid
  existing visibility document remains authoritative and is not overwritten.
- The immutable 147-row catalog carries 144 height profiles, including one
  genuine two-band profile, and three no-source rows. Profile parsing adds no
  runtime Actor scan, filesystem read, or allocation.
- Every visible Area Quest is classified during the already bounded 80-slot
  marker pass. For a multi-band row, authored marker Z performs one bounded
  nearest-band selection over existing source bands; it never becomes height
  data. The normal black frame shows three white dots inside the selected
  band's inclusive +/-500 margin. The same pieces point up when the player is
  below that band and down when above it. An exact-distance tie or missing
  source profile remains neutral with no dots or direction. Comparison uses the
  same numeric `playerZ - 150` as the other height channels.
- Renderer geometry/visibility changes occur only when that marker's discrete
  state changes. No separate task-arrow widget, UObject query, allocation, or
  new schedule is added to the 16 ms motion path.

### Localization cost boundary

- F6 exposes only the 11 explicit languages. A legacy AUTO preference is read
  only for migration on the next actual F6 opening or F7 activation, resolves
  `DGameUserSettings.LanguageText`, then Kismet and English, and persists one
  explicit language. Explicit preferences add no game-language read.
- Localized strings are fixed tables. The closed F6 path performs no
  localization work, allocation, UObject access, or file I/O.
- Font setup runs only while constructing the open F6 page. It selects already-
  loaded Common/TC/JP/TH game Font objects by script; missing evidence falls
  back without an asset-path guess, FontMaterial change, recurring scan, or
  language change. If the constructed
  widget reports exactly `Font.Size == 0`, one bounded reference size is
  seeded. A game-widget construction, target-size, or font-commit failure
  retries that text once as base UMG `TextBlock` during the same open.
  The real reflected `Font.Size` participates in layout
  and is bounded by the slot-safe line height. After `AddToViewport` and prepass,
  font size is reapplied and read back; a missing core Font/SetFont ABI or failed
  size application/readback closes F6 fail closed. Missing optional font
  evidence never changes the selected language.
- Korean and Traditional Chinese fixed labels use event-only generated 2x
  overlays from pinned DroidSansFallback at base size 32, with a one-pixel
  translucent stroke and role-specific optical baselines. The six status-
  specific main assets cover all 30 fixed ko/zh-Hant text slots, the shared
  popup covers only those two language names, and the other nine languages stay
  on native game fonts. Static generation confirms no clipping. Loading those
  fixed assets adds no tick-time font work; live visual acceptance remains
  separate.

### Responsive F6 cost boundary

- F6 uses one transient responsive reference page scaled and clamped to the
  current viewport and DPI at open time.
- The existing 50 ms service is active only while the page is open. Closing F6
  returns to the same no-UObject-access path.
- An explicit F6 request may perform bounded 250 ms Controller/readiness probes
  for at most 15 seconds before the page opens or the request expires. This work
  exists only after the key press and is not a steady-state poll.
- Bug Report and Close are independent top-bar controls. Off/On/Fault status is
  read-only text with a thin state-colored strip and updates only when status
  changes. Enable, Disable, or Retry is a separate one-shot action that keeps the
  page open; the fixed Nexus Posts launch is also one-shot. These commands add no
  recurring service when F6 is closed. Translucent cards, equal-width filter
  choices, and alignment changes are presentation-only.
- The zero-size seed and optional-to-base retry are bounded to the explicit F6
  construction path. They add no engine-tick branch, steady allocation, font
  scan, language poll, or closed-panel work.
- A real setting change writes one bounded atomic configuration replacement;
  unchanged samples write nothing. Expanded-map changes remain coalesced into
  at most one rebuild when the page closes.

### Historical expanded-map atlas-layout stability boundary

- The historical 2.2.0 implementation placed two outer slots on the native
  Canvas, each occupying the atlas parent-local rectangle
  `{atlas_left,atlas_top,atlas_width,atlas_height}`. Each `Panel_Point` Image is
  local `{0,0,atlas_width,atlas_height}`. The live-rejected full-parent
  outer-host experiment was not part of that historical baseline. Version 2.2.1
  supersedes this ownership model with independent viewport-owned hosts and a
  read-only native Canvas as described above.
- This correction adds no immediate parent-size post-check, parent-growth
  rejection, extent-change token, or projection fallback. Later layout changes
  continue through the existing witnessed stable-geometry and bounded map/zoom
  reconstruction strategy.
- Live parent resolution, cached-Slate transforms, DPI, window, zoom,
  independent X/Y projection, and aspect-ratio handling remain in the existing
  finite observation chain. No `3000`/`8000` extent is accepted as a geometry
  substitute.
- Alignment remained pending live acceptance of the 2.2.0 replacement DLL.
  The observed runtime snapshot contained 1,632 total markers, including 1,501
  Treasures, below the old 1,785 limit. Capacity was therefore not the flicker
  root. The historical exact 84A360B0 log shows one attach and no repeated detach/rebuild
  sequence, but diagnostics-disabled and diagnostics-enabled runtime comparison
  is still required before any dense-map performance or stability claim.
- Fixed snapshot capacity is now 4,096. The accepted maximum is 2,500 Treasure
  rows plus 279 fixed non-Treasure rows, or 2,779 total, leaving 1,317 spare
  slots without adding a dynamically growing steady-state container.
- Style revision 50 builds two 3072-by-3072 atlases. This is 50 percent more
  linear raster density and approximately 72 MiB raw for two decoded BGRA
  atlases versus about 32 MiB at 2048. The additional roughly 40 MiB belongs to
  event-built atlas textures, not per-frame allocation. Marker coordinates,
  projection, zoom, parent ownership, and outer/inner container geometry remain
  unchanged. Runtime memory, attach time, visual quality, and clean exit still
  require exact-artifact acceptance.

For 2.2.0, refreshed Core `2/2`, all static gates, release hygiene, and the
clean native `/W4 /WX` build passed for DLL
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`,
bound to compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
`Build-Release.ps1` package validation passed for that exact DLL: Setup reports
  `20/20`, the manual-copy matrix reports `2/2`, payload equivalence, manual
  layout, and clean-target policy validation pass, and all three public ZIPs
  re-extract byte-identically. Local diagnostics-enabled deployment of
  exact DLL `6AEFDACC...` passed with matching source, build, and installed
  hashes. Its rollback backup is
  `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
  The prior backup
  `dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`.
  belongs to the superseded intermediate 59529B2A deployment and was not final
  2.2.0 candidate evidence. This is not Setup ownership; the earlier 634D283A
  deployment is historical only. None of these records validate 2.2.1.

The packaged game-1.0.11 owner RVA and `0x128` key-member offset remain fast
paths. One FullActivation shares a total budget of at most 24 active-`.db` key
validations across packaged and structural owner routes. Packaged-owner failure
permits one unique structural scan that counts only targets inside the mapped
image and scans executable sections through `min(SizeOfRawData, VirtualSize)`.
Structurally incompatible updates fail closed, and this is not an all-future-
version guarantee. F6 measured desired-size text reflow applies only to exact
font-layout TextBlocks during first open and explicit language, status, or popup
presentation changes. The return structure must match the known `Vector2D`
identity. Invalid evidence and the render-scale fallback preserve authored
geometry; buttons, maps, and the per-frame
path are untouched. Those are static scheduling properties, not measured
runtime performance or visual acceptance.
Real gameplay, simultaneous
height guidance, physical-controller behavior, all 11 language glyphs,
responsive layouts, F6 vertical alignment, exit behavior, and diagnostics-
disabled frame-time comparison remain
`NOT_VALIDATED` until captured against that exact final artifact. Static
verification cannot replace live visual acceptance.
