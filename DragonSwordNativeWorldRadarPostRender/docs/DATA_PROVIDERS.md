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
alignment pivot and exact native icon Canvas geometry, then produces numeric
native-parent-local atlas placement. The first valid numeric result only seeds
stability state. A sample at least 150 ms later is accepted when stable; if it
changed, one third sample may establish stability, for three samples maximum.
Only numeric coordinates and timing cross samples--no sampled UObject wrapper
or `FGeometry`. Missing or implausible geometry fails into the existing bounded
attach budget. No centered fallback, new timer, polling service, or per-frame
projection path is added.

The 2.2.1 candidate places each `Panel_Point` Image inside a Mod-owned, hit-test-
invisible child of the directly resolved current `DLayerMap.FogAbovePanel`.
`ArrayIconInfo` supplies only a creation-time instantiable icon class; it does
not select the parent, and retained-host validation/refresh does not scan it.
The outer slot is full stretch with zero offsets, `AutoSize=false`, alignment
`(0,0)`, and maximum Z. After every fresh attachment, including a scheduler-
accepted rebuild, the cloned host's inner `Panel_Point` slot is also forced to
full stretch with zero offsets,
`AutoSize=false`, and alignment `(0,0)`. Only the locally owned Image Canvas slot
holds `{atlas_left,atlas_top,atlas_width,atlas_height}`; Image render translation
remains `(0,0)`. The host has no render transform and the route uses no forced
layout prepass. Pan, zoom, clipping,
visibility, and RetainerBox composition are inherited directly, so same-parent
motion performs no viewport-transform sync. Every event-tail pass reads only
the live `FogAbovePanel` extent. A fully unchanged same-parent pass, including
the final pass, performs no layout/transform/visibility write, `ArrayIconInfo`
or `PlayerIconWidget` scan, restack, Remove/Add, or `RequestRender`. Sibling-icon
anchor drift is ignored and the attach-time Image slot stays immutable. A real
`FogAbovePanel` replacement reports `RebuildRequired`; only a scheduler-owned
fresh attachment restores the outer and inner fills, Image atlas rectangle,
and zero Image translation. The
finite event tail starts from a fresh post-attach completion time, and attach
does not submit an empty Retainer `RequestRender` before visibility is applied.
Extent drift must produce two matching successful observations before returning
`RebuildRequired`. That request is report-only and does not pre-collapse a valid
payload; only an accepted schedule owns detach/rebuild. Retained-RetainerBox or
owned-payload drift may report the same result directly. One open-map session
may rebuild at most once while preserving the marker snapshot. The rebuild has
its own three-attempt attach/geometry budget, bounding the session to initial 3
plus rebuild 3. No
`3000`/`8000` constant replaces live geometry, and no new timer, poll, or per-
frame projection route is added.

Temporary 2.2.1 topology logs instead captured the superseded first-valid-parent
heuristic changing between `FogAbovePanel` and `FogUnderPanel` as zoom rebuilt
native icon widgets. Four host reattachments in one zoom sequence crossed the
fog boundary and explain the observed occlusion, hitching, and flashing. These
logs are development diagnosis and do not validate the corrected runtime.

The subsequent full-stretch-outer/Image-translation candidate is also runtime
rejected. Quantitative 2026-09-05 screenshots measured near-identical base-map/
Radar zoom scales (1.214/1.218 zooming in and 0.760/0.758 reversing) but relative
translations of about `(+113,-190)` and `(+67,+200)` px, with only 0.28 px and
0.82 px mean fit residuals. The sign-reversing vertical offset rules out a scale
formula or cumulative frame-drift cause and identifies a different local origin/
zoom pivot. The following outer-atlas-rectangle candidate is runtime rejected as
well. Exact DLL
`CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2`,
compiled source
`433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`, and
backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`
remain exact-byte historical evidence. Its negative atlas-left outer slot grew
the native parent extent from `3000x3000` to `3191.520996x3000`, causing a self-
induced rebuild loop with 6 attaches and 5 detaches, flashing, and an empty map.
The same run populated 1,632 markers and reported no data, texture, or ABI fault.

The current WM-06 immutable-slot candidate passes source/static gates, Core
`2/2`, release hygiene, and the local native build at DLL
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`, from
compiled source
`0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`, size
1,107,968 bytes. Rollback-backed diagnostics-enabled local developer deployment
passes for the exact DLL. Installed identity matches, the single controlling Mod
entry is enabled (`mods=1`), and `debug_logging=true`; backup:
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
Package and installer validation pass for the exact WM-06 bytes: Setup `20/20`,
Manual `2/2`, payload equivalence, layout, clean-target, and all three archive
re-extractions. Exact-artifact gameplay, visual, performance, and resolution
acceptance remain `NOT_VALIDATED`.

Historical exact 2.2.0 logs recorded one atlas attach and no repeated
detach/rebuild sequence. Their 1,632 total markers, including 1,501 Treasures,
were below the old 1,785 limit, proving capacity was not that dense-map flicker
  root. Neither historical nor temporary diagnostic logs validate the 2.2.1 ownership fix; exact-artifact
alignment, dense-Treasure stability, native icon/click behavior, and gameplay
acceptance remain pending.

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
Only safe live-GameThread finalization and the TitleMap owner boundary may
hard-clear the fixed mask. True process teardown closes ingress and returns;
UObject-array shutdown does not clear it. The
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

#### 2.2.0 shared mini-game height contract

Each of the 83 map-100 Fly/Mole/Wave records has one trusted compact-arrow height
derived offline from the exact actor `MiniGame_<kind>_<id>_NPC_Start`. The generator joins
the pinned mini-game rows to the pinned ActorPositionData table, requires exact
one-to-one identity and finite numeric coordinates, and emits immutable numeric
rows. The two input hashes and generator boundary are recorded in
`DEPENDENCY_SOURCES.md`.

The runtime never extracts PAKs, parses XML, scans the global UObject array, or
reads the filesystem for mini-game height. The target is compared with
`playerZ - 150`; the inclusive +/-500 interval hides the triangle, while values
above or below it point up or down. A missing, duplicate, or invalid trusted
height fails closed by hiding only the shared mini-game triangle; the ordinary
marker remains governed by its existing catalog and visibility rules.

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

#### 2.2.0 height-band contract

The compact Area Quest height presentation uses five additional columns in
`area-quests.tsv`: `Height1MinZ`, `Height1MaxZ`, `Height2MinZ`, `Height2MaxZ`,
and `HeightBandCount`. The original `Id`, `X`, `Y`, and `Z` columns remain the
validated MnMRadar catalog and retain their existing ordering and marker-position
meaning. The height-band profile is generated offline from:

`../DragonSwordWorldDataProbe/reference/assault-support/xml/017_ActorPositionData.xml`

The pinned ActorPositionData SHA-256 is
`11CA916050AFA25F856DF0AFF46CAE0D23F928E8490617FBD3B524DC183E3ACF`.
`tools/Build-AreaQuestHeightCatalog.ps1` rejects any other source hash. It
considers only map-group 100 Actors whose `Name` contains the exact catalog ID
with numeric boundaries and whose position is within 1,000 planar Unreal units
of the MnMRadar X/Y anchor. Candidates are ordered deterministically and grouped
by Z; a gap greater than 250 Unreal units begins a second band. A
Move_Check-only band is an activation/traversal trigger rather than a task-actor
destination and is excluded when a real task-actor band exists. Each retained
band publishes its minimum and maximum Z, and the fixed format accepts at most
two bands. A no-source row stores zeroes with `HeightBandCount=0`.

Run this reproducibility check from the project root before packaging:

```powershell
& .\tools\Build-AreaQuestHeightCatalog.ps1 -VerifyOnly
```

The 2.2.0 contract is exactly 147 rows: 144 rows have height profiles, task
`1103061` has the sole genuine two-band profile, and three rows have no source
profile. Task `1110037` is a single band at `24702`; its discarded Move_Check
trigger is not a second destination band. No-source markers remain eligible for
ordinary compact and world-map display, but their compact height presentation
is neutral. A single-band profile is used directly. For task `1103061`, authored
marker Z selects the uniquely nearest existing source band; it is selection
evidence only and never replaces source height. An exact-distance tie fails
closed neutral. The shared `-150` comparison offset calibrates the player's root
position for Treasure, Area Quest, and mini-game guidance only; it is not an
Area Quest height source and is never applied while generating a profile.

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
length and a valid live key make the packaged RVA the fast path. The pointer
RVA and the key member offset inside its owner are separate bindings. The game
build identified by Steam build `25076183` uses RVA `0x94F4FA8` and moved the
key `FString` from owner offset `0x120` to `0x128`. When the RVA is stale, the
same below-normal save worker performs at most one process-lifetime scan of
executable PE sections and requires exactly one bounded pattern target. It then
tries the cached member offset, legacy `0x120`, verified current `0x128`, and a
bounded aligned fallback with at most 24 structurally valid candidates per
explicit F7. A candidate must decrypt the active `.db` and read
`sqlite_master`; `.bak` alone is never sufficient. Only numeric RVA/member
offsets are cached. No key bytes or absolute process address are logged or
persisted, and the path adds no watcher, periodic scanner, game-thread work, or
UObject retention.
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

## F6 responsive settings page

F6 opens one transient native UMG page backed by two fixed numeric visibility
masks, three height switches, two filter modes, and one language preference.
The compact mask controls clock, treasure, Boss, Assault, mini-games,
area quests, and bird eggs. The expanded-map mask controls treasure, Boss,
Assault, mini-games, and area quests; clock and bird eggs have no expanded-map
renderer. Mini-games are one display category covering Fly, Mole, and Wave.
These masks filter presentation only; they do not mutate catalogs, save data,
cooldowns, quest states, completion latches, or provider scheduling.

The startup-only `[height_arrows]` section stores independent Treasure, Area
Quest, and Mole `true|false` values. All three default ON on a clean install; a
valid existing configuration keeps its values. These switches control compact-
radar height guidance only and never hide a marker. Every visible Area Quest
uses its generated height-band profile. For a multi-band profile, authored
marker Z first chooses the uniquely nearest existing source band. Within that
selected band expanded by the inclusive +/-500 vertical-unit margin, its normal
black frame shows three white dots. A player below the selected band gets an up
triangle and a player above it gets a down triangle. An exact-distance tie or
missing source profile gets neutral presentation with no dots or direction.
The shared mini-game channel also uses an inclusive +/-500 interval. All three
height channels compare against `playerZ - 150`, while Treasure retains its
existing dead-zone behavior.

The `[interface]` section stores one of the 11 explicit language choices. A
legacy AUTO value is accepted only as migration input: the next actual F6
opening or F7 activation resolves `DGameUserSettings.LanguageText`, then Kismet
and English, and persists the matching explicit language. AUTO/Use Game
Language is not displayed. F6 selects already-loaded game Font objects by
script: `DsCompositFont_CommonSystem` for Korean and Latin/Cyrillic languages,
`DsCompositFont_TCSystem` for both Chinese choices,
`DsCompositFont_JPSystem` for Japanese, and `DsCompositFont_THSystem` for Thai.
Missing loaded-font evidence falls back safely without changing the language,
guessing an asset path, or replacing FontMaterial. If the constructed widget reports exactly `Font.Size == 0`,
one bounded reference size is seeded. A game-widget construction, target-size,
or font-commit failure retries that text once as base UMG `TextBlock` during
the same open. The real reflected
`Font.Size` participates in layout under a slot-safe
line-height bound. After `AddToViewport` and layout prepass, font size is
reapplied and read back; a missing core Font/SetFont ABI or failed size
verification remains fail closed. Font evidence never changes the language.
Korean and Traditional Chinese fixed labels also use generated 2x overlays from
pinned DroidSansFallback at base size 32, with a one-pixel translucent stroke
and role-specific optical baselines. Static generation verifies that no overlay
slot clips; live visual acceptance remains required. The provider does no
recurring language query.

The expanded-map renderer reserves 4,096 fixed marker slots. The accepted input
ceiling is 2,500 Treasure rows plus 279 fixed non-Treasure rows, or 2,779 total,
leaving 1,317 spare slots. Its two event-built 2048-by-2048 atlases use atlas
style revision 51 and approximately 32 MiB raw decoded BGRA memory. The cache
envelope is `DSNWRA52`, separate from the atlas style revision, and quantizes its
fingerprint at 1/4096 UMG logical unit. A hit requires exact dimensions/header/
magic/fingerprint/visible count, full RLE decode to exactly 2048-by-2048 pixels,
encoded-payload checksum validation, and exact EOF. Revision-51, corrupt,
truncated, or trailing-byte files miss. Writes use a same-directory temporary
file and atomically publish through `MoveFileExW` with replace-existing and
write-through flags; failure removes the temporary file. Cache validation/write
and texture import remain bounded attachment work. Marker coordinates remain unchanged. Live visual and performance
acceptance remains `NOT_VALIDATED`.

F6 may open while Radar is Off, On, or Faulted. Its status/action transaction
exposes Enable, Disable, or Retry without changing the playable-world guard for
activation. Bug Report launches the fixed Nexus Posts URL. These are explicit
user actions and add no steady provider schedule.

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

The page retains only weak widget identities while open and never retains a
player controller. Its open panel services check boxes at a fixed 50 ms
interval. When closed it performs no UObject access, reflection, allocation,
polling, or file I/O. A changed selection publishes both masks in one
game-thread transaction together with both display modes, height choices, and
language preference and performs one atomic replacement of
`config/visibility.ini`; unchanged samples perform no write. The open-only
service conditionally restores the cursor if gameplay hides it. The file is
read once during native startup, is never hot-polled, and
is preserved by deployment. F8, travel, shutdown, and an explicit second F6
close remove the transient panel and clear its weak runtime handles.

The current 4 KiB-bounded document requires complete `[radar]`, `[map]`,
`[modes]`, `[height_arrows]`, and `[interface]` sections. Radar, map, and height
categories use named `true|false` keys; modes use `available|all`; interface
stores one supported language token. Duplicate, mixed-format, unknown,
incomplete, malformed,
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
