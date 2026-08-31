# Architecture

## Process boundary

The shipped mod is one UE4SS native C++ module. Catalog loading, state
collection, reconciliation, selection, projection, and UMG rendering all remain
inside the game process. There is no external renderer or motion bridge.

Production `main.dll` creates no shared-memory mapping and contains no connected
PostRender/Present canary. The legacy canary sources and disabled configuration
are historical audit fixtures only: CMake does not compile them and the runtime
owner does not read or reference them. Diagnostics are written only through the
bounded native event log.

## Lifecycle

- Construction loads and validates immutable catalogs into bounded native
  containers.
- `on_unreal_init` resolves reflected metadata, initializes both renderers,
  registers engine/actor/travel callbacks, registers F6/F7/F8, installs interaction
  and area-task hooks, and registers one UObject creation listener.
- A fresh F7 activation starts a new activation/epoch, clears activation-local
  numeric state, captures task definitions, schedules a bounded task-class
  identity capture, requests one save reconciliation and one transactional
  area-task scan, and permits bounded compact/world-map layer catch-up. Encounter
  actors are supplied only by exact lifecycle callbacks and the fixed 49-slot
  weak creation cache; F7 performs no encounter-class enumeration. If F7 is pressed
  while already active after task-class mapping exhausted its attempts, the
  mapping action rearms only that capture transaction and preserves
  exact-completion bits, revisions, and repeatable-reactivation latches.
- Before F7 reset, F8 disable, travel reset, or activity-suppression cleanup,
  the boundary may consume accepted encounter-death bits into numeric cooldown
  and eligibility state without touching either renderer. This authoritative
  drain does not require ordinary Pawn/context validity and does not recheck an
  accepted event's time window.
- F8 then detaches compact rendering, collapses the expanded host when valid,
  and clears activation-local state.
- Travel begins fail closed. Before task-class mapping and atomic publication
  state are cleared, `InitGameState` pre-transition drains already published
  exact-completion bits into activation-local numeric completion latches and
  completed quest IDs. It then detaches both renderers, clears weak candidates
  and staged work, increments the epoch, and only resumes after a valid new
  context. The pre-transition drain dereferences no task actor or other UObject.
- The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is a
  save-owner hard boundary, not an ordinary activity-suppression edge. Existing
  transition-end and world-identity signals disable the radar, detach both
  renderers, clear weak candidates, pending reconciliation/confirmation state,
  runtime-opened treasures, effective encounter cooldowns, area-task state,
  clock state, and the previous save's world/context baselines. Immutable
  catalogs, user visibility masks, configured treasure exclusions, and the
  event-only UObject creation listener remain process-owned. The boundary
  latches activation off; only an explicit F7 after a fully loaded open-world
  identity clears the latch and starts a fresh activation. F7 in `TitleMap` or
  during an incomplete/non-open-world load is rejected. This uses existing
  lifecycle services and adds no new poll, enumeration, or SQL path.
- The UE4SS module-unload callback unregisters hooks/listeners, stops the save
  worker, detaches UMG, and leaves the deliberately pinned module resident until
  process exit. This callback is runtime cleanup, not a user-facing uninstall
  tool.
  If UObject-array shutdown has already begun, the create listener is removed
  first and all pinned callbacks become inert through the atomic shutdown gate;
  late UFunction/global-hook unregistration is deliberately avoided.

Only process shutdown and UObject-array shutdown may hard-clear the whole fixed
encounter-death mask. Ordinary service and authoritative boundaries clear each
bit only after its numeric application succeeds; an exception retains that bit.

No Pawn, Controller, Canvas, Actor, task actor, data-table UObject, reflected
array element, or raw widget pointer is retained across frames/worlds. Retained
runtime identities use `FWeakObjectPtr`; renderer state stores validated weak
identity graphs and scalar geometry only.

### Fault recovery

The engine-tick SEH boundary records the exception code, disables feature work,
and publishes a pending scalar fault. The handler performs no UObject cleanup.
The next registered engine tick services that pending state before ordinary
work: it spends the single process-lifetime automatic recovery budget, runs
guarded disable/detach cleanup, and may call activation only when the radar had
been active. A second fault, an inactive fault, or an unsuccessful recovery logs
`ENGINE_TICK_RECOVERY_STOPPED` and leaves the radar disabled. There is no
watchdog, delay timer, retry queue, or replenishing budget.

Renderer-local recovery is explicit. A later F7 activation may recover a prior
runtime-only world-map `Faulted` state after guarded detach cleared weak handles,
provided that call raised no new fault and the ABI failure mask is zero. F7 also
treats a faulted area-quest scanner as bounded activation work.
The visibility Hub applies the equivalent runtime-only recovery only on a later
explicit F6 open. Same-call faults and ABI failures remain terminal. These paths
do not run in clean steady state.

### Event diagnostics

Diagnostics are controlled by one bounded startup-only
`config/diagnostics.ini` document. The current commented INI uses a single
`[diagnostics]` section with exactly one `debug_logging=true|false` key, accepts
at most 2,048 bytes, and defaults to `debug_logging=false`. The exact legacy
one-line `event_log_enabled=true|false` form remains accepted for preserved
older installations. A missing, oversized, duplicate, unknown, or malformed
value fails closed to disabled. When disabled, each diagnostic
site performs one atomic enabled check and returns before evaluating its detail
expression. It therefore performs no formatting, logger mutex acquisition,
directory creation, rollover, or file I/O. A developer or test deployment may
explicitly enable the switch before process startup; it is not hot reloaded.

When enabled, the native logger captures each event into a fixed 256-record
queue. The producer uses `try_lock` and never waits: contention or saturation
drops the record and increments a health counter. A dedicated below-normal-
priority writer owns the persistent process-session stream at
`runtime/logs/DragonSwordNativeWorldRadarPostRender.Native.log`, drains at most
16 records per batch, and flushes an idle partial batch after 250 ms.
Compact/world-map state changes use that bounded path; critical lifecycle and
fault-class events request an immediate flush after the worker dequeues them.
At 1 MiB the current file rotates to one `previous` file; no longer history is
retained. One `SESSION_BEGIN` line identifies bounded log schema 2. Every record
receives capture-time `seq`, `utc_ms`, and `elapsed_ms` fields. Numeric
`ENGINE_TICK_SLOW` and `ENGINE_TICK_PROFILE` payloads are copied without string
formatting on the game thread; the writer expands them and reports
`logger_dropped` and `logger_truncated`. These
fields correlate completion witnesses, save confirmations, and world-map
geometry transactions without adding another event, timer, or poll. The 16 ms
compact motion path does not log. Failed open, write, or flush state disables
only the logger, so a bad stream cannot create a repeating failed-write path.
Shutdown disables producers, drains the bounded queue, joins the writer, and
then closes the stream. Logs are local and not packaged, but may contain coordinates,
world/class names, catalog/task IDs, and lifecycle timings; redact them before
public sharing. Disabled-default behavior and static verification do not replace
fresh diagnostics-enabled gameplay acceptance.

## State collection

### Position and compact projection

The current Controller/Pawn is reacquired from the current engine on each
16 ms position sample. Cursor state is copied from that Controller before Pawn
resolution. A failed current-Pawn sample marks position invalid and immediately
collapses the independent compact host; it does not wait for a later travel or
world-identity callback. The compact renderer owns one player-controller widget
host and one moving Canvas root. Up to 80 preallocated marker slots are rebound
only on a real catalog/state/radius change, 1,000 Unreal units of movement, or a
five-second bound. Each valid motion sample changes at most the root translation
and the nearest-height group rotation/translation.

### Treasure and encounter state

Treasure interaction is authoritative. The native
`DsAnimationProp.NetMultiExecuteInteractProp` pre-hook treats its context as the
exact treasure Actor and accepts the event only when the reflected Actor
parameter equals the freshly resolved local Pawn. Mounted underwater
interaction instead supplies that Pawn's character as its `Rider`; 2.1.0
accepts it only when the callback Actor is the exact current `Pawn.Rider`
pointer, Rider and Pawn share a non-null World, the receiver is a mount-only
treasure class, and the exact receiver is within eight metres. All references
are callback-local. The receiver's reported `ObjectID` is validated against
exact class and 3D position first. If it is unavailable, an activation/epoch-
matched weak observation may supply the ID; the final bounded fallback requires
a unique exact-class 3D catalog match. `DsAnimationProp.SetDeathProcess` uses
the same exact receiver as a second completion route. When any other rejected
non-Pawn event resolves an exact treasure receiver within eight metres, one
immutable save ID enters the existing delayed fallback and a fixed catalog bit
sleeps until a 15-second due edge.
The existing below-normal save worker then selects only that ID's category. A
negative or failed result receives one final attempt 285 seconds later. Only a
positive exact save bit hides the marker. The request is capped at 64 IDs,
skips encounter and dynamic-quest SQL, holds no UObject, survives travel as
numeric evidence, and adds no poll or recurring full snapshot. BeginPlay/
EndPlay observation and a 10-second disappearance grace
provide a bounded fallback for positively seen nearby actors. Dev53 and later
do not enumerate treasure classes. All treasure correlation is event-only; it
adds no timer, poll, queue, SQL, or frame work. Runtime
eligibility excludes only save ID `11230106` at `(182813, 162051, 3150)`, the
verified sole difference between the 1,693-entry render catalog and 1,692-entry
actor catalog. The exact startup override is data, not a recurring lookup; any
catalog drift fails static verification. Runtime application changes compact
dirty state only when the matching treasure or
linked mini-game eligibility actually transitions from `1` to `0`. A map-100
transition rebuilds once when the exact attachment is visibly open; a hidden
retained layer is detached and deferred until its next exact
`SetWorldMapImage` edge. A
duplicate event that finds every matching entry already hidden performs no
additional dirtying, atlas invalidation, or rebuild.

Boss and Assault state uses an optional
`/Script/DS.DsFieldCharacter:NetMulticastNotifyDeath` pre-hook as its primary
completion boundary and the schema-validated
`/Script/DS.DsFieldCharacter:NetMulticastSetDeathProcess` receiver as a second
boundary. The second route accepts only `DENM_ProcessState::End`. Both callbacks
accept only their exact already-observed weak
receiver, an exact immutable catalog-class match, `DsMonsterCharacter`
inheritance, current activation and epoch, current numeric availability,
`visible_seen`, and a current player-to-Actor distance no greater than
100 metres. It parses no event parameter and retains no Actor. The callback
only set the corresponding bit in one of two fixed 49-bit atomic masks. Ordinary
consumption in the existing 250 ms control service requires an enabled,
non-transitioning, non-suppressed context with valid player position. It does
not recheck the time window accepted by the event. Each successfully applied or
already-deduplicated bit is cleared independently after numeric cooldown/state
application; an apply exception retains that bit without erasing unrelated or
newly published bits. Ordinary application may invalidate render state, while
an authoritative F7/disable/travel/suppression drain applies only numeric state
and defers renderer work. Missing, non-native, or failed hook registration is optional and
cannot disable required radar initialization. The R3 moribund experiment is not
part of this route: the target `MonsterCharacterData` rows use
`UseMoribund=0`, and its diagnostic sessions recorded no moribund hit.

BeginPlay/EndPlay plus a fixed 49-slot weak creation cache supplies observation
and the conservative fallback. The cache is keyed directly by immutable catalog
index. The UObject creation callback accepts only exact encounter classes and
replaces at most one weak identity in the matching slot. At the existing 250 ms
discovery edge, the fixed slots are visited round-robin. A weak Actor is
resolved, its current position is read, and only then is the 100-metre
player-to-actor bound applied. No more than eight actor-position queries may run
in one control tick. A far live position may determine that the Actor is now
outside the observation radius, but it cannot overwrite the last trusted
in-range numeric position; this prevents pooled Actor relocation from poisoning
the fallback. There is no `FindAllOf`, dynamic queue, static catalog-coordinate
prefilter, activation class scan, death-event SQL work, or continuous death
polling. Travel clears the cache; F8/F7 preserves valid same-world candidates.
Strict exact-class `Destroyed` EndPlay recovery remains a secondary event route
when the actor still passes the unique-class, current-availability, lifecycle,
and 100-metre checks. `RemovedFromWorld` is never completion evidence by itself.
For an already observed encounter, the EndPlay boundary clears the weak UObject
immediately and retains only numeric position/visibility history for the
ordinary ten-second nearby disappearance gate. If a visible short-lived
Assault reaches that boundary before the ordinary one-second presence history
can arm, the exact observed EndPlay edge starts the same missing window. It
does not complete it, and a returned Actor cancels the sequence. An ordinary
cursor-owning map pauses no numeric work and cannot erase that ended evidence;
the same cursor still invalidates ordinary live weak-pointer disappearance.
Travel and activity suppression remain fatal lifecycle boundaries.

The ordinary disappearance fallback is encounter-specific and deliberately
slower than the treasure gate. While the weak actor remains live and in range,
the 250 ms service refreshes its last trusted in-range actor position. The same observation must be
visibly present for at least one second and four discovery samples before it is
armed. It must then remain continuously absent for at least ten seconds and
forty discovery samples while the player stays within 100 metres of that last
trusted in-range position. Reappearance clears the missing interval. Leaving the radius,
travel, teleport/context replacement, activity suppression, invalid player
state, or a visible mouse cursor resets the confirmation gate and cannot write a
cooldown. Eviction also clears the corresponding fixed-slot processed identity,
so a still-live actor can be observed again after the player returns. Candidate
resolution checks current numeric availability before reflected actor work, so
a cooling-down placeholder cannot create repeated observations or rejection
logs. This conservative disappearance/`Destroyed` fallback independently
rechecks the current time condition and cooldown; a rejected race clears the
processed identity so the same live actor can be rebound without F8/F7. That
second time check never applies to an exact death event already accepted into
the R7 mask.

The confirmed activity-suppression entry edge clears every armed encounter
observation and all fixed processed identities immediately. This cleanup does
not depend on a current Pawn position or a later 250 ms discovery pass. The
fixed weak candidate slots remain available for bounded same-world rebinding
after the open-world return edge; ordinary travel still clears all slots.

Only accepted exact observed `NetMulticastNotifyDeath`, exact observed
`NetMulticastSetDeathProcess(End)`, strict `Destroyed`
evidence, or the fully satisfied conservative fallback applies a two-hour
process-local cooldown and invalidates an attached expanded atlas without
rebuilding during combat. The bounded 49-entry cooldown map survives F8/F7. A
one-shot save timestamp is merged by maximum, so an older snapshot cannot revive
a newer native defeat. Static assertions, unit tests, compilation, and package
identity do not prove the current artifact. R4 supplies historical gameplay
proof for the original route; the changed R7 handoff remains pending fresh
exact-artifact gameplay acceptance.

An exact area-quest witness that remains inconclusive after ten seconds is
not converted directly into completion. Its immutable catalog index is merged
into a fixed 147-bit pending set. Once the full F7 reconciliation is complete,
the single below-normal worker may consume all currently due bits in one
completion-confirmation request. The first attempt is due immediately; an
unresolved exact ID may be rescheduled twice at 15-second intervals, for three
attempts total. F7 supplies the required exact per-ID `COMPLETE_CNT` baseline.
A missing row means zero only when the query proved one valid save owner;
otherwise that ID's baseline is unknown and the witness cannot queue SQL. Every
request skips `tb_treasure_box`, reads only encounter respawn and dynamic-quest
completion state, and applies only an unambiguous exact-ID count strictly
greater than its known baseline. Equal, lower, ambiguous, mismatched,
unavailable, and failed results change no state. After three non-confirming
attempts the same task generation is locked: repeated exact events cannot
replenish it. A new F7 clears that lock, while ordinary runtime can clear it
only after current `NONE`/`END` and then later `ACCEPTABLE`/`PROGRESS` prove a
new repeatable generation. At most one worker
request can be pending, running, or awaiting collection. Deadlines and attempt
counters live in fixed 147-entry storage; no periodic SQL or idle poll exists.

The existing 250 ms control service contains a rate-limited 1 Hz scalar check
for the earliest future cooldown edge. Normal checks do not traverse the
catalog. Only that edge or a displayed world-hour change recomputes one fixed
49-bit visibility mask and the next scalar deadline. If direct encounter or
linked area-task visibility changes, compact selection is dirtied and an
exact visibly open attachment may rebuild once, while a hidden retained layer
is retired until its next exact `SetWorldMapImage` edge. No 49-entry work runs
on the 16 ms position path.

### Save reconciliation

After the first valid F7 player sample, one below-normal worker reads the
current SQLCipher save snapshot. It publishes only scalar/fixed results for
treasure, encounter, mini-game, and completed dynamic-task state. One positive
`USER_DBID` is required; mixed identity fails closed. Runtime native deltas win
over the older snapshot. Dynamic-task publication merges positive completed IDs
into the activation-local runtime set and never clears newer exact or generic
completion evidence that arrived while the worker was running. Encounter
timestamps merge by maximum into the preserved process-local cooldown map.
If the result arrives after an expanded atlas was attached, the same bounded
runtime-delta path rebuilds once when the exact attachment is visibly open, or
detaches a hidden retained layer until its next exact `SetWorldMapImage` edge.
Only activation lifecycle reset owns
the full dynamic-task clear. No periodic SQL worker exists.
Each SQLite C callback catches all C++ exceptions before returning across the C
ABI and enforces a fixed row cap: 4,096 treasure rows and 65,536 encounter or
dynamic-completion rows. Invalid fields, overflow, excessive rows, and allocation
failure mark that one result invalid and fail the reconciliation attempt closed.

### Area quests

The catalog contains 147 dynamic quest IDs and positions. F7 copies numeric
Main/Group definitions from the already loaded game table and discards every
UObject. Supported `NONE`, ordinary `QUEST_CLEAR`, saved
`DYNAMIC_QUEST_COMPLETE`, and uniquely linked `MONSTER_ALIVE` conditions can
prove eligibility. Unsupported conditions and weighted selection ambiguity
remain hidden.

Each refresh reads one dynamic task per game frame, then any deduplicated
ordinary prerequisites, into private fixed staging arrays. Publication occurs
only after the entire transaction succeeds. Triggers are F7, completed travel,
generic quest blueprint teardown, exact task completion, and a displayed
game-hour edge; there is no independent idle polling timer.

After F7 or completed travel, one guarded reflection reaches the current game
instance's `TaskActorClassContainer.DynamicQuestTaskList`. Rows whose numeric
`TaskUseType` identifies dynamic quests contribute a `CreateTaskClass` full
name and the unique nonzero ID from `UseQuestList`. Those IDs are joined to the
validated area-quest catalog. The map becomes ready only when all 147 catalog
IDs are covered exactly once and no class maps to multiple IDs. A task class
used only by IDs outside the 147-entry catalog is ignored; a class shared by a
catalog ID and any other ID is ambiguous because its callback cannot identify
which quest completed. Invalid array shapes, missing fields/classes, ambiguous
class-to-ID rows, duplicate catalog coverage, and incomplete coverage fail
closed. Only copied class-name strings and catalog indices survive; no
container, row, class, game instance, or task actor is retained. Capture tries
once after the existing activation/travel stability
gate and, only if that attempt fails, once more after a fixed bounded delay.
Success and the second failure both stop further work; completed travel can
start a fresh bounded capture, while an already-active explicit F7 can rearm
only the failed capture without resetting task-completion state. Confirmed
non-open-world activity suppression keeps a pending capture dormant until the
open-world context returns.

`ADETTaskBaseActor.OnRecvCompleteQuest` supplies the current task actor as the
post-hook context. The callback reads its exact class full name, looks up the
prevalidated catalog index, and sets one bit in a fixed three-word atomic mask.
The next game tick consumes that bit, immediately records the exact catalog ID
as completed, hides it from the compact numeric snapshot, invalidates any
retained expanded atlas under the same visible-rebuild/hidden-defer policy, and adds it to
activation-local dynamic-prerequisite evidence. An unmapped class logs a
bounded diagnostic and cannot hide anything.

If `InitGameState` pre-transition arrives before that normal engine-tick
consumer, the lifecycle path drains the same fixed bit mask first and applies
the same numeric catalog result before clearing the map and mask. No callback
context, task actor, class, or other UObject is needed or retained. F8 remains a
fresh activation reset; the travel drain does not weaken explicit F8/F7
resynchronization.

Each transactional scan snapshots a fixed 147-entry exact-completion revision
array. Applying an exact completion increments that task's revision before
publishing the completed state. A scan whose snapshot is now stale preserves
the newer completion instead of clearing it with an older
`ACCEPTABLE`/`PROGRESS` sample. A later scan at the current revision also keeps
the completion latched if it still sees an active state. Reactivation is armed
only when a current scan first observes the task inactive as `NONE` or `END`;
only a subsequent current-revision scan that sees `ACCEPTABLE` or `PROGRESS`
may clear the latch for a genuinely repeatable task. `FAIL` does not arm
reactivation because unrelated blueprint teardown can expose it transiently.

If an expanded atlas is retained when task eligibility changes, it is
invalidated immediately but is not rebuilt on the next 16 ms gameplay sample.
This applies equally to exact completion, settled quest callbacks, and direct or
linked eligibility changes at a displayed world-hour/cooldown edge. The next
real expanded-map session builds the fresh atlas, matching the existing
encounter-delta policy and avoiding a synchronous gameplay hitch.

Every real completion also requests the existing generic transactional refresh.
Callback bursts use a one-second quiet period capped at two seconds; the scan
settles current task state and can supply the required `NONE`/`END` inactive
boundary. A still later current scan may make a repeatable task visible again
when the game reports `ACCEPTABLE` or `PROGRESS`. Generic blueprint-teardown
callbacks still accept only `PROGRESS -> END`; `FAIL` remains non-proof for
completion and reactivation.

The single native `ETSendQuestEventTrigger` hook still validates every reflected
parameter by name, type, offset, size, and total parameter bounds. A dynamic
event for one exact 147-entry catalog ID arms that task's witness without a
prior `PROGRESS` requirement. This exact event proves identity only and never
completion. Dev67 additionally uses `OnQuestBlueprintEndPlay` and
`OnRenewQuestBlueprintEndPlay`: the exact task Actor class must map through the
complete 147-entry task-class table before it may arm the same witness. Generic
interaction remains treasure-only.

The witness is a fixed per-task Boolean, original ten-second deadline, next
750 ms probe time, and probe counter plus one deduplicated index in a fixed
147-entry queue. Repeated exact events do not extend the deadline or append a
second index. Each due probe queries only that exact catalog task ID, and all
witnesses share a hard budget of one reflected query per engine tick. Final
`END` is authoritative for the exact witnessed task even if the preceding
published sample was `NONE`, `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` itself
is not completion evidence and unwitnessed terminal states remain fail closed.
At expiry, one due final exact-ID query is preserved for a later tick when the
global budget was already consumed. Success or final expiry removes the fixed
queue entry. After completion, `NONE` or `END` has its separate existing role
as the inactive boundary for repeatable reactivation; `FAIL` does not arm that
boundary. That completed two-scan reactivation also starts a new task generation
and releases any exhausted save-confirmation lock. A new F7 resets the baseline
and lock directly. No witness retains a task Actor, world context, or other
UObject.

Current `MONSTER_ALIVE` definitions carry `value1=0`. Definition capture joins
such a row only when the task coordinate has exactly one Assault inside 150
metres. Nonzero unknown values, zero candidates, and multiple candidates stay
unlinked. The linked task reuses the existing Assault availability, cooldown,
and displayed-hour edge service; the join never runs on a render path.
The first valid displayed-hour sample after each F7 activation schedules one
coalesced transactional task refresh as well, covering activation inside an
already-open time window without adding an idle scan.

## Rendering

### Visibility Hub

F6 creates one transient native UMG panel with independent compact and
expanded-map category masks. Bird eggs have one independent compact checkbox;
their expanded-map cell is unavailable and the sanitizer never admits that bit
to the world mask. One radio-style `AVAILABLE` / `ALL` area-quest setting is
stored beside the masks. `AVAILABLE` consumes the existing prerequisite proof;
`ALL` shows any task not excluded by a saved or exact runtime completion, while
an active repeatable cycle remains visible. The mode changes presentation only
and does not mutate quest state or provider cadence.

A second radio-style `AVAILABLE` / `ALL` setting controls Assault presentation.
`AVAILABLE` preserves the normal state, time-window, and future 120-minute
cooldown gates. `ALL` returns every Assault entry from the immutable catalog
without consulting save readiness, time, defeat state, or cooldown. This is a
draw-only override: completion/cooldown authority is not changed, and switching
back to `AVAILABLE` immediately restores the live filters. Boss availability
and every area-quest rule stay on their existing strict paths. This branch is evaluated only inside the fixed
compact and expanded-map catalog selection passes; it does not change encounter
observation, death confirmation, cooldown ownership, or provider cadence.
It adds no timer, SQL request, object scan, retained UObject, or steady
allocation.

A real checkbox transition publishes both masks and both modes in one result,
dirties compact selection once, and atomically replaces
`config/visibility.ini`. Expanded-map changes are held as one numeric dirty bit
relative to the opening masks and modes while the Hub remains open. A top-right
`X` or second F6 closes the panel and rearms an
attached or already-open expanded-map candidate at most once through the
existing bounded gate. Returning to the opening state clears the bit and does
not rebuild. Unchanged service
samples do not publish or write.

The startup-only visibility parser accepts at most 4 KiB. Its current format
requires exactly one `[radar]`, `[map]`, and `[modes]` section with all named
category Boolean keys and both `available|all` mode keys present exactly once.
Unknown, duplicate, mixed-format, incomplete, malformed, or inconsistent-line-
ending input falls back to safe defaults. Strict legacy schema 1-4 packed-mask
files remain accepted for upgrades; the next real F6 change atomically
serializes the current readable format. There is no hot polling or closed-panel
file work.

The presentation is a fixed 620-by-526 reference card with a frame, header,
separate minimap/world chips, one content surface, alternating rows, and fixed
toggle geometry. Open-time layout scales by the smaller viewport ratio against
2560-by-1440, divides local UMG units by the live viewport DPI scale, and centers
the resulting physical rectangle. Each TextBlock receives the same unit scale
as a render transform with a top-left pivot, preventing the game DPI curve from
scaling text independently from its slot. A fixed role multiplier then sizes
the title, RADAR/MAP headings, row labels, unavailable marker, and close glyph
without changing their layout slots. Aspect ratio never distorts the panel.
These widgets exist only for the lifetime of an open Hub.
Deployment creates the file only when it is absent, preserving user choices.

The owner services check boxes at 50 ms only while the panel is open. The
closed path returns before current-controller lookup or any UObject access.
F6 key repeat is coalesced by a 250 ms toggle bound. Temporary reflected
`FText` inputs are explicitly destroyed after `SetText`. Pre-transition
cleanup removes the panel and restores input before old-world widgets can
become stale. Restoration resolves the panel host's owning Controller first, so
a silently replaced current Controller cannot inherit stale UI input state;
only weak widget identities cross frames while the panel is open.
Opening establishes Game-and-UI mode before writing the cursor visible bit. The
open-only service reads that bit and repeats the input/cursor transaction only
after observing that gameplay hid it again; the ordinary sample is read-only.
A prior runtime-only Hub fault may be cleared only by a later explicit F6 open
after a fault-free guarded detach and valid ABI. A fault raised during that same
open attempt remains terminal.

### Compact map

One preallocated UMG tree contains treasure, Boss, Assault, mini-game,
area-task, bird-egg, two independent six-piece height groups, and clock pieces.
The nearest treasure and nearest visible area quest may therefore retain
simultaneous Z pointers. Treasure uses the selected treasure-category fill;
area quest uses the official cyan accent; both use the same dark outline. The
two channels retain independent numeric target Z and transform state, so neither
suppresses or reuses the other. No child widgets are allocated or laid out on
the 16 ms motion path. Invalid current position, menu/cursor, and confirmed
non-open-world activity suppression stop compact updates; open-world interiors
remain supported.
The existing one-hertz minimap-scale read also samples live game viewport size
and DPI, never desktop monitor geometry.
If either changes, the retained host receives one top-left render-scale update,
one viewport-position update, and one layout prepass; an unchanged sample is
read-only. The fixed clock and moving marker root therefore remain aligned after
a fullscreen/windowed or DPI transition without adding a per-frame path.
Menu/cursor suppression keeps the current host `Collapsed`. A confirmed
non-open-world activity edge is a world-lifecycle boundary instead: it detaches
the compact host and clears every weak runtime handle. The open-world return
edge begins one fresh activation. The UObject creation listener publishes an
exact new `DLayerMiniMap` through a single weak mailbox; when it is consumed in
a valid enabled, non-transition, non-suppressed context, it can rearm the one
bounded attachment budget and is passed directly to the renderer. A distinct
replacement identity rearms once even when the old renderer still reports
attached or menu-suppressed; duplicate identities and invalid weak handles do
not replenish work. During travel, the active layer is cleared while the
single weak mailbox is deferred until transition end compares its layer world
with the exact new GameMode world. A mismatch is discarded. This prevents an
old dungeon-boundary widget tree from being uncollapsed and avoids depending
on a short timing window before the new layer exists. There is no steady retry
or recurring layer lookup.
The area-task glyph reuses one ABI-gated RoundedBox Brush and three existing dot
pieces: a 55-percent translucent charcoal fill, thick dark frame, and solid
white dots. This is a color-only change inside the fixed four-piece slot.

The clock is numeric world time plus a presentation glyph. Its configured bands
begin at 06:00, 12:00, 18:00, and 21:00. The sunrise, full-sun, sunset, and
crescent-star names and boundaries are presentation policy because no
authoritative reflected game phase enum or schedule has been proved. Weather is
unavailable and is not queried.

### Runtime-only bird eggs

Bird eggs have no immutable catalog or save-state identity. The UObject
creation listener accepts only exact `Bird_Egg01_C` and `Bird_Egg02_C` class
name keys and publishes `FWeakObjectPtr` identities into one fixed 512-slot
pool. Duplicate weak identities are coalesced; a full pool drops the new event
instead of allocating or growing.

The existing 250 ms control service synchronizes that pool and examines at most
eight previously unresolved candidates per tick. It resolves the Actor-owned
`InteractComponent`, verifies that the component is owned by the exact Actor,
and accepts availability only when the component's integer
`InteractableValue=2` and `InteractTypeValue=2` conditions both hold. The two
fields are scoped enums; schema resolution requires exact `FEnumProperty`
metadata, an integer underlying property, and one-byte width before reading.
Actor-level hidden state is intentionally not an availability signal. Missing
schema, unavailable component ownership, or an unreadable value remains unknown
and leaves the candidate pending for a later pass of the same bounded service.
Once available, the Actor position is copied into numeric state.

A fixed nearest-16 set inside the live compact radius receives bounded samples
of the same exact interaction state on the existing 250 ms discovery edge while
the compact bird-egg category is enabled. Known unavailable state follows the existing bounded disappearance
gate; exact Bird Egg EndPlay immediately retires its matching weak identity.
Invalid weak identity and world mismatch retire the candidate; travel prunes
the pool to the exact new world. F8 and ordinary activity suppression reset
active visibility without creating new discovery work, while the `TitleMap`
save-owner boundary clears the entire pool. The renderer receives only numeric
positions and the fixed egg marker kind.

This feature is compact-only. It never enters either expanded-map atlas and
performs no global UObject enumeration, SQL or save query, dynamic queue,
filesystem access, retained raw Actor, or 16 ms position read. The corrected
availability source and unknown-state retry reuse the existing 250 ms service
and introduce no additional polling schedule.

The four preallocated marker pieces share one native rounded-box brush. Narrow
outer and shell geometry produces a vertical egg oval, while the highlight and
nest remain inside the same slot. This is presentation-only and adds no widget,
layer, allocation, query, or update schedule.

### Expanded map

An explicit map session builds one fixed 1,785-entry numeric marker snapshot.
Two transparent 2048-by-2048 RLE-TGA atlases are attached to the exact current
game-native map-icon Canvas. Native icons and radar hosts can all occupy the
maximum Canvas Z, so creation-time Z is not an ordering proof. After each real
same-layer `SetWorldMapImage` post event, and on F7 resume, both retained hosts
are removed and reinserted after the native children at maximum Z. Background
is inserted before foreground, preserving radar-internal order without any
per-frame layer work:

- lower radar host: area tasks and mini-games, then treasure drawn last;
- upper radar host: Boss and Assault.

The game owns pan, zoom, clipping, and map visibility. There is no expanded-map
per-marker tick. Attachment performs guarded layer/parent/owner validation,
atlas build or process-local fingerprint reuse, texture import, and UMG attach.
The player projection anchor is resolved only during attachment. Each sample
uses the live `PlayerIconWidget` Canvas-slot alignment pivot, transforms it from
the player's current cached Slate geometry through `LocalToAbsolute`, and then
through `AbsoluteToLocal` into the selected native icon Canvas's current cached
  geometry. The accepted result is validated against that exact Canvas's finite
  positive local extent. `WorldMapUISize` is authored metadata and never stands
  in for the parent width/height. World-space X and Y deltas are scaled
  independently by the live parent-local width and height. Initial attachment
  retains only numeric observations and uses a separate bounded three-attempt
  readiness service; no sampled UObject wrapper or `FGeometry` crosses calls.
  Missing, implausible, or unstable geometry fails into that bounded budget.
  There is no centered fallback, desktop-resolution substitution, or new poll.

Attach, same-layer `SetWorldMapImage`, F7 resume, and exact wheel zoom events arm
one finite five-deadline retained-host tail at 100, 250, 500, 1,000, and
1,250 ms. Each due game-thread pass takes exactly one fresh numeric observation;
if the thread is late, overdue deadlines remain due and advance only one
observation on each later pass. Every pass re-resolves the retained/witnessed native
icon Canvas plus its live player anchor and local extent. The first four passes
are observation-only. Only the final pass may mutate the widget tree, and only
after two consecutive samples bound to the exact parent identity remain within
0.5 logical units.

When the retained and current parent extents match, stable geometry translates
the atlas bounds by the old-to-new anchor delta and reparents the same two hosts.
When stable width or height changes, translation alone is rejected because it
would scale marker glyphs with position; the final pass may instead consume one
candidate-bound full attach. Wheel events cannot replenish that token. The
equal-extent retained-host path performs no marker collection, atlas
rasterization, file access, texture import, or widget construction. A partial
reparent fails closed and detaches through the existing renderer fault path.

F8 may retain a valid host as `Collapsed`; travel, confirmed non-open-world
activity, and graph mismatch detach it.
State-delta invalidation first proves both exact attachment identity and current
layer visibility. If the exact layer is visibly open, the current session may
perform one bounded rebuild. If the renderer is attached to a retained but
hidden layer, the stale atlas is detached and all rebuild work is deferred until
the next real `SetWorldMapImage` event. That event may rearm the bounded
three-attempt readiness budget once only when its candidate serial matches the
serial already serviced. This includes an exhausted three-probe `map_id`
transaction that ended before the renderer could publish a retryable failure.
Repeated same-layer create or image events cannot replenish a completed or
exhausted budget after that one rearm. This prevents a first-open blank without
moving 40-100 ms atlas work into closed-map gameplay.
A prior runtime-only world-map fault may be cleared only by a later explicit F7
activation after fault-free guarded detach and valid ABI. It is not an automatic
map-open retry.

## Release packaging

`Build-Release.ps1` is the only supported release builder. It verifies the
source-bound native receipt, static gates, Setup resources, isolated 20-case
installer matrix, two-case manual matrix, and freshly re-extracted installer,
Manual-No-UE4SS, and Manual-With-UE4SS archives. Legacy `Stage-Release.ps1`
and `Install.cmd` paths plus
`installer/Install-DragonSwordNativeWorldRadar.ps1` are retired and must not
produce a distributable artifact.

Setup's `Uninstall` path is a separate confirmed, token-bound transaction. It
repeats strict same-product ownership and layout validation immediately before
mutation, removes only the exact product tree and at most one valid load-control
entry, and verifies that the UE4SS loader state is unchanged. The transaction
preserves unrelated Mods, saves, encoding, and line endings and rolls back every
recorded mutation on failure.

The installer archive contains exactly the unsigned Setup executable, its
SHA-256 sidecar, `INSTALL.md`, and `THIRD_PARTY_NOTICES.txt`. Complete
ExperimentalNested runtime resources are embedded inside Setup: the native
`main.dll`, build receipt, ten generated catalogs, the exact treasure override,
SQLCipher runtime, metadata, licenses/notices, and a generated package manifest. Immutable
`visibility.example.ini` and `diagnostics.example.ini` resources supply
clean-install defaults inside Setup but are not written into the installed
target; only live `visibility.ini` and `diagnostics.ini` are installed or
preserved. Neither the public archive nor installed runtime contains
`enabled.txt`, source, external renderer, Lua loop,
runtime data generator, retired canary, or PostRender/Present configuration.
The two manual channels remain ExperimentalNested only. Manual-No-UE4SS carries
the exact Mod payload plus its checksum/readme/load-control material for an
existing compatible nested runtime. Manual-With-UE4SS adds the pinned verified
ExperimentalNested runtime. Both are script-free archives with one
root that maps directly to `DS/Binaries/Win64`. No-UE4SS carries a clean
single-product `ue4ss/Mods/mods.txt`, but its instructions copy only the Mod
folder when an active load-control file already exists and require a manual
single-line merge. With-UE4SS carries one clean `mods.txt` and is valid only for
a target with no existing UE4SS. Neither archive contains or supports
StableRoot.
The embedded Setup
converts a structurally recognized alternate UE4SS layout transactionally,
retains a verified conversion backup, and normalizes the one authoritative
`mods.txt` without requiring a second installation contract.

The authoritative `dist/final-2.1.0` release contains exactly
`DragonSwordNativeWorldRadarPostRender-v2.1.0-Installer.zip`,
`DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-No-UE4SS.zip`,
`DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`,
`release-manifest.json`, and `SHA256SUMS.txt`. It passed Setup `20/20`,
manual-copy `2/2`, payload equivalence, clean-target layout, and three-archive
re-extraction. Existing `4AFE...` and `BDE21...` directories are historical and
non-authoritative for the current source. The packaged `/WX` build is
`D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`
and has not been deployed. Public diagnostics default to
`debug_logging=false` in every newly resealed channel.

Every embedded payload file is compared with its declared source hash, and
Setup requires strict top-level existing-target identity consistent with the
package manifest; nested product-name text cannot establish ownership. Existing
targets require schema-5 release and schema-1 package metadata, 2-256 unique
manifest entries, exact size/hash identity including `metadata/release.json`
and a version/label-bearing `dlls/main.dll`, and an exact recursive tree limited
to manifest paths plus the bounded live-state allowlist. Ownership JSON is at
most 4 MiB with depth 16; each and total owned payload size are at most 256 MiB;
legacy marker/example files are at most 64 KiB, each log 2 MiB, each atlas cache
16 MiB, and an install record 1-64 KiB with matching version/label. Unknown
paths or bound failures reject before backup or mutation.

The external `DragonSwordWorldRadar` is never installer-owned. An active
`DragonSwordWorldRadar : 1`, malformed same-name entry, or exact
`DragonSwordWorldRadar/enabled.txt` path in any approved Mods root causes a
zero-mutation reject. An exact disabled `DragonSwordWorldRadar : 0` entry is
preserved byte-for-byte. Setup never deletes or disables this external renderer;
only the distinct `DragonSwordWorldRadarObjectState` predecessor participates in
owned migration.

Native
release builds validate the exact commit, canonical origin, parent gitlink, and
complete worktree state of every supplied or reused dependency, accept only the
deterministic pinned UE4SS `fmt` patch, reject partial or unexpected
FetchContent source sets, override the upstream floating IconFont branch with
its pinned checkout, clean graph-owned output, and bind the DLL to a build
receipt. The payload carries the applicable `{fmt}`, UE4SS interface,
SQLitePCLRaw, SQLCipher, SQLite 3.39.2, and LibTomCrypt 1.18.2 notices and
licenses. UE4SS remains separately installed. See `DEPENDENCY_SOURCES.md` for
the unresolved public-release clearance boundary; package verification is not
legal clearance.

## Failure policy

Catalog shape, reflected ABI, save identity, task semantics, weak identity,
world/layer ownership, and capacity checks fail closed. Refresh faults preserve
the last complete area-task snapshot when one exists. A renderer attachment has
a bounded attempt count and no automatic unbounded retry. The sole automatic
engine-tick recovery budget is process-lifetime and fault-only; all renderer and
Hub recovery described above is explicit and bounded. Build, static, package,
and deployment success do not prove gameplay behavior.
