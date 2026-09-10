# Architecture

Current source is **3.0.0 / SG-12**. All eleven languages use pinned regular-font
pixels for settings, help, confirmations and numeric values. Reset to Defaults,
Vote for This Mod and Feedback share the same footer font size. The 760 x 852
reference layout retains its four settings cards; short viewports scroll the
body while keeping the header and footer accessible. Vote opens the mod's Nexus
page for its monthly voting action; it does not submit a vote.
Current build and delivery evidence is tracked in [Release Status](RELEASE_STATUS.md).

The historical **3.0.0 / SG-10** checkpoint was installed locally and promoted
to `dist/final-3.0.0`. Core 7/7, clean native compilation (446 targets), all four
source/release gates, Setup 20/20 and Manual 2/2 pass. The native receipt is
dated `2026-09-09T16:43:53.6047126Z`; deployment is independently verified at
`2026-09-09T16:51:20.6801835Z`, and final-package promotion at
`2026-09-09T16:55:11.2153219Z`. SG-09 candidate packages were never promoted;
SG-03 was backed up before the SG-10 replacement.
See `ACCEPTANCE_CHECKLIST.md`, `RELEASE_STATUS.md` and
`SCENE_GUIDANCE_ATTEMPT_LEDGER.md` for separate source, build, installation,
package and owner gameplay records. Historical checkpoints retain their own
dates and scope. No Nexus upload or game/FPS acceptance is implied.

## SG-09 / 3.0.0 scene display

Scene visibility occupies a third independent byte in the settings mask, with
Treasure, Area Quest and Mini-game bits enabled by the schema. All three now
default on for clean or missing settings; valid explicit false choices remain
preserved when reading an existing configuration. A 250 ms numeric
pass filters the existing treasure save snapshot, Area Quest display policy and
83-entry Fly/Mole/Wave catalog eligibility. It retains up to the configured
count and range: 24 points / 600 m by default, capped at 50 / 1000 m. Zero count
or range disables Scene work. Each EngineTickPost frame receives only that
fixed selection, updates at most 50 scalar distances with a linear pass, and
uses the current controller's engine-provided, DPI-adjusted world-to-widget
projection. It no longer sorts or rebuilds catalog selection every frame;
the catalog/eligibility selection stays on its 250 ms cadence. It performs no actor
discovery, save I/O, collision trace or new provider scan. Behind-camera,
off-screen and crowded projections are hidden; this is not terrain occlusion.

The independent UMG host retains 50 marker groups with one Image per group.
Six shared 128-by-144 textures contain the four Treasure colors, Area Quest and
Mini-game symbols, including each small outlined `v`. Six collapsed keeper
Images retain these textures through reflected Brush.ResourceObject references
in the same widget tree, including kinds that are not currently displayed.
All six imports and ownership checks occur during bounded attachment; the
display route neither imports textures nor repairs ownership. Marker image
brushes change only when their kind changes. Raw UObject pointers and AddToRoot
are not used for cross-frame texture ownership. These 50 glyph Images replace
the earlier 600 Border pieces; groups and lazy distance labels still exist.
Treasure and Mini-game glyphs remain approximately 18-19 reference units wide.
Since SG-06, the Area Quest diamond body is 18% larger (approximately 22 units)
with a 1.8-unit outline. Area Quests retain the gray diamond rails, three
horizontal white dots and original footprint; SG-09 adds a restrained
translucent gray-blue interior backing to separate them from rocky scenery.
Treasure retains its horizontal lid/clasp chest and category colors; Mini-games
retain purple crossed flags, distinct from green Mini-game reward chests. The
small `v` follows this UI group and is not a ground/source-coordinate marker.
Scene projection alone raises
Treasure by 160 cm (previously 100 cm), Area Quests by 180 cm and Mini-games
by 150 cm. Only the Treasure UI lift changes in SG-09. This lifted
point is the icon center and focus point. Raw source XYZ, candidate distances,
range eligibility and compact/map height calculations remain unchanged.

The displayed distance first applies one offset to the true raw-anchor distance
in meters: Treasure/Area Quest use `max(0, d - 1)`, Mole uses `max(0, d - 2)`,
and Fly/Wave use `d`. It is then rounded to an integer meter. The lift is never
included in this calculation; the label is guidance rather than proof that a
game interaction is available.

Public distance labels are Off, Aim Focus (default), Auto Focus and All. The
stored values remain `off`, `central_radius`, `nearest_center` and `all`.
Aim uses an ellipse centered on the screen: horizontal radius is 16% and
vertical radius is 34% of the shorter viewport dimension. Eligibility is
strictly inside the ellipse, and selection ranks visible icons by normalized
ellipse distance, reducing the weight of vertical displacement. The same
identity must remain selected for 120 ms before its sole label appears.
An acquired Aim target may remain inside a 1.2-times exit ellipse. A challenger
must continuously improve normalized center distance by over 20% and over 0.08
for 350 ms before replacing it. Auto acquires a visible target immediately and
uses Euclidean center distance: over 20% improvement and over 1.2% of the short
viewport side for 500 ms. Both retain the old label during that wait. A changed
challenger or lost advantage clears its timer; identity tracks namespace and ID
across projection reordering. Mode changes or backward time clear all timers.
Hidden, out-of-range, crowded or unselected targets cannot retain a label. On
loss Auto selects another visible target immediately; Aim begins its 120 ms
initial dwell again. Only already acquired Aim targets get the outer margin.
Off hides only text; All labels
all displayed icons. Stored mode tokens remain unchanged.

SG-12 compares projected group positions with the last submission and writes
every changed subpixel position through SetRenderTranslation. Marker groups
are volatile; pixel snapping is disabled when the reflected enum is available.
For eight or more selected markers, five engine calibration points and one real
marker validate a frame-local perspective projection. Both depth axes and a
radial 0.25-physical-pixel witness tolerance must pass. Unsuitable calibration,
including orthographic views, retains native projection; engine exceptions keep
the existing Scene fault isolation. No previous camera pose is interpolated.
New/retained visibility margins are 52/44 pixels at screen edges and 40/32 for
overlap separation. These stabilize thresholds without delaying coordinates.

TextBlocks are created only for slots that first need a label. Integer-meter
FText values are cached lazily in a bounded 0..1000 table;
normal movement considers text refresh at 100 ms intervals and writes only a
changed value. New/changed focus can update immediately, within a per-update
cold-creation budget of four TextBlocks and eight FText values. Deferred labels
stay pending for a later update rather than displaying a previous identity's
distance. A text fault disables labels while preserving Scene icons. Off
creates no new text metadata, TextBlock or FText. Live GameThread detach releases initialized
text values; UObject/process shutdown only abandons handles and never performs
late reflected destruction.

Scene is hit-test invisible. Opening this Mod's own F6 Settings over active
gameplay permits a live Scene preview while editing its controls. That narrow
exception does not bypass a real game menu, pause, world map, activity or
HUDHidden guard. Menu sampling preserves the renderer tree and its readiness;
a genuinely suppressed menu collapses the existing host, and returning to
gameplay reuses it. Routine menu-state sampling never detaches a valid tree.
Disable, travel, activity/world-context reset and safe shutdown retain their
explicit cleanup boundaries. Faults and three bounded
attachment attempts do not alter existing renderer readiness. The Scene-off
branch precedes initialization, selection and controller lookup. Diagnostics
attribute scene work separately through `scene_umg_us` and the `scene_umg`
profile metric; release defaults keep diagnostics disabled.

Area Quest Scene positions come from the independent 147-row companion
catalog, with 143 verified authored entity origins and four unavailable rows.
The loader validates the complete file before committing it. Missing or invalid
Scene data disables only Area Quest Scene anchors, preserving compact/map data
and other Scene categories. No raw marker fallback invents an unavailable
Scene position. The original Area Quest XYZ and height bands remain unchanged.

Compact Boss/Assault height uses a validated single-band spawn-Z profile and
the existing `player.z - 150` correction. The inclusive +/-500 band preserves
the original glyph; direction changes replace it with a category-colored
triangle. Area Quests use the same correction and margin with their existing
selected band. Boss/Assault/Area Quest reference sizes are 35/30/25; all three
share a 4-unit visible triangle stroke and normal frame thickness scaled only
by display/DPI. Encounter triangles reserve another unit of dark-green outline
per side. Cached shape state avoids rewriting unchanged marker geometry.

SG-09 gameplay, visual, resolution and performance acceptance have not been
recorded. SG-08's verified installation remains a separate historical identity.
The Treasure lift changes only its displayed point; distance corrections and
Auto behavior stay unchanged, and Aim retains the ellipse above. Every-frame
projection and reduced sorting/layout work are source changes, not measured
FPS or frame-time improvement.

SG-04 remains a historical source/build checkpoint: its 12-piece glyph groups
introduced the smaller symbols and `v`, and its Aim region was the SG-03 10%
circle. Its nine F6 images used a 680-by-896 panel, 41 slots and 170 codepoints.
SG-04 was not deployed or packaged. Its build evidence, the SG-03 package/
deployment and SG-02 build/install records identify their own exact bytes.
SG-01 is retained in the attempt ledger as the earlier two-category, fixed
24/600, single segment-distance-label implementation; it is not the current
3.0 rendering contract.

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

2.3.0 adds a bounded constructor-only read of `config/hotkeys.ini`. A complete
valid document replaces all three default virtual keys; otherwise all remain
F6/F7/F8. Existing generation-guarded UE4SS keydown callbacks use those selected
codes and publish the same atomic Settings/Enable/Disable requests. F6/F7/F8 in
the lifecycle and logs below are logical default-action names, not hardcoded
physical keys. There is no key polling, watcher, runtime rebinding, or extra
input interception. F6 writes only visibility/language preferences, never the
standalone hotkey file. See `RELEASE_PLAN_2_3_0.md` for the grammar and tests.

- Construction loads and validates immutable catalogs into bounded native
  containers.
- `on_unreal_init` resolves reflected metadata, initializes compact and
  expanded-map renderers (Scene remains lazy until enabled),
  registers engine/actor/travel callbacks, registers the configured keys (default
  F6/F7/F8), installs interaction
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
- F8 then detaches compact and Scene rendering, collapses the expanded host
  when valid, and clears activation-local state.
- Travel begins fail closed. Before task-class mapping and atomic publication
  state are cleared, `InitGameState` pre-transition drains already published
  exact-completion bits into activation-local numeric completion latches and
  completed quest IDs. It then detaches compact, expanded-map and Scene hosts, clears weak candidates
  and staged work, increments the epoch, and only resumes after a valid new
  context. The pre-transition drain dereferences no task actor or other UObject.
- The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is a
  save-owner hard boundary, not an ordinary activity-suppression edge. Existing
  transition-end and world-identity signals disable the radar, detach compact,
  expanded-map and Scene hosts, clear weak candidates, pending reconciliation/confirmation state,
  runtime-opened treasures, effective encounter cooldowns, area-task state,
  clock state, and the previous save's world/context baselines. Immutable
  catalogs, user visibility masks, configured treasure exclusions, and the
  event-only UObject creation listener remain process-owned. The boundary
  latches activation off; only an explicit F7 after a fully loaded open-world
  identity clears the latch and starts a fresh activation. F7 in `TitleMap` or
  during an incomplete/non-open-world load is rejected. This uses existing
  lifecycle services and adds no new poll, enumeration, or SQL path.
- Shutdown first atomically closes every ingress. Only a known live GameThread,
  before UObject/process teardown, may unregister hooks, detach UMG, stop/join
  the save worker, and perform UE cleanup. True process teardown returns without
  I/O, logging, joining, closing, or UE access. UObject-array shutdown removes
  and drains the creation listener but performs no late UFunction/UMG work. One
  finalizer owns the final logger flush.

Only safe live-GameThread process-lifetime cleanup and the TitleMap owner
boundary may hard-clear the whole fixed
encounter-death mask. Ordinary service and authoritative boundaries clear each
bit only after its numeric application succeeds; an exception retains that bit.
UObject-array shutdown does not clear it.

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
The live-GameThread finalizer may drain, join, flush, and close the writer once.
True process teardown performs none of those operations. Logs are local and not packaged, but may contain coordinates,
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

Controller-safe menu suppression does not inspect controller bindings or input
mapping. Exact `DLayerMap.SetWorldMapImage` delivery immediately latches compact
world-map suppression even when no mouse cursor appears. The existing shared
250 ms control edge samples validated visibility from the exact current map
layer and an optional ABI-validated `GameplayStatics.IsGamePaused` provider;
unknown pause samples preserve the previous known state and diagnostics report
only edges. No new timer or polling schedule is introduced. The 16 ms position
path reads only the resulting Boolean suppression state.

CM-04 also reads the current weak minimap candidate's native paint ancestry on
that existing 250 ms edge, independently of debug logging. Bound owner lookup
to eight nodes and require the main panel's `DLayerMiniMap` to equal the exact
current-world/class candidate. Follow at most 24 cycle-checked paint nodes:
`Slot.Content == child` proves a Slot.Parent link; a nested UserWidget bridge
requires both `WidgetTree.RootWidget == child` and `owner.WidgetTree == tree`.
Hidden/Collapsed or finite zero opacity is sufficient to suppress the owned
Radar host. Fully known visible ancestry up to the proven panel releases it.
Broken links, unsupported fields, cycles, stale identities or exceptions yield
Unknown, which does not latch previous hiding and does not bypass existing
cursor/map/pause/activity guards. Candidate/pool reset clears this additional
state. No input-route inference, native visibility write, scan, extra timer,
world-map transform change or pool destruction occurs on this new edge.
Positive fade opacity is not arbitrarily thresholded; reaction is bounded by
the existing activity sample cadence, not claimed to be frame-exact.

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

Owner-pointer resolution and owner-member resolution are separate update
boundaries. The packaged executable-length/RVA pair and verified
game-build-1.0.11 member offset `0x128` are zero-scan fast paths, not fixed
compatibility requirements. Within the packaged owner, the worker tries its
process-cached key-field offset, legacy `0x120`, and verified `0x128` without
opening its aligned neighborhood scan. At most 24 active `.db` key validations
are shared across the packaged and structural owner routes for one explicit F7
FullActivation. Authentication must read
`sqlite_master` from the active `.db`; an old `.bak` cannot choose a stale key.
If every candidate reached through the packaged owner fails, FullActivation
clears the stale numeric owner/member cache, performs at most one scan of the
current executable for exactly one retained structural signature, and uses
only the remaining shared key-validation budget for bounded aligned discovery
against the replacement owner. Each executable section is scanned only through
`min(SizeOfRawData, VirtualSize)`. A signature counts only after its
RIP-relative target is proven inside `SizeOfImage`; invalid-target byte noise
does not create ambiguity. This also
covers a stale nonzero packaged RVA when a future executable has the same file
length. Zero or multiple signature matches, or failure to authenticate after
the retry, fail closed. Success caches only numeric offsets, while key bytes
remain local to the one worker request and are never logged or persisted. The
contract covers structurally compatible updates only, not every future game
version, and adds no game-thread, render-frame, timer, or idle polling work.

### Area quests

The catalog contains 147 dynamic quest IDs and the nine-column
`Id/X/Y/Z/Height1MinZ/Height1MaxZ/Height2MinZ/Height2MaxZ/HeightBandCount`
presentation contract. Marker `X/Y/Z` retains the previously validated
coordinate lineage, while height presentation uses an independently derived
one- or two-band profile. Exactly 144 rows have a profile, one row has two
source bands, and three rows have no source profile. A Move_Check-only band is
excluded when an alternative task-source band exists. These legacy bands are
unchanged in 3.0.0 and do not establish a physical Scene target: the separate
Scene catalog above requires verified entity-origin evidence. A single-band
profile is used directly. For the multi-band profile, authored marker Z chooses
the uniquely nearest existing source band; it is selection evidence only and
never a synthetic height. An exact-distance tie or no-source row retains its
marker with neutral height presentation. F7 copies numeric
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

### Responsive F6 settings page

F6 creates one transient native UMG page with independent compact,
expanded-map and Scene category masks. Scene admits only Treasure, Area Quests
and Mini-games; Boss, Assault, clock and bird-egg Scene cells remain unavailable.
It may open while Radar is Off, On, or Faulted;
Bug Report and Close occupy separate top-bar controls. The status presentation
is read-only text with a thin state-colored strip. Enable, Disable, or Retry is
a separate action that keeps the page open, and Bug Report opens the fixed
Nexus Posts URL. Enable remains guarded by the same playable-
world requirement as F7. Bird eggs have one independent compact checkbox;
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

A real checkbox transition publishes all three masks, both modes, five
compact-only height switches, Scene range/count/distance settings and the
language preference in one result. Only affected render selections are dirtied.
Checkbox/mode/language changes atomically replace `config/visibility.ini`;
Scene range/count/distance changes apply immediately and coalesce disk writes
behind a 300 ms quiet period, with pending values flushed on panel close.
Expanded-map changes are held as one numeric dirty bit
relative to the opening masks and modes while the Hub remains open. A top-right
Close control or second F6 closes the panel and rearms an
attached or already-open expanded-map candidate at most once through the
existing bounded gate. Returning to the opening state clears the bit and does
not rebuild. Unchanged service
samples do not publish or write.

The startup-only visibility parser accepts at most 4 KiB. The current writer
emits `[radar]`, `[map]`, `[scene]`, `[modes]`, `[height_arrows]` and `[interface]`
once each. Scene has three category Booleans plus `range_meters`, `marker_limit`
and `distance_mode`; all three categories default OFF. Treasure, Area Quest,
Mole (nearest mini-game), Boss and Assault height default ON. The reader retains
strict legacy schema 1-4 packed masks and the old three/five-section layouts.
Absent Scene extensions and new Boss/Assault height keys use their defaults;
existing explicit choices are preserved. If `[scene]` is present, its original
Treasure and Area Quest keys remain required; the four newer keys can be absent
only when reading an older live preference file. Public defaults and current
serialization include the full six-section document.
Unknown, duplicate, mixed-format, incomplete, malformed, or inconsistent-line-
ending input falls back to safe defaults. Strict legacy schema 1-4 packed-mask
files remain accepted for upgrades; the next real F6 change atomically
serializes the current readable format. There is no hot polling or closed-panel
file work.

The presentation is a responsive 760-by-852 reference panel. A title bar and
compact Language/Status utility row precede four dark-gray cards: Marker
Visibility, Scene Guidance, Height Indicators and Filter Modes. The marker
table has only Radar and Map columns; Scene owns three independent category
chips. Height choices form a three-plus-two chip grid, and the two filter groups
sit side by side. The main title uses role scale 0.80, module headings 0.62 and
ordinary category labels 0.46. The original four cards retain their coordinates.
A fifth footer card spans reference Y 794..844 and holds Reset to Defaults,
Vote for This Mod and Feedback; Close stays in the title bar. Language-popup dismissal
and modal shielding cover the footer as well. Text and hit rectangles remain
separate. The body uses a retained ScrollBox on short viewports. A bounded 250 ms
viewport/DPI probe runs only while F6 is open and updates the retained layout;
the settings tree is not rebuilt. Radar/Map checks
have a 22-by-22 visual inside a 24-by-24 hit rectangle, matching the 24-unit row
step without overlapping adjacent hits. Clock appears after Bird Eggs; category
bit values and map eligibility are unchanged.

Scene range/count use two real reflected USlider controls; distance mode has
four discrete choices. After explicit Yes confirmation, Reset to Defaults resets all persistent F6
display preferences together: supported Radar/Map categories and all five height
switches on, Scene categories on / 600 m / 24 / Aim Focus, both filters Available
and language AUTO. It preserves the module's enabled/fault state and startup
hotkeys. The owner publishes one preference update through the existing
persistence path; the explicit control edge flushes pending edits even when
controls already match the preset. No provider or save-state reset occurs.
The confirmation introduced in SG-10 remains non-blocking and in-page for Reset, Vote and
Feedback. Yes confirms the selected action; No or Esc dismisses only the
confirmation and leaves preferences and external links unchanged. Vote
opens the mod's Nexus page for the monthly mod vote; it never submits one.
The internal `Endorse` ID remains unchanged. Feedback opens the Posts page. The modal disables background
controls and guards the dismissal edge against the same click reaching them.
F6, loss of focus and Travel retain whole-page cleanup semantics.
SG-12 computes one physical layout with 16-pixel margins and divides local UMG
units by the live DPI scale. The header and footer remain fixed; a ScrollBox
contains the body on short screens. At 720p the controls retain reference size;
640x360 is the minimum supported viewport. Every 250 ms while open, a viewport
or DPI change reflows the retained tree, preserves scroll/settings and cancels
pending confirmations before geometry changes. The native text fallback uses
the reflected Font size with a fixed role multiplier for the title, RADAR/MAP
headings, row labels, unavailable marker, and close glyph. After the second
layout prepass and exact font-size readback, optional `GetDesiredSize` evidence
is accepted only when its ReturnValue uses the same known `Vector2D` struct
identity as the validated viewport-size return. It then shrinks each exact-font
native text slot to its real line height and repositions it around the authored
vertical center. An unavailable, mismatched, faulting, non-finite, or oversized
desired size leaves that text's original slot geometry unchanged and does not
reject the Hub. Text that reaches the bounded render-scale fallback is excluded
from desired-size recentering: it keeps its authored slot and uses a
justification-aware horizontal pivot with vertical pivot `0.5`. This open-time
presentation pass changes neither the
separate button hit boxes nor compact-radar or expanded-map geometry, and it
adds no closed-panel or per-frame work. Aspect ratio never distorts the panel.
In 2.3.0 (including the unpublished 2.2.2 work), the language popup contains
`AUTO (Game Language)` first, followed by 11 explicit languages. Popup labels,
click mapping and highlights use the same preference order; Korean/Traditional
Chinese raster labels are regenerated into cells 3 and 5 (zero-based). AUTO
stays persisted: each actual F6 opening or F7 activation samples
`DGameUserSettings.LanguageText`, with the bounded Kismet fallback. An invalid
sample retains the last valid detection; English is used only before the first
valid sample. Detection does not write preferences. Choosing a manual language
adds no game-language read and remains authoritative. The selector face shows
the resolved language; the popup highlights AUTO when following is selected.
On a real F6 open, missing or expired weak identities are retried
for the already-loaded game Font objects by script:
`DsCompositFont_CommonSystem` for Korean and Latin/Cyrillic
languages, `DsCompositFont_TCSystem` for both Chinese choices,
`DsCompositFont_JPSystem` for Japanese, and `DsCompositFont_THSystem` for Thai.
The external `DS_HYFont_P.pak` overrides Common/TC without complete glyph
coverage and must be disabled or replaced for localization QA. Raw Pretendard
FontFace assets are not bundled or routed as `UFont` substitutes. Missing
loaded-font evidence falls back safely without changing the selected
language, guessing an asset path, or replacing FontMaterial. If the constructed widget
reports exactly `Font.Size == 0`, it receives one bounded reference size. If
game-widget construction, target-size calculation, or font commit fails, that
same text retries once as base UMG `TextBlock` in the same F6 transaction. The real
reflected `Font.Size` participates in layout and is capped by
the safe line height of its assigned slot. After `AddToViewport` and layout
prepass, font size is reapplied and read back. A missing core Font/SetFont ABI
or failed size application/readback closes F6 fail closed; missing optional
  game-font evidence does not change the language. Font work exists only on a
  real F6 opening; no closed-panel or tick scan exists. These widgets exist only
  for the lifetime of an open page. All eleven languages use generated 2x main
  overlays from pinned regular Droid CJK, Liberation Sans or Noto Sans Thai,
  selected by script. The canonical `assets/ui/f6` payload contains 33
  `<language>-{off,on,fault}.tga` main images, `language-popup.tga`,
  `fr-language-value.tga`, `es-language-value.tga`, 11 `tooltip-<language>.tga`
  atlases, six glass/check/chip skins, and `manifest.json`. Each
  status-specific main overlay replaces all 44 fixed main-panel text slots for
  its language. Header/body/footer Images share that one texture through three
  clipped fragments. The shared popup supplies all twelve labels, including AUTO,
  with each endonym's script font. The two retained French/Spanish name-only
  images are fallback only: a successful main overlay suppresses them to avoid
  double-painted text. They are 440-by-52 pixels; main and popup images are
  1520-by-1704. Main labels use a 32-reference base times the role scale, without
  artificial bold outlines or per-language shrinking. All three footer labels
  use role .42: 13 reference pixels, rasterized at 26 pixels.

  The retained SG-03 DLL contains intact UTF-16 `Français` and `Español
  (España)` literals. The code passes wide text through FString/FText without
  an ASCII conversion, and these languages share the Common font route with
  English. This points to glyph coverage rather than lost translation text;
  current `DS_HYFont_P.pak` presence is supporting evidence, not an in-game A/B
  attribution. The pinned Droid fallback also lacks ç/ñ. Their image labels use
  unmodified Liberation Sans from `tools/f6-fonts`, with its SIL OFL notice,
  solely at build time. No new runtime font loading or scanning is introduced.

  The name-only runtime path has one Image, two weak texture slots and two
  per-open failure slots. It uses the resolved language on open/language/status
  edges, hides only the native LanguageValue after Brush.ResourceObject readback,
  and restores native fallback on import/apply failure or another language.
  Detach/travel clears these handles and failure state.
  The selected popup fill is derived from its 190-by-32 cell with a two-unit
  inset, giving 186-by-28. The verifier binds the actual Border call and checks
  all 12 choices at four scales (48 bounds), rather than checking hit boxes only.
  SG-07 retains the 44 disjoint main text slots and expands each of 11 language
  records to 74 strings: 43 main strings and 31 specific hover explanations.
  The nine fixed-label images retain 177 required codepoints, minimum fit
  `1.000` and maximum optical-center error `0.5` raster pixel. Eleven tooltip
  atlases cover 1,076 required codepoints across 341 complete, measured tiles.
  The 26 RLE TGAs and schema-3 manifest bind source text, geometry and all
  font/generator pins. Sixty-two in-memory negative cases reject stale geometry,
  accent/AUTO regressions, overlapping checks, missing tooltip clipping/reset,
  incorrect topic or tile selection, row hover blocking checkbox columns,
  lost Box-brush readback and altered raster pixels. SG-08 adds independent
  final-pixel counterexamples for opaque stacking, excess transparency,
  linear-white contrast failure, blue tint and duplicate native backdrops.
  Source-derived English,
  Simplified Chinese, popup and all-language previews are retained under root
  `out/handoff/F6_SG07_*`; all 438 reference-font runs fit. Preview font metrics, native slider drawing and example tooltip
  placement do not establish live game size, weight, alignment or input behavior.

Six shared skin textures provide program-generated rounded glass tints,
gradients and fine edges, without game/Apple assets or runtime blur/refraction.
At the SG-09 checkpoint, SG-08's charcoal material changed to controlled soft blue-gray, preserving
the 760-by-792 layout, all text, square checks,
31 tooltip topics and 26 F6 TGAs. Accepted main/popup sheets replace their
native background fills, avoiding a second opaque panel underneath.

The final main TGA is checked after its own base/card layers are combined.
Across 3,384 inset body samples, alpha is 216/255: 15.294% background transmission.
Current SG-12 gap samples transmit 21.961-23.922%; tooltip reading surfaces remain 96-99% opaque.
A separate pixel gate requires body transmission 15-18%, gaps 20-25%, restrained
blue-gray RGB and at least 4.5:1 contrast for text RGB (247,253,255) over a white
background composed in linear light. The SG-09 card-body minimum is 4.522:1. This
guards static readability against byte-space blending that appears too dark;
it does not establish the game's actual composition, tonemapping or font result.
Current SG-12 checks cover 118 text-slot states: 44 main, the independent Vote label, two slider values and
12 popup choices, each with inactive/active controls. All small text exceeds
4.5:1 (SG-09 minimum 4.522:1); the large title has a separate 3:1 requirement. Utility
buttons, the language strip and a narrow status reading plate preserve this
contrast without making card gaps opaque.
Historical source-derived EN/ZH day/dark/stress previews retain both sRGB-byte and linear
assumptions under root `out/handoff/F6_SG09_*`; current footer and confirmation
previews are under `out/handoff/F6_SG12_*`. Slider/status authored colors
are read from the final native constants and actual setter bindings; reference
fonts, slider brush shape and example tooltip placement still approximate play.
Main and popup sheets are 1520 by 1704, matching the panel at 2x. Idle/active
chip images are 444 by 60; reflected Slate Box margins `10/222,10/30` and
scaled ImageSize preserve ten-reference-unit corner caps across chip widths.
The reflected brush copy, numeric types/sizes, applied metrics, DrawAs=Box and
ResourceObject readback are verified before the Image is accepted. Square
check images are 88 by 88 at 4x. Each skin import is attempted once per opening;
native controls and decoded sRGB Border colors remain the fallback. Textures
retain their sRGB import path; native authored Border colors are decoded only
at the brush-color boundary. The native status-text fallback separately
requires at least 4.5:1 from its actual frame/panel color constants; its
SG-09 minimum is 4.613:1 after setting the panel alpha to 0.79. Twenty-one
focused pixel counterexamples (14 material, seven Scene backing) and two
native status-color regressions are rejected; these do not establish gameplay.

Every setting has native hover help. The 32 topics distinguish seven marker
categories, three Scene switches, five height switches, four filter choices
and thirteen common controls, including Vote. Both Radar/Map checkbox columns bind the actual
category topic; its concise wording explains the displayed content. Seven
independent transparent, hit-test-visible row-label targets
also expose that topic, including when a packaged main-text overlay hides the
native TextBlock. Their 420-unit width ends before the checkbox columns.
Scene/height/filter controls bind their own topics in their actual enum order;
unknown categories have no fabricated default topic.

There are 56 distinct owner records within the unchanged 64-record pool.
Each uses its own SizeBox/Canvas/Image widget while sharing one atlas for the
resolved language. The 640-by-7632 atlas holds 53 tiles: 32 glass-backed hover
tiles, seven transparent confirmation-text tiles and fourteen numeric/placeholder
tiles. A 320-by-72
reference SizeBox clips each hover tile using reflected SetClipping and a
numeric vertical offset. Both dimensions remain below the 8192 texture bound.
Hover timing and placement belong to Slate, with no hover polling,
provider scan or per-frame file work. Language/open edges import at most once
per attempt; failed language switching clears the old custom widget and uses
native text. Reflected ToolTipWidget/Brush ownership keeps content alive;
detach/travel clears weak records. Tooltip glyphs use pinned Droid CJK,
Liberation Latin/Cyrillic and Noto Sans Thai build inputs; fonts are not runtime
payloads. See `tools/f6-fonts/README.md` and `THIRD_PARTY_NOTICES.txt`.
Range and count help explain their shared scope across enabled Scene categories,
zero hiding Scene markers, and the possible performance cost of higher counts.
Aim help describes aiming at a marker and pausing briefly; Auto describes choosing
near the screen center with stable switching. The UI revision preserves the
+160/+180/+150 cm projection-only lifts inherited from SG-09 and does not
recalibrate compact Treasure height. The height label is now Mini-games in each
language; its internal `HeightMole`/`mole` identity is unchanged and still covers
all 83 Fly/Mole/Wave points.
Deployment creates the file only when it is absent, preserving user choices.

The existing localization record holds 43 main strings plus 32 help strings
per language. A separate seven-string record supplies Vote, confirmation
title, three action bodies, Yes and No: 82 strings per language, 902 across
11 languages. Main overlay slots remain 44. Required glyph coverage is 423 main,
750 tooltip and 250 confirmation codepoints. Each confirmation body is one short
question, without repeating button instructions. Five clipped consumers share
the same verified language atlas; the body uses a 320-by-72 crop at 1.6 scale,
and Vote uses a 206-by-24 crop so all 11 languages retain the full 26-pixel font
without shrinking. F6 contains 53 TGAs plus its manifest, adding 27 main images.
The 18-byte TGA header must establish 640-by-7632
RGBA before import. Brush.ResourceObject readback precedes hiding native
fallback text. Missing or stale atlases retain readable ASCII cancellation
text and disable Yes. All seven localized strings have complete measured
glyph/crop checks. Fourteen character tiles supply `0123456789 m-v` in 16-by-26
crops to fifteen bounded numeric/placeholder consumers. Unknown characters or
failed binding restore native text; numeric changes reuse the imported atlas.
Forty-three source, manifest and pixel counterexamples are rejected, including
synthetic bolding, shrinking, missing digits and duplicate language-name layers.
Evidence: root `out/handoff/F6_SG12_SOURCE_PREVIEW.json`,
`out/handoff/F6_SG12_VIEWPORT_PREVIEW.json` and
`out/handoff/F6_SG12_NEGATIVE_VERIFICATION.json`. These are static checks,
not gameplay or external-site acceptance.

The owner services check boxes at 50 ms only while the panel is open. The
closed UI path returns before current-controller lookup or any UObject access.
The separate owner may still flush one pending Scene preference write after
close; it does not poll the preference file.
F6 key repeat is coalesced by a 250 ms toggle bound. Temporary reflected
`FText` inputs are explicitly destroyed after `SetText`. Pre-transition
cleanup removes the panel and restores input before old-world widgets can
become stale. Restoration resolves the panel host's owning Controller first, so
a silently replaced current Controller cannot inherit stale UI input state;
only weak widget identities cross frames while the panel is open.

With no confirmation active, Escape closes the entire F6 page even when its language popup is open or
a slider/text control has focus. The owner samples all controls before closing,
publishes their latest values and flushes the last slider change. Opening first
validates the foreground `UnrealWindow` belongs to this process and installs a
thread-local `WH_GETMESSAGE` consumer plus a `WH_CALLWNDPROC` focus observer.
Failure rejects the page with hub failure 40
and the Win32 error; it does not expose a panel with unguarded Escape input.
The hook consumes only Escape for that window and its children by replacing
the message with `WM_NULL` before `TranslateMessage`/`DispatchMessage`. Its
callback owns only locked numeric state and makes no Unreal call. After the
page disappears, the same press's repeats and release remain consumed; a later
independent press returns to normal game input. The observation-only hook catches
window deactivation, focus loss and destruction between owner ticks; it never
consumes input. Loss of focus requests page close and clears gesture ownership,
while retaining page-level consumption until that close is serviced. This
covers switching away and back between ticks. Travel/context cleanup removes
both hooks; process
shutdown only disables atomic ingress and leaves OS teardown to remove it.
Fifty-eight SG-10 offline routing checks verify a hidden owned window, and
55 pure confirmation-model checks cover action/gesture state. They do not
establish the game's actual menu behavior; that acceptance remains in the checklist.

Opening establishes Game-and-UI mode before writing the cursor visible bit. The
open-only service reads that bit and repeats the input/cursor transaction only
after observing that gameplay hid it again; the ordinary sample is read-only.
A prior runtime-only Hub fault may be cleared only by a later explicit F6 open
after a fault-free guarded detach and valid ABI. A fault raised during that same
open attempt remains terminal.

### Compact map

One preallocated UMG tree contains treasure, Boss, Assault, mini-game,
area-task, bird-egg, fixed Treasure/shared-mini-game height groups, and clock
pieces. The nearest Treasure and nearest visible Fly/Mole/Wave marker retain
independent Z targets, while every visible Area Quest, Boss and Assault
evaluates height in the existing bounded 80-slot pass. Boss/Assault use a finite
single-point band at authored spawn Z. They retain their original glyph when
aligned or when height is unavailable; above/below states replace it with the
category-colored triangle. Unknown height does not claim alignment.
Treasure uses its unchanged selected treasure-category fill and six-piece full
shafted pointer to the left of the selected chest.
For a multi-band Area Quest, authored marker Z first selects the uniquely
nearest existing source band. Within the selected band expanded by the inclusive
+/-500 vertical-unit margin, it keeps its normal black frame and displays three
white dots. If the player is below that band, the same pieces form an upward
black triangle; if the player is above it, they form a downward black triangle.
An exact-distance tie or missing source profile keeps the frame with neither
dots nor direction. The task marker never adopts Treasure's horizontal
clearance. No separate task arrow exists. Geometry changes only on a discrete
state edge. Fly, Mole, and Wave share one nearest-mini-game channel. Its
shaftless triangle is centered directly below the selected icon and takes the
actual selected marker's kind palette. A near-black contrast outline is added
without changing the triangle's size, position, or projection. A target more than 500 vertical units
above the comparable player Z shows an up triangle; a target more than 500
below shows a down triangle; the inclusive +/-500 band hides it. All five
height categories use comparable `playerZ - 150`; Treasure
retains its existing dead-zone behavior. The channel
uses one of 83 trusted map-100 `NPC_Start` heights (33 Fly, 40 Mole, 10 Wave).
If that height is unavailable, only the mini-game triangle is hidden. The persisted key
remains `mole`. Treasure/shared-mini-game targets and per-marker Area
Quest/Boss/Assault profiles retain independent scalar state. The three latter
glyphs use reference sizes 35/30/25 for Boss/Assault/Area Quest and share a
4-unit visible stroke/frame thickness; Encounter triangles add one unit of
dark-green outline per side. Display/DPI scaling applies to both core and
outline. No monster-origin correction is added to the existing player offset.
No child widgets are allocated or laid out on
the 16 ms motion path. Invalid current position, menu/cursor, and confirmed
non-open-world activity suppression stop compact updates; open-world interiors
remain supported.
The existing one-hertz minimap-scale read also samples live game viewport size
and DPI, never desktop monitor geometry.
If either changes, the retained host receives one top-left render-scale update,
one viewport-position update, and one layout prepass; an unchanged sample is
read-only. The fixed clock and moving marker root therefore remain aligned after
a fullscreen/windowed or DPI transition without adding a per-frame path.
Cursor, world-map, and paused-game suppression keep the current host
`Collapsed`. A confirmed
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
white dots. Shape and style changes remain inside the fixed four-piece slot.

The clock is numeric world time plus a presentation glyph. Its configured bands
begin at 06:00, 12:00, 18:00, and 21:00. The sunrise, full-sun, sunset, and
crescent-star names and boundaries are presentation policy because no
authoritative reflected game phase enum or schedule has been proved. Weather is
unavailable and is not queried.

SG-09 places the clock in the measured vertical gap between the minimap's
RetainerBox and DLayerQuest on the existing 1 Hz layout service. The glyph's
visual center is 15 reference units from the group top; the 42-unit container's
21-unit center must not substitute for it. Valid placement requires a gap of
at least 30 reference units, finite completed geometry and screen/host fit.
The exact main-panel owner is reached within eight ancestors and must point
back through DLayerMiniMap; painted ancestry is bounded to 24 nodes. No global
widget scan is added. Missing/invalid geometry uses the retained 178-reference-
unit top offset from the minimap center. A resize defers that sample, and
motion accumulates from the last applied position until it exceeds 0.25
reference units. This changes presentation, not the world-time provider or
hourly task/encounter refresh cadence.

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

An explicit map session builds one fixed 4,096-entry numeric marker snapshot.
The accepted ceiling is 2,500 Treasure rows plus 279 fixed non-Treasure rows,
or 2,779 total, leaving 1,317 spare slots. The observed 1,632-marker snapshot,
including 1,501 Treasures, was below the old 1,785 limit, so capacity was not
the dense-map flicker root.
Two transparent 2048-by-2048 RLE-TGA atlases are attached beneath the exact
current game-native map-icon Canvas through Mod-owned hosts. Each outer
`UCanvasPanelSlot` is full stretch with zero offsets, `AutoSize=false`, alignment
`(0,0)`, and maximum Canvas Z. The cloned inner `Panel_Point` slot is separately
reasserted as full stretch with zero offsets. Only the Image Canvas slot owns
`{atlas_left,atlas_top,atlas_width,atlas_height}`, and Image render translation
is `(0,0)`. The native parent therefore sees only zero-offset full-stretch Mod
hosts; negative atlas coordinates cannot enlarge its desired extent. Stable
native-child insertion plus maximum Z establishes the ordering contract; an
unchanged same-parent pass, including the final tail pass, makes no layout or
transform write, never restacks, performs Remove/Add, or requests a RetainerBox
render.
Only a fresh attachment authors those slots. A real weak parent identity change
is reported to the bounded scheduler for a fresh rebuild; retained content is
never reparented or rebased in place. Background remains
before foreground, preserving radar-internal order without per-frame layer work:

- lower radar host: area tasks and mini-games, then treasure drawn last;
- upper radar host: Boss and Assault.

Each decoded BGRA atlas occupies 16 MiB, approximately 32 MiB for both. The
persistent envelope magic is `DSNWRA52`; its normalized fingerprint is quantized
at 1/4096 UMG logical unit. A hit requires exact dimensions, header, magic,
fingerprint, and visible count, complete RLE decoding to exactly 2048-by-2048
pixels, a matching encoded-payload checksum, and payload termination exactly at
EOF. Revision-51, corrupt, truncated, or trailing-byte files miss automatically.
Writes use a same-directory temporary file and `MoveFileExW` with replace-
existing and write-through flags; failure removes the temporary file. The
bounded file read, optional write, and texture import still occur only at an
explicit attachment edge. The cache is an optimization, not runtime or hitch
acceptance. Marker coordinates and the fixed 4,096-entry snapshot are unchanged.

The final F6/localization/compact-indicator closeout changes no expanded-map
source. The atlas geometry, 4,096-entry capacity, coordinates, projection, zoom,
parent ownership, and style-revision path remain outside that source change.

The game owns pan, zoom, clipping, map visibility, native icon layout, click
routing, and RetainerBox composition. There is no expanded-map per-marker tick.
In the 2.2.1 candidate the Mod hosts are hit-test-invisible children of the current
`DLayerMap.FogAbovePanel`, resolved directly from the current layer. The native
`ArrayIconInfo` array is not an ownership source: it is consulted only while a
missing host is created, solely to obtain one instantiable icon class. Retained-
host validation and refresh do not scan it. The hosts' outer slots are full
stretch with zero offsets, `AutoSize=false`, zero alignment, and maximum Canvas
Z. After each fresh attachment, including a scheduler-accepted rebuild, the
cloned host's inner `Panel_Point`
Canvas slot is also forced to full stretch with zero offsets, `AutoSize=false`,
and zero alignment, without a Z override. Each `Panel_Point` Image Canvas slot
alone owns `{atlas_left,atlas_top,atlas_width,atlas_height}`, while Image render
translation stays `(0,0)`. The host itself receives no render transform, and the
route uses no forced layout prepass. This lets the atlases inherit
`FogAbovePanel` pan, zoom,
clipping, visibility, and RetainerBox updates directly instead of sampling and
replaying viewport transforms. Exact-artifact live alignment, dense-Treasure,
native-icon, click-target, resolution, and performance acceptance remains
`NOT_VALIDATED`.
The player projection anchor is resolved only during attachment. Each sample
uses the live `PlayerIconWidget` Canvas-slot alignment pivot, transforms it from
the player's current cached Slate geometry through `LocalToAbsolute`, and then
through `AbsoluteToLocal` into the current `FogAbovePanel` cached geometry. The
accepted result is validated against that exact Canvas's finite positive local
extent. `WorldMapUISize` is authored metadata and never stands in for the parent
width/height. World-space X and Y deltas are scaled independently by the live
parent-local width and height, then rasterized into atlas-local coordinates.
Initial attachment retains only numeric observations and uses a separate bounded
three-attempt readiness service; no sampled UObject wrapper or `FGeometry`
crosses calls. Missing or implausible geometry fails into that bounded budget.
There is no centered fallback, desktop-resolution substitution, or new poll.

WM-07 changes only initial-attachment readiness. Each numeric sample binds the
layer, direct FogAbove parent, PlayerIcon, and owning-controller index/serial,
map ID/dimensions/authored UI size, current-tick player XY, anchor XY, and live
parent extent. It projects world `(0,0)` with the existing projection function
to obtain an origin. Only matching identities/metadata and two origins/extents
within the existing 0.5 Slate-unit tolerance may attach. Real paired player
motion cancels; UI-only drift does not. Invalid observations clear the attach
seed. Raw anchors remain validated and are used with the same attempt's player
XY for the actual atlas. The separate generic extent sampler is unchanged.
`WORLD_MAP_ATTACH_PROJECTION` reports raw-anchor, world-position, extent, and
origin deltas during bounded attempts only. Cached Slate geometry can still
lag live player data; same-tick reads do not prove same-render-frame alignment,
so flying/accelerating and opening-while-zooming require live acceptance.

Attach, same-layer `SetWorldMapImage`, F7 resume, and exact wheel zoom events arm
one finite five-deadline retained-host tail at 100, 250, 500, 1,000, and
1,250 ms. A successful attach takes a fresh clock sample only after attachment
completes before arming this tail, so attachment cost cannot consume its first
deadline. Attachment does not call `RequestRender` while both new hosts are
still collapsed; the subsequent visibility transaction owns the first valid
repaint. Each due game-thread pass takes exactly one fresh numeric observation;
if the thread is late, overdue deadlines remain due and advance only one
observation on each later pass. Every pass directly re-resolves the current
`FogAbovePanel` plus its live local extent and never trusts a cached native-
parent pointer as ownership evidence. It does not scan `ArrayIconInfo` or read
`PlayerIconWidget`; that witness is attach-only. A fully
unchanged same-parent pass, including the final tail pass, returns before Image,
widget-tree, or RetainerBox mutation: it does
not write layout/transforms, replay pan/zoom transforms, restack, Remove/Add, or
call `RequestRender`; its visibility edge is published separately by Main.
Changing sibling-icon anchors are ignored while the direct parent identity and
extent remain stable, so the attach-time Image Canvas-slot atlas positions are
immutable. A real weak `FogAbovePanel` replacement is reported as
`RebuildRequired`; only the scheduler may perform a fresh attachment that
restores the full-stretch outer host, full-stretch inner `Panel_Point`, Image
atlas rectangle, and zero Image translation. A changed live
extent is first placed in the existing geometry-stability sampler and must be
seen in two matching samples before it can return `RebuildRequired`; until then
the valid retained layer remains visible. Retained RetainerBox replacement or
owned-payload replacement/invalidity can also report `RebuildRequired`.

`RebuildRequired` is report-only at the renderer boundary: detecting it does not
collapse, hide, detach, or mark the last complete payload transform-unready.
Only the session scheduler may accept the request and begin the detach/rebuild
transaction. If the once-per-session latch is already consumed, the last valid
payload remains usable instead of being stranded collapsed. At most one
`RebuildRequired` transaction may run in one open-map session. It
preserves the existing 4,096-entry marker snapshot and receives its own hard-
capped three-attempt attach/geometry budget. The whole open session is therefore
bounded to initial attachment's three attempts plus rebuild's three attempts;
consumed initial attempts are not carried into the rebuild budget. A final-tail
`RetryLater` also closes through this same bounded rebuild transaction. Mod-
owned payload, ABI, and guarded runtime failures remain terminal and detach the
hosts.

The full-stretch zero-offset outer slots, full-stretch inner `Panel_Point`
slots, Image Canvas-slot atlas rectangles, and zero Image render translation
are the placement contract. A stable map session must show one attach, zero
detaches, no native-parent extent feedback, and no
`WORLD_MAP_LAYERING_REBUILD_REQUIRED`. The normal unchanged settle path never
rerasterizes or rebuilds marker data. The single permitted rebuild reuses the
existing marker snapshot rather than recollecting it. Transient ownership gaps follow the
bounded hidden-or-retained policy without adding steady marker collection,
cache access, texture import, or widget construction.

The temporary zoom-topology trace explains why the superseded ownership
heuristic violated this boundary. Zoom-driven native icon reconstruction changed
the first valid `ArrayIconInfo` parent between `FogAbovePanel` and
`FogUnderPanel`. The Mod followed that transient ordering and reattached both
hosts four times in one zoom sequence, crossing the fog composition boundary and
producing occlusion, hitching, and flashing. The trace is development evidence
for the diagnosis only; it does not validate corrected gameplay.

The later direct-`FogAbovePanel` full-stretch-outer/Image-translation candidate
also violated the placement boundary. Quantitative 2026-09-05 screenshots show
that zoom-in scaled the base map/Radar by about 1.214/1.218 but left relative
translation about `(+113,-190)` px (32-point mean residual 0.28 px). Reverse
zoom scaled them by about 0.760/0.758 but left about `(+67,+200)` px (35-point
mean residual 0.82 px). The near-identical scale and sign-reversing vertical
offset correspond to an estimated vertical pivot difference of roughly
823-872 px. This rejects scale-formula and cumulative-frame-drift explanations
and identifies different local origins/zoom pivots. It is runtime rejection
evidence for DLL `CCC6B117...AE00` from compiled source `B650B5FB...74EA`, not
gameplay evidence for the current candidate.

The subsequent outer-atlas-rectangle candidate is also runtime rejected. DLL
`CD41F0E1...6FBB2` from compiled source `433710E0...E62C` successfully attached
1,632 markers with zero data, texture, and ABI faults, but its negative outer
slot offset fed back into the native layout. The same parent changed from
`3000` to `3191.521`, triggered `WORLD_MAP_LAYERING_REBUILD_REQUIRED`, and
oscillated through six attaches and five detaches. Its diagnostics-enabled
deployment backup,
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`, is
preserved only as rejected-candidate evidence. This is the direct reason the
current architecture keeps both outer and inner hosts full stretch and moves
the atlas rectangle into the Mod-owned Image Canvas slot.

The previous independent viewport plus extreme-Z candidate is runtime rejected.
It made the atlas visible, but user testing found severe lag, wrong placement,
and delayed updates because event-tail viewport transforms could not inherit
continuous native parent motion. That build and its technical checks are not
acceptance evidence for this hybrid candidate.

F8 may retain a valid host as `Collapsed`; travel, confirmed non-open-world
activity, and graph mismatch detach it.
State-delta or geometry invalidation first proves both exact attachment identity
and current layer visibility. If the exact layer is visibly open, the current
session may perform the one bounded, snapshot-preserving rebuild described
above. If the renderer is attached to a retained but
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

### Historical 2.2.x expanded-map validation records

The following candidate identities and validation results are preserved history;
they do not describe the 3.0.0 / SG-05 source. Current evidence
is indexed in `SCENE_GUIDANCE_ATTEMPT_LEDGER.md`.

The available healthy runtime log is bound to the prior exact 84A360B0 DLL. It
records no renderer, ABI, F6, or UE4SS fatal error and reaches normal shutdown,
but cannot validate the later F6 presentation, localized-overlay, or compact-
outline source changes. The direct-`FogAbovePanel` full-stretch-outer/Image-
translation build, DLL `CCC6B117...AE00` from compiled source
`B650B5FB...74EA`, passed its source/static/build and rollback-backed deployment
checks but was subsequently runtime rejected by the pivot-drift evidence above.
Its backup is
`dist/work/deployment/deploy-backups/20260905-092836-495-native-only-deploy`.
Those checks validate only rejected bytes. The later `CD41F0E1...6FBB2` /
`433710E0...E62C` deployment and backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy` are
also runtime-rejected evidence only. The recorded source review, static gates, Core
`2/2`, release hygiene, and local native build pass for WM-06 DLL
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1` from
compiled source
`0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`, size
1,107,968 bytes. Rollback-backed diagnostics-enabled local developer deployment
passes for those exact bytes. Installed identity matches the source DLL, the
single controlling Mod entry is enabled (`mods=1`), and diagnostics use
`debug_logging=true`. Backup:
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
Package, installer, gameplay, visual, performance, and resolution acceptance
remain pending or `NOT_VALIDATED`.

The historical 2.2.0 clean candidate was native DLL SHA-256
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`
from compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
Core/static/clean-build gates and rollback-backed local developer deployment
with diagnostics enabled passed for that exact DLL. This is not Setup ownership;
`Build-Release.ps1` package validation also passed for the exact DLL, with Setup
`20/20`, Manual `2/2`, passing payload equivalence, manual layout, and clean-target
policy validation, and byte-identical re-extraction of all three public ZIPs. Live
runtime/gameplay, F6 visual, controller, localization, exit, and performance
acceptance remained `NOT_VALIDATED`. This historical exact-byte evidence does
not validate 2.2.1. Earlier 2.2.1 technical checks remain bound to superseded
candidate bytes, and the independent-viewport/extreme-Z, first-valid-parent,
full-stretch-outer/Image-translation `CCC6B117...AE00` / `B650B5FB...74EA`, and
outer-atlas-rectangle `CD41F0E1...6FBB2` / `433710E0...E62C` deployments are
runtime rejected. The last candidate's backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`
remains rejected evidence only. Historical WM-06 immutable-slot source
review, static gates, Core `2/2`, release hygiene, and local native build pass
for DLL
`6435E100...C723A1` from compiled source `0A1A4CE3...B5E5BC5`, size 1,107,968
bytes. Rollback-backed diagnostics-enabled local developer deployment passes for
the exact DLL, with exact installed identity, `mods=1`, `debug_logging=true`, and
backup `dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
Package and installer validation pass for the exact WM-06 bytes: Setup `20/20`,
Manual `2/2`, payload equivalence, layout, clean-target, and all three archive
re-extractions.
Exact-
artifact runtime acceptance remains
`NOT_VALIDATED`.

## Release packaging

The current SG-10 manifest is
`dist/work/candidates/radar-3.0.0-sg10-20260909/release-packages/release-manifest.json`,
UTC `2026-09-09T16:53:04.9764235Z`. Its three final ZIPs contain 4/69/73 entries,
with 65 runtime files and 61 manifest members. Setup 20/20 and Manual 2/2 pass
with zero failures/skips; Setup/Manual runtime equivalence and three fresh ZIP
re-extractions pass. `package-verification.json` independently verifies all
64 payload files in each manual archive, 60 installed static files, and
promotion at `2026-09-09T16:55:11.2153219Z`. The previous SG-03 final files are
backed up in that candidate's `previous-final-sg03` directory.

The SG-10 DLL is 1,310,208 bytes, SHA-256
`F1203366DA7488FBFC9E8BF101598A4FC4D2E840F850D66CEC487452FEC7EB65`.
Its candidate `deployment-verification.json` records installation at
`2026-09-09T16:51:20.6801835Z`, matching build receipt and 34 UI files, four
byte-preserved settings files, two unchanged AutoPickup files, and one enabled
native Radar entry. The previous SG-09 installation is backed up at
`dist/work/deployment/deploy-backups/20260909-095118-801-native-only-deploy`.
The frozen build metadata and `validation-summary.json` explicitly precede
deployment and packaging. Subsequent deployment/package receipts establish
the completed state without rewriting those snapshots. Exact archive and
Setup identities are in `assets/nexus/NEXUS_FILES.txt`. Gameplay remains
`PENDING_OWNER_TEST`; no external publication is recorded.

### Retained earlier package and deployment checkpoints

The retained, unpromoted SG-09 candidate package manifest is
`dist/work/candidates/radar-3.0.0-sg09-20260909/release-packages/release-manifest.json`,
UTC `2026-09-09T15:56:21.9516109Z`. It never identified `dist/final-3.0.0`;
that directory still held SG-03 until the later SG-10 promotion.
Setup 20/20 and Manual 2/2 pass with no failures or skips. All 65 runtime files
are byte-equivalent between Setup and manual packages; all three archives
re-extract identically. Installer/Manual-No-UE4SS/Manual-With-UE4SS contain
4/69/73 entries. The 1,299,456-byte DLL is
`EC82CA04927ABCB3148409AEC16D6574641984E4A9F94790515D2B94F800F1BF`.
Actual local deployment is independently verified at
`2026-09-09T15:52:57.0256520Z` in the same candidate's
`deployment-verification.json`: DLL, build receipt and 34 UI files match;
four settings files and the two checked AutoPickup files remain byte-identical.
Backup: `dist/work/deployment/deploy-backups/20260909-085255-045-native-only-deploy`.
Packaged metadata is the explicit pre-deployment snapshot. That snapshot and
the release builder's `NOT_PERFORMED` deployment output do not supersede the
actual installation receipt. Gameplay remains `PENDING_OWNER_TEST`; no Nexus
upload is recorded.

The previous SG-03 package set is backed up under
`dist/work/candidates/radar-3.0.0-sg10-20260909/previous-final-sg03`, with its own
`release-manifest.json` and `SHA256SUMS.txt`. Its source-bound native build,
static gates, Setup 20/20, manual installation 2/2, 39-file runtime equivalence
and all three archive re-extractions pass. Installer/Manual-No-UE4SS/
Manual-With-UE4SS contain 4/43/47 entries. The exact DLL is 1,194,496 bytes,
SHA-256 `4C0B9E788E35A6E46F45B4B5AB6EFCF925B3CDB6CF9E3F3D70E86EB13A853E32`.

Rollback-backed local deployment is independently verified at
`2026-09-09T05:08:44.4698483Z`: the game was stopped, four user-owned files were
preserved byte-for-byte, the other 35 payload files matched the package, and
exactly one native Radar entry was enabled. The candidate's
`installed-verification.json` identifies that checkpoint; its backup is
`dist/work/deployment/deploy-backups/20260908-220537-090-native-only-deploy`.
Owner gameplay approval remains pending, and no Nexus upload/publication has
been performed. The 2.3.0 / CM-04 records remain historical and do not identify
this package set. See `RELEASE_STATUS.md` for the publication boundary.

SG-04 remains an undeployed source/build checkpoint. SG-05 was later deployed:
`dist/work/candidates/radar-3.0.0-sg05-20260909/deployment-verification.json`
records `2026-09-09T12:30:52.1411039Z`, DLL `C04C6E29...CC252B97`,
17 verified UI files and four byte-preserved user files. Its backup is
`dist/work/deployment/deploy-backups/20260909-052906-388-native-only-deploy`.
SG-08 was subsequently deployed with owner authorization and verified at
`2026-09-09T14:50:01.0367076Z` by its candidate `deployment-verification.json`.
The 1,288,192-byte DLL, 34 UI asset files and build receipt match; four user
settings files and the two checked AutoPickup files remain byte-identical.
Backup: `dist/work/deployment/deploy-backups/20260909-074959-027-native-only-deploy`.
This installation record supersedes the earlier build-only deployment status;
gameplay remains `PENDING_OWNER_TEST`. SG-06/SG-07 retain separate local build
evidence. The retained SG-03 archives contain none of these later changes.
At that historical SG-08 checkpoint, its allowlist planned 65 runtime
files (64 plus manifest; 61 manifest members exclude three examples) and 69/73
entries for future manual No-UE4SS/With-UE4SS archives. SG-09 now validates those
counts in the actual release manifest above; the older SG-08 record itself
remains deployment/build evidence, not SG-09 package acceptance.

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
`main.dll`, build receipt, eleven generated data files (including the independent
Area Quest Scene companion), the exact treasure override,
SQLCipher runtime, metadata, licenses/notices, and a generated package manifest. Immutable
`visibility.example.ini`, `diagnostics.example.ini` and `hotkeys.example.ini` resources supply
clean-install defaults inside Setup but are not written into the installed
target. Developer deployment preserves four validated user-owned files
byte-for-byte: live `config/visibility.ini`, `config/diagnostics.ini`,
`config/hotkeys.ini`, and `data/defaults/treasure_overrides.txt`. Setup likewise
preserves settings unless the owner explicitly confirms a hotkey change in its
Install / Update / Repair flow. F6 never writes `hotkeys.ini`. Neither
the public archive nor installed runtime contains
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

### Historical 2.2.1 package-planning snapshot

This preserved planning/status snapshot applies only to 2.2.1. Later WM-06
package results above and the 2.3.0 package records do not identify the SG-03
3.0.0 package set described above.

The 2.2.1 release target is `dist/final-2.2.1` and contains exactly
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Installer.zip`,
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-No-UE4SS.zip`,
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`,
`release-manifest.json`, and `SHA256SUMS.txt`. The current full-stretch-host/
inner-atlas candidate has passed Core, static, and clean native build gates and
must repeat Setup, Manual, payload-equivalence, manual-layout, clean-target, and
three-archive byte-identical re-extraction gates. The
future generated manifest and `SHA256SUMS.txt` are the authority for Setup and
ZIP hashes. Rollback-backed diagnostics-enabled local developer deployment of
the current exact DLL passes with backup
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`;
this is not Setup ownership or gameplay acceptance. All rejected candidate
deployments establish no acceptance.
Controller,
height visual, responsive-layout, localization-glyph, gameplay, native-icon,
click-target, exit, and external-performance acceptance remain `NOT_VALIDATED`.
Historical 2.1.1 results remain bound to
`dist/final-2.1.1` and packaged `main.dll` SHA-256
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
The prior 2.1.0 `D4EE...` package and its Setup/manual results remain historical
evidence only. Public diagnostics default to
`debug_logging=false` in every future resealed channel.

### Current ownership and dependency boundaries

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
