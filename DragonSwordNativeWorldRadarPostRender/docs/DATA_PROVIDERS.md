# Data Sources and Runtime Providers

This native project does not contain or run the legacy C# installer, Overlay,
watcher, Lua provider modules, PAK extractor, or `ooz.exe`. The committed files
under `src/data/generated` are immutable inputs copied from the previously
validated DragonSword World Radar generation pipeline. Deployment validates
their headers, counts, and cross-catalog identities before copying them.

## Save-owner lifecycle boundary

The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is a hard
boundary between save owners. Existing transition-end and world-identity
signals disable the radar, detach both renderers, clear runtime weak candidates,
one-shot and confirmation request state, runtime treasure/encounter deltas,
area-task state, clock state, and the prior world/context baselines. Immutable
catalogs, user visibility masks, configured treasure exclusions, and the
event-only UObject creation listener remain process-owned.

The boundary latches activation off. Loading a save never resumes providers
automatically; F7 is accepted only after the new save exposes a fully loaded
open-world identity. F7 in `TitleMap` or during an incomplete/non-open-world
load fails closed. Detection reuses existing lifecycle services and adds no
polling, UObject enumeration, save query, or SQL schedule.

## World-map projection provider

Projection is attach-only. Each bounded sample reads the live player-icon
alignment pivot, converts through the player widget's current cached Slate
geometry with `LocalToAbsolute`, then converts through the selected native icon
Canvas's current cached geometry with `AbsoluteToLocal`. The first valid numeric
result only seeds stability state. A sample at least 150 ms later is accepted
when stable; if it changed, one third sample may establish stability, for three
samples maximum. Only numeric coordinates and timing cross samples--no sampled
UObject wrapper or `FGeometry`. Missing, implausible, or still-unstable geometry
fails into the existing bounded attach budget. No centered fallback, new timer,
polling service, or per-frame projection path is added.

## Installed catalogs

### Treasure

- `treasures.lua`: 1,693 section-aware render records, including 1,506 map-100
  records.
- `treasure-actors.tsv`: 1,692 unique save IDs with exact generated actor class
  and XYZ for native interaction/lifecycle matching.
- `data/defaults/treasure_overrides.txt`: one startup-only exact exclusion,
  `ignore 11230106`.

Save ID `11230106`, section `2142120000100`, generated UID
`DT_Unlock_G3_11206`, at `(182813, 162051, 3150)` is the sole render-catalog ID
absent from the actor catalog and is confirmed not to exist in game. It remains
in the immutable 1,693-entry source catalog for provenance but is ineligible for
both renderers. Verification binds the override to that exact record and the
1,693/1,692 singleton set difference; a regenerated actor record, count change,
coordinate change, or additional ignore fails closed pending review.

The source lineage is the installed game's matching
`SectionTreasureBoxData.xml` and `PropTreasureBoxData.xml`. Runtime never
extracts the PAK and never polls these files. Dev53/dev54 do not schedule
treasure-class `FindAllOf`; the interaction hook plus observed BeginPlay/EndPlay
state owns live completion.

Dev67 binds interaction to the exact
`DsAnimationProp.NetMultiExecuteInteractProp` receiver Actor and requires the
reflected Actor argument to equal the freshly resolved local Pawn. Version
2.1.0 additionally accepts only the exact current `Pawn.Rider` pointer for a
mount-only treasure receiver within eight metres when Rider and Pawn share a
non-null World. Its reported `ObjectID` must pass exact class and 3D catalog
validation; activation-matched weak identity and bounded unique-class 3D
matching are the only fallbacks. `SetDeathProcess` supplies a second exact-
receiver event. None of these paths adds recurring provider work. Any other
rejected non-Pawn interaction remains a request for delayed proof, not
completion: only a uniquely resolved exact receiver within eight metres may
enter the fixed catalog-bit set. The below-normal worker queries only the
requested treasure categories at 15 seconds and, if necessary, one final time
285 seconds later. Positive save bits are authoritative; negative, unavailable,
ambiguous, or exhausted results leave the marker unchanged.

### Boss and Assault

- `bosses.lua` and `boss-actors.tsv`: exactly 9 Boss records.
- `assaults.lua` and `assault-actors.tsv`: exactly 40 map-100 Assault records.

Actor catalogs use exact SectionMonster CID-to-generated-class joins. The
single supported Assault time condition remains numeric and fail closed;
unsupported weather semantics are not inferred. Runtime uses the immutable 49
class index, save cooldown rows, and positively observed native lifecycle
events. The bounded effective cooldown map survives F8/F7, and save timestamps
merge by maximum so an older snapshot cannot reverse a newer native defeat.
Only a cooldown or displayed-hour edge recomputes the fixed 49-bit visibility
mask; the ordinary 16 ms motion path does no catalog-wide encounter work. A
single UObject creation listener stores at most one weak identity per immutable
encounter catalog index in a fixed 49-slot cache. The 250 ms service visits the
slots round-robin, resolves the current Actor position, and applies the
100-metre player-to-current-actor bound. It performs at most eight Actor
position queries per control tick. Travel clears the cache, F8/F7 preserves
valid same-world candidates, and no catalog-coordinate prefilter, encounter
class enumeration, or dynamic queue remains.

The primary exact `NetMulticastNotifyDeath` event and secondary schema-validated
`NetMulticastSetDeathProcess(End)` event publish one catalog bit only
after exact observed identity, class, activation/epoch, current availability,
visibility, and 100-metre proximity pass. Ordinary consumption on the existing
250 ms service requires a valid runtime context but does not recheck the time
window already accepted by that event. Each applied or deduplicated bit clears
independently; an apply exception retains its bit without dropping unrelated or
new bits. F7, disable, travel, and activity-suppression boundaries may settle
pending numeric cooldown/eligibility without renderer mutation before reset.
Only process shutdown and UObject-array shutdown hard-clear the fixed mask. The
ordinary disappearance and strict `Destroyed` fallbacks continue to check their
current time/cooldown gates. This handoff adds no poll, timer, scan, SQL query,
dynamic queue, or steady work.

Dev56 also supports the current reflected dynamic-quest `MONSTER_ALIVE`
condition without assigning semantics to an unknown numeric ID. Only a row
whose `value1` is zero may use the fallback. Its task coordinate must have
exactly one Assault inside a 150-metre planar neighborhood; that Assault's
validated numeric CID becomes the temporary link used by the existing cooldown
and world-time availability model. A nonzero value, no candidate, or multiple
candidates remains unknown and hidden. No actor, table row, or condition UObject
is retained after the F7 definition snapshot.

### Mini-games

`moles.lua` contains the 83 records rendered by this candidate:

- 33 Fly;
- 40 Mole/hammer;
- 10 Wave/ring.

The legacy extractor identified 34 Fly source rows, but ID 11024 is excluded
from the native render contract. Deployment explicitly validates the 33/40/10
shape. Coordinates and save mask identities are immutable installed data; no
runtime coordinate scan or Lua completion provider exists in this mod.

### Runtime-only bird eggs

Bird eggs are not a generated catalog and have no save-backed completion bit.
The UObject creation listener accepts only exact `Bird_Egg01_C` and
`Bird_Egg02_C` Actors and stores their weak identities in a fixed 512-slot
pool. The existing 250 ms control service examines at most eight unresolved
candidates per tick. It reads availability only from the Actor-owned
`DInteractableComponent` exposed through `InteractComponent`, requires exact
ownership, and accepts only `InteractableValue=2` together with
`InteractTypeValue=2`. Actor hidden state is not a provider signal. Missing
schema, invalid component ownership, or unreadable values stay unknown and are
retried by the same bounded service; once available, the Actor position is
copied into numeric state. A fixed nearest-16 set inside the current compact
radius receives bounded checks on that same 250 ms service edge.

The provider is compact-only and has one independent F6 RADAR category. Its MAP
control is unavailable, and no bird-egg entry is ever added to the expanded
atlas. Duplicate creation events coalesce. Invalid or wrong-world weak Actors
are retired, known unavailable active entries follow the bounded disappearance
gate, and exact Bird Egg EndPlay removes its matching weak identity immediately.
F8 and ordinary activity suppression reset active visibility; travel prunes to
the new world, and the `TitleMap` hard boundary clears the whole pool. The
provider retains no raw Actor and uses no new polling, UObject enumeration, SQL
or save query, dynamically growing queue, filesystem polling, or 16 ms position
work.

### Area quests

`area-quests.tsv` contains exactly 147 catalog IDs and XYZ positions. Coordinate
lineage is the previously validated MnMRadar table. Availability is not encoded
in the TSV: F7 copies current numeric Main/Group definitions from the loaded
game database, reads the one-shot save completion snapshot, and performs
bounded reflected runtime-state queries.

`/Script/DS.DETUtil:ETSendQuestEventTrigger` may arm one exact catalog task when
the event is dynamic and its numeric ID is one of the 147 catalog entries. No
prior `PROGRESS` sample is required: the exact dynamic event proves task
identity, but is never completion by itself. Dev67 also accepts the exact task
Actor supplied by `/Script/DSClient.DClientQuestSystem:OnQuestBlueprintEndPlay`
or `OnRenewQuestBlueprintEndPlay`, but only after its class maps unambiguously
through the complete 147-entry task-class table. Generic interaction remains
treasure-only.

Each exact witness owns a fixed Boolean, original ten-second deadline, 750 ms
probe schedule, and counter. A deduplicated fixed 147-entry queue contains only
numeric catalog indices. Repeated events do not extend the deadline or append a
second entry. Each due probe queries only that exact task ID, with a global hard
limit of one reflected witness query per engine tick. Final `END` proves the
exact witnessed task complete regardless of whether the previous published
state was `NONE`, `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` alone is not
completion evidence and an unwitnessed `END` remains fail closed. Success or
final expiry removes the witness queue entry; unresolved expiry queues only that
exact catalog bit for the bounded positive-only save-confirmation path described
below. The separate post-completion
repeatable-task fence still requires a current `NONE` or `END` followed by a
later current `ACCEPTABLE` or `PROGRESS`; `FAIL` never arms reactivation. That
two-scan transition begins a new task generation and releases an exhausted
save-confirmation lock; a new F7 also starts a fresh baseline/generation.
Witnesses retain no event parameter, task Actor, world context, or other
UObject.

A delayed snapshot max-merges encounter cooldowns and positive task completion
evidence. If an expanded atlas was already attached, applying the result may
rebuild once when the exact attachment is visibly open; a hidden retained layer
is retired and deferred until its next exact `SetWorldMapImage` edge.

Dev54 obtains exact completion identity from live game metadata rather than a
generated file. Each F7 activation or completed travel permits one reflection
attempt after the stability gate and only one delayed retry after failure:

`GameInstance.TaskActorClassContainer.DynamicQuestTaskList`

Only dynamic-quest rows are considered. Each `CreateTaskClass` full name must
map through one unambiguous `UseQuestList` ID to exactly one of the 147 catalog
entries. All 147 IDs must be covered once. Missing, ambiguous, duplicate, or
unmapped data disables direct identity completion; only the generic
transactional state refresh remains. The retained map contains strings and
catalog indices only, never task classes, task actors, the container, or array
elements. Classes used only by non-catalog IDs are ignored, while a class shared
between a catalog ID and any other ID is ambiguous and fails closed.
Success or the second failure stops capture work; explicit F7 or completed
travel can rearm a failed map. If F7 is pressed while already active, that is a
mapping-only rearm and preserves exact-completion bits, revisions, and
repeatable-reactivation latches. A confirmed non-open-world activity defers a
pending attempt until open-world rendering resumes. There is no steady-state
metadata polling.

The exact completion callback publishes only a fixed catalog bit. Per-task
revisions prevent an older in-flight transaction from reviving the task, and a
fixed reactivation latch also rejects later still-active samples. A current
scan must first observe `NONE` or `END`; only a subsequent current scan may
restore a genuinely repeatable task on `ACCEPTABLE` or `PROGRESS`. `FAIL` is
neither completion proof nor an inactive boundary. Expanded-map task state is
invalidated at the numeric delta. A visibly open exact attachment may rebuild
once in the current session; a hidden retained layer defers until its next exact
`SetWorldMapImage` edge.

## Save owner binding

`save_owner_pointer.cfg` records a schema version, source executable
fingerprint, executable length, owner-pointer RVA, and provenance. It contains
neither the SQLCipher key nor an absolute process address. Matching executable
length and a valid live key make the packaged RVA the fast path. When a game
update invalidates it, the same below-normal save worker performs at most one
process-lifetime scan of executable PE sections, requires exactly one bounded
pattern target, caches only the numeric RVA, and validates the live key. It
adds no watcher, periodic scanner, game-thread work, or UObject retention.
SQLCipher work includes one full below-normal
attempt per F7 activation and may include a coalesced event-driven completion
confirmation after an exact area-quest witness remains inconclusive for ten
seconds. The valid single-owner F7 result records every requested task's exact
`COMPLETE_CNT`; only in that valid result does a missing ID mean baseline zero.
Any invalid or ambiguous owner/query leaves the baseline unknown, and that ID
queues no confirmation SQL. The first exact-ID attempt is due immediately; an
unresolved ID may retry twice at 15-second intervals, for three attempts total.
Only a count strictly greater than the known baseline confirms completion.
After three non-confirming attempts the same task generation is locked against
repeated events until new F7 or `NONE`/`END` then later
`ACCEPTABLE`/`PROGRESS`. Every confirmation skips treasure SQL. The fixed deadlines and attempt counters
do not constitute periodic save polling. Dynamic-task
results merge only positive completion IDs into activation-local native state,
so an older asynchronous snapshot cannot erase an exact or generic runtime
completion that arrived after the snapshot was taken. Encounter cooldown rows
likewise max-merge into the process-local effective map.
All three SQLite row callbacks catch exceptions before returning through the C
ABI and enforce fixed limits: 4,096 treasure rows and 65,536 encounter or
dynamic-completion rows. Invalid or excessive input fails only that one snapshot
closed.

## Runtime-only world time

World time uses one guarded numeric `DGameSingleton.TimeOfDay` baseline attempt
two seconds after F7, followed by local 60x `steady_clock` extrapolation. No
singleton UObject is retained and no retry occurs before a new activation.
The first valid displayed-hour sample after F7 and every later displayed-hour
edge each request one coalesced transactional area-quest refresh. The same
bounded edge service recomputes the fixed encounter visibility mask and linked
`MONSTER_ALIVE` task eligibility. It runs through the existing one-hertz
control edge; the 16 ms position path performs no hourly task or encounter
catalog scan. Weather is unavailable and unqueried.

## F6 visibility Hub

F6 opens one transient native UMG panel backed by two fixed numeric visibility
masks. The compact mask controls clock, treasure, Boss, Assault, mini-games,
area quests, and bird eggs. The expanded-map mask controls treasure, Boss,
Assault, mini-games, and area quests; clock and bird eggs have no expanded-map
renderer. Mini-games are one display category covering Fly, Mole, and Wave.
These masks filter presentation only; they do not mutate catalogs, save data,
cooldowns, quest states, completion latches, or provider scheduling.

The startup-only `[modes]` section stores `area_quests=available|all`. Available
mode consumes the existing strict prerequisite proof. All mode is an explicit
display override that includes unfinished tasks with unknown or ineligible
prerequisite proof, while saved and exact runtime completions remain excluded.
It does not add a provider query or change task-state authority.

It also stores `assault=available|all`. Available mode retains the authored
time-window behavior. Strictly valid legacy schema 1-4 files remain accepted;
the legacy `assault_mode=current` token maps to Available during upgrades.
Available mode preserves the normal Assault
state, time, and future 120-minute cooldown conditions. All mode displays the
immutable 40-record Assault catalog without consulting save readiness, time,
defeat, or cooldown during marker selection. Switching back to Available
restores those live filters. Boss availability and area-quest availability remain on their
strict paths. This presentation branch reuses the fixed 49-entry selection pass
and neither changes encounter-state authority nor adds a provider request.

The Hub retains only weak widget identities while open and never retains a
player controller. Its open panel services check boxes at a fixed 50 ms
interval. When closed it performs no UObject access, reflection, allocation,
polling, or file I/O. A changed selection publishes both masks in one
game-thread transaction together with both display modes and performs one atomic replacement of
`config/visibility.ini`; unchanged samples perform no write. The open-only
service conditionally restores the cursor if gameplay hides it. The file is
read once during native startup, is never hot-polled, and
is preserved by deployment. F8, travel, shutdown, and an explicit second F6
close remove the transient panel and clear its weak runtime handles.

The current 4 KiB-bounded document requires complete `[radar]`, `[map]`, and
`[modes]` sections. Radar and map categories use named `true|false` keys; modes
use `available|all`. Duplicate, mixed-format, unknown, incomplete, malformed,
or inconsistent-line-ending input falls back to safe defaults. A real F6
change upgrades an accepted legacy document by atomically writing the current
readable format.

## Adding or regenerating data

1. Regenerate data with the separate validated source-generation pipeline when
   a game update actually changes the relevant static records or coordinates.
2. Copy only deterministic outputs into `src/data/generated`.
3. Update native loaders and fixed bounds when the schema or record count
   changes.
4. Add core tests and static gates for headers, counts, identity joins, and
   fail-closed behavior.
5. Run deployment validation, then collect new gameplay evidence.

Never place PAK keys, SQLCipher keys, absolute process addresses, extracted
licensed data, or temporary extraction output in this repository or package.
