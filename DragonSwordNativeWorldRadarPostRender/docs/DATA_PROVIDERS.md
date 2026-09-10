# Data Sources and Runtime Providers

This native project does not contain or run the legacy C# installer, Overlay,
watcher, Lua provider modules, PAK extractor, or `ooz.exe`. The local files
under `src/data/generated` are immutable inputs copied from the previously
validated DragonSword World Radar generation pipeline. Deployment validates
their headers, counts, and cross-catalog identities before copying them.

Current source is **3.0.0 / SG-12**. It standardizes fonts and wording, adds
scrolling/responsive settings and validates frame-local scene projection.
Provider data, Scene points, height classification and filter behavior are
unchanged. Mini-games covers the existing Fly/Mole/Wave height channel.
See [Release Status](RELEASE_STATUS.md) for the current artifact and delivery evidence.

At the historical **3.0.0 / SG-10** checkpoint, provider behavior was unchanged.
SG-10 adds localized, in-page action confirmations without changing provider,
catalog, Scene, height or filter behavior. Core 7/7, clean native 446 targets,
all four source/release gates and F6 resource checks pass. Its 1,310,208-byte
DLL `F1203366...EC7EB65` is installed and independently verified at
`2026-09-09T16:51:20.6801835Z`: 34 UI files/build receipt match, four settings
files retain their bytes, and two checked AutoPickup files remain unchanged.
Backup: `dist/work/deployment/deploy-backups/20260909-095118-801-native-only-deploy`.
The `2026-09-09T16:53:04.9764235Z` release manifest and candidate
`package-verification.json` establish Setup 20/20, Manual 2/2, 65 equivalent
runtime files, 4/69/73 ZIP entries, 64 hashed payload files per manual ZIP,
60 installed static files and final-directory promotion at
`2026-09-09T16:55:11.2153219Z`. The SG-03 set is backed up in the SG-10
candidate's `previous-final-sg03` directory. Gameplay remains unvalidated;
Nexus publication has not been performed. Frozen build metadata and
`validation-summary.json` retain their pre-deployment phase; subsequent
deployment/package receipts establish the completed state without rewriting
those snapshots.

At the retained SG-09 checkpoint, installation was verified by its candidate
`deployment-verification.json` at
`2026-09-09T15:52:57.0256520Z`, DLL `EC82CA04...800F1BF` (1,299,456 bytes).
The DLL, 34 UI files and build receipt match; four settings files and the two
checked AutoPickup files remain byte-identical. The prior SG-08 installation
is backed up at `dist/work/deployment/deploy-backups/20260909-085255-045-native-only-deploy`.
The new package manifest at `2026-09-09T15:56:21.9516109Z` verifies three ZIPs
with 4/69/73 entries and 65 equivalent runtime files, Setup 20/20 and Manual
2/2, without failures/skips. Those SG-09 candidate packages were not promoted;
`dist/final-3.0.0` held SG-03 until the subsequent SG-10 replacement.
Packaged metadata intentionally retains its pre-deployment snapshot; the
later actual installation receipt supersedes that snapshot's installed status.
Provider, source, installed and package identities remain separate; see
`SCENE_GUIDANCE_ATTEMPT_LEDGER.md` and `RELEASE_STATUS.md`. SG-09 gameplay,
visual and performance acceptance is unrecorded. Named historical records
retain their original artifact scope; no Nexus publication is claimed.

The earlier SG-02 documentation closeout updated source metadata descriptions
and installer defaults for the six-section/three-mask/five-height contract.
These metadata revisions postdate the installed SG-02 verification checkpoint.
They were documentation/default-profile changes only: that running DLL and
installed metadata/configuration snapshot were not replaced at the time. Use the retained
`installed-verification.json` for that deployment's identity, rather than
assuming today's source metadata hashes equal the installed copies.

## SG-09 / 3.0.0 Scene providers

Scene reuses immutable catalog positions and existing completion/eligibility
snapshots. Treasure, Area Quest and the 83 Fly/Mole/Wave Mini-games have separate
Scene switches, independent of compact and expanded-map visibility. Mini-game
guidance denotes each activity's authored catalog location, not its reward
chest. Area Quest markers retain gray diamond rails and three horizontal white
dots, now with a restrained translucent gray-blue interior backing. Their
footprint and lower v are unchanged; Mini-games use purple crossed flags;
Treasure keeps its palette and
lid/clasp chest. Treasure/Mini-game glyphs remain approximately 18-19 reference
units wide; SG-06 enlarged the quest diamond body by 18%, to approximately 22,
with a 1.8-unit outline. All retain a small `v` below. Six shared 128-by-144
textures contain four Treasure variants, one quest and one Mini-game image. Fifty
fixed marker Images replace 600 individual Border pieces. Six collapsed keeper
Images retain all textures through verified reflected Brush references;
imports happen only during bounded attachment, including currently unused
kinds. Changing a marker kind changes its image brush without importing again.
These display assets introduce no provider or catalog changes.
These are static authored targets, not proof of a loaded actor, terrain
visibility, current interaction range or current quest-stage destination.

`area-quest-scene-anchors.tsv` is a separate 147-row companion with the exact
header `Id X Y Z SourceAvailable` (tab-separated). A valid row carries one
complete source XYZ and `SourceAvailable=1`; an unavailable row carries three
zero coordinates and `SourceAvailable=0`. The pinned source is the same local
ActorPositionData XML documented below, SHA-256
`11CA916050AFA25F856DF0AFF46CAE0D23F928E8490617FBD3B524DC183E3ACF`.
Only numeric coordinates/IDs and availability enter the runtime TSV; private
XML, entity names and game-asset paths are not copied into the payload.

`tools/Build-AreaQuestSceneCatalog.ps1 -VerifyOnly` reproduces the TSV and its
verification metadata, runs finite/selection/ambiguity self-tests, and verifies
that the original `area-quests.tsv` is unchanged. Normal selection requires an
exact same-task ID in map group 100, within 10 m of the original XY and in the
uniquely selected existing height band. AcceptActor, named NPC and interaction
entities take precedence; MoveCheck and generic position/Empty proxies do not
become physical targets. Equally ranked distinct positions remain unavailable.
Every selected point retains the source's full XYZ rather than mixing sources.

SG-09 raises only the displayed Treasure point from +100 to +160 cm; Area
Quest remains +180 cm and Mini-game +150 cm. No raw catalog coordinate, selected anchor, compact/map
height profile or range/selection distance is changed. Integer Scene text uses
one total correction before rounding: Treasure/Area Quest `max(0, d - 1)` m,
Mole `max(0, d - 2)` m, Fly/Wave unchanged. These are display conventions,
not calibrated interaction distances or evidence of exact object-center height.

The reviewed exception is task `1110038`: its old band was derived from an
Empty proxy, while source ActorID `560707418635547564` is the unique named task
entity at 10.79 m planar distance. The generator pins that identity and an
inclusive 11 m limit, using `(263801,184814,7864)` for Scene only. Task `1110033`
uses its AcceptActor at `(266388,82392,-687)` instead of raw marker Z `19502`.
The sole two-band task `1103061` retains the old marker-Z band selection and
uses the complete Backpack point `(84840,166450,6620)`.

Coverage is exactly 143 available / 4 unavailable. `1101301` has MoveCheck and
two StepPosition records, but no confirmed start/interaction entity; `1103108`,
`1104104` and `1104203` have no exact task-ID source anywhere in the full XML.
These IDs remain ordinary compact/map tasks; only their unproven Scene anchors
are withheld. Metadata records the fixed unresolved IDs, reasons, selected
numeric source IDs and hashes. This establishes authored-source provenance,
not gameplay acceptance or ground-level calibration.

The main-thread loader stages and checks the entire <=64 KiB companion before
assigning any Scene position. A missing/malformed companion disables Area Quest
Scene anchors only. The payload allowlist includes the numeric TSV, and local
deployment verifies the installed TSV hash. Numeric candidate refresh is
bounded to 250 ms and the existing catalogs. Every EngineTickPost frame projects
at most 50 selected points, with a linear distance refresh instead of sorting
the retained candidates each frame. Defaults are 600 m / 24, configurable to
1000 m / 50.
No global actor enumeration, trace, save query or new provider poll is added.

Distance Off does not create text resources. Other modes use lazy TextBlocks,
a bounded integer-meter FText cache (0..1000), 100 ms ordinary refresh and
changed-value writes; focus changes can refresh immediately. Cold work is
limited to four new TextBlocks and eight FText values per update. Pending labels
wait without inheriting stale identity text; a text fault disables labels while
Scene icons remain available. Scene lifecycle, projection and text caching are
described in `ARCHITECTURE.md`. Aim Focus uses a centered ellipse with radii of
16% horizontally and 34% vertically of the shorter viewport side. It chooses
the nearest eligible marker by normalized ellipse distance and requires 120 ms
continuous identity dwell. Auto Focus still uses Euclidean center distance and
the separate 15% focus-switch buffer. Losing a valid Aim target immediately
hides its distance label. Only the Treasure UI lift changes in SG-09;
displayed-distance corrections and raw range checks remain unchanged.

Render translation is skipped while accumulated displacement from the last
submission is at most 0.25 physical pixels. Larger motion uses
SetRenderTranslation instead of Canvas-slot SetPosition, reducing layout
invalidation. Smaller sampled motion accumulates until the threshold is
exceeded. Projection runs each frame; eligibility stays at 250 ms. Own F6
Settings permits Scene previews over active gameplay, while real game menus,
pause, world map, activities and HUDHidden still suppress them.
Routine menu sampling preserves
Scene renderer state and the existing widget tree, collapsing/revealing the
host at suppression edges instead of detaching it. Disable, world/travel/reset
and fault cleanup retain their own boundaries. These changes reduce widget and
reflected-call work in source; no live FPS improvement has been measured.

SG-04's 12-piece marker groups and 10% circular Aim region remain historical.
SG-04 was not deployed separately. SG-05 retains its deployment checkpoint;
SG-08 was subsequently deployed with its own verification receipt as noted
above. No replacement SG-08 release packages were generated.
Its then-future contract expected 65 runtime files and manual archive counts
69/73. SG-09 verified those candidate counts without replacing the final
directory. SG-10 subsequently replaced the final set and backed up the SG-03
archives and their 39-file identity in its `previous-final-sg03` directory.

## Save-owner lifecycle boundary

The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is a hard
boundary between save owners. Existing transition-end and world-identity
signals disable the radar, detach compact, expanded-map and Scene hosts, clear runtime weak candidates,
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

### Historical 2.2.x provider evidence

These exact-byte logs and package results are preserved as history; they do not
identify or accept the current 3.0.0 test build.

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

The historical WM-06 immutable-slot candidate passed source/static gates, Core
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
all Treasure presentations, including Scene. Verification binds the override to that exact record and the
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

#### Shared mini-game height contract (introduced in 2.2.0, retained in 3.0.0)

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

#### Map/compact height-band contract (introduced in 2.2.0, retained in 3.0.0)

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

The retained contract is exactly 147 rows: 144 rows have height profiles, task
`1103061` has the sole two-band profile, and three rows have no source
profile. Task `1110037` is a single band at `24702`; its discarded Move_Check
trigger is not a second destination band. No-source markers remain eligible for
ordinary compact and world-map display, but their compact height presentation
is neutral. A single-band profile is used directly. For task `1103061`, authored
marker Z selects the uniquely nearest existing source band; it is selection
evidence only and never replaces source height. An exact-distance tie fails
closed neutral. The shared `-150` comparison offset calibrates the player's root
position for Treasure, Area Quest, mini-game and Boss/Assault guidance; it is not an
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

SG-09 Clock positioning reuses that existing 1 Hz layout service. It measures
the minimap RetainerBox and the same owner's DLayerQuest, placing the glyph's
15-unit visual center in their vertical gap when the gap is at least 30
reference units and the group fits the screen/host. The 42-unit group's
geometric center (21) is not used as the visible glyph center. Exact ownership
requires the main DLayerMiniMap back-reference, within eight ancestors;
paint visibility walks at most 24 nodes. Invalid geometry retains top offset
178 from the minimap center; resize defers a geometry sample. Applied-position
deltas accumulate against 0.25 reference units. No new scan, time source or
timer is introduced, and live visual alignment is still unverified.

## F6 responsive settings page

F6 opens one transient native UMG page backed by three fixed numeric visibility
masks, five height switches, two filter modes, one language preference and
Scene range/count/distance settings. Its reference layout is 760 by 852, with
a compact Language/Status row, a two-column Radar/Map table and separate Scene,
Height and Filter cards. Scene and Height use horizontal category chips; Filter
groups sit side by side. The original four cards retain their coordinates;
Reset to Defaults, Endorse and Feedback occupy a fifth footer card at Y 794..844.
Close remains in the title bar. Popup dismissal and modal shielding include the footer.
Radar/Map use square 22-unit checks with non-overlapping 24-unit hits, and Clock
appears after Bird Eggs. Card placement and language modal bounds share the
same derived layout. Scaling uses a 1920-by-1080 reference and live viewport DPI.
The compact mask controls clock, treasure, Boss, Assault, mini-games,
area quests, and bird eggs. The expanded-map mask controls treasure, Boss,
Assault, mini-games, and area quests; clock and bird eggs have no expanded-map
renderer. Mini-games are one display category covering Fly, Mole, and Wave.
The Scene mask admits only Treasure, Area Quests and Mini-games and defaults to
all on for clean/missing defaults; valid stored false values are preserved.
Its two real USliders select shared range (0..1000 m) and count (0..50);
defaults are 600 m and 24. Four public distance labels are Off, Aim Focus
(default), Auto Focus and All; existing stored tokens remain unchanged. Either
zero numeric setting suppresses the full Scene
service; Distance Off suppresses only labels.
After Yes confirmation, Reset to Defaults resets every F6 display preference: all supported
Radar/Map categories and all five heights on, Scene categories on / 600 m /
24 / Aim Focus, both filters Available and language AUTO. Mod enabled/fault
state and startup hotkeys are preserved. The existing preference path flushes
this explicit reset, including pending slider edits when controls already
match. This action does not reset providers, save state or cooldowns.
Reset, Endorse and Feedback all require an in-page Yes confirmation. No or
Esc dismisses the confirmation only; it changes no preference and opens no
website. Endorse opens the Mod's Nexus page for the user to endorse there,
without submitting endorsement. Feedback opens its Posts page. The modal
blocks background controls and consumes its dismissal edge. No provider or
save mutation is added; no blocking OS dialog is used.
These masks filter presentation only; they do not mutate catalogs, save data,
cooldowns, quest states, completion latches, or provider scheduling.

Outside a confirmation, Escape closes the entire Settings page, including an open language popup,
after sampling all controls and preserving the last slider value. A verified
process-local window-thread message hook consumes that Escape press, repeat
and release before normal game message dispatch. It does no Unreal/provider
work. Hook setup failure prevents opening the page; loss of focus closes it.
Offline input-routing tests do not establish gameplay menu acceptance.

The `[height_arrows]` section stores independent Treasure, Area Quest, Mole,
Boss and Assault `true|false` values. The internal `mole` key and `HeightMole`
identity remain unchanged; their localized label is now Mini-games and covers
all 83 Fly/Mole/Wave points. All five default ON on a clean install; a
valid existing configuration keeps its values. These switches control compact-
radar height guidance only and never hide a marker. Every visible Area Quest
uses its generated height-band profile. For a multi-band profile, authored
marker Z first chooses the uniquely nearest existing source band. Within that
selected band expanded by the inclusive +/-500 vertical-unit margin, its normal
black frame shows three white dots. A player below the selected band gets an up
triangle and a player above it gets a down triangle. An exact-distance tie or
missing source profile gets neutral presentation with no dots or direction.
The shared mini-game channel also uses an inclusive +/-500 interval. Boss and
Assault use their existing static spawn Z as a single-point band, compare the
same `playerZ - 150` against the same inclusive +/-500 margin, and restore the
original glyph when aligned. Missing/invalid encounter source preserves that
glyph without claiming alignment. No extra monster-center correction is added.
All five height categories compare against `playerZ - 150`, while Treasure
retains its existing dead-zone behavior. The 3.0 compact sizes are 35/30/25 for
Boss/Assault/Area Quest, with the same 4-unit visible stroke/frame thickness
scaled by display/DPI; Encounter triangles add a 1-unit dark-green outline on
each side of that visible core.

### SG-12 font resources

All eleven languages now use pinned regular-font pixels for 44 main slots in
each of three status states, all twelve popup choices, 32 help topics, seven
confirmation strings and fourteen numeric/symbol tiles. Footer actions all use
13 reference units / 26 raster pixels. There are 53 F6 TGAs and one manifest;
with Scene's six TGAs and manifest, the UI inventory is 61 files. Main sheets
are 1520x1704 and each language atlas is 640x7632, below the 8192 limit.
Coverage checks validate 423 main, 750 tooltip and 250 confirmation codepoints,
including Thai combining marks and Latin accents. Forty-three isolated resource
regressions are rejected. No runtime font asset or per-frame discovery is added.
The fixed header/body/footer share each main texture through separate clips;
only the middle body scrolls. Native text remains a bounded regular-font
fallback; missing confirmation resources remain cancel-only.

### Historical interface/font route through SG-11

The following route records the earlier native/partial-raster font implementation.
SG-12's complete raster coverage and fixed font roles above supersede its
partial coverage, stroke and inventory figures.

The `[interface]` section stores AUTO or one of 11 explicit language choices.
AUTO is the clean default and the first popup option; it remains persisted.
Each actual F6 opening or F7 activation resolves `DGameUserSettings.LanguageText`
with a bounded Kismet fallback. Invalid input retains the last valid detection,
using English only before the first valid sample. Detection does not overwrite
the stored preference. A manual choice remains fixed until AUTO is selected
again; the selector face shows the resolved language and the popup highlights
AUTO when following is enabled. F6 selects already-loaded game Font objects by
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
slot clips; live visual acceptance remains required. SG-11 uses 11 languages
/ 75 strings (43 main strings plus 32 specific tooltip explanations) and 44
Korean/Traditional Chinese main-overlay slots. Popup raster
coverage is now Korean, Traditional Chinese, French and Spanish, independent of
the current page language. French/Spanish names use a pinned build-only
Liberation Sans input with its OFL license, because the existing Droid fallback
does not contain ç/ñ. Correct mother-tongue strings are unchanged.
Two 440-by-52 images also cover the selected French/Spanish LanguageValue,
including AUTO resolution; other main-card/body text stays native. Successful
Brush.ResourceObject readback precedes hiding that one TextBlock; failures
retain native fallback, and two weak texture/failure slots reset with the page.
The fixed-label inventory remains nine RLE TGAs and 180 required codepoints;
seven full-panel text images are 1520 by 1704. SG-06 introduced six shared glass/check/
chip skins and 11 tooltip atlases, for 26 TGAs plus a schema-3 manifest.
Strict source/asset checks cover 11 x 3 x 44 language/status slots, 352 complete
tooltip tiles, 77 transparent confirmation-text tiles and all 27 retained SG-10
in-memory regressions. Earlier named regression checkpoints retain their scope.
Popup selection is 186-by-28 inside its 190-by-32 cell with inset two; 48 bounds
and actual Border-call binding guard against the old cross-column highlight.
Popup cells start at `(78,101)` and use the existing 202/40 column/row steps.
LanguageValue moves to `(126,58,220,26)` without expanding the name-only fallback
to other body text. The provider does no recurring language query or new runtime
font scan. These source assets are not present in the retained SG-03 archives.

Hover help reads 32 strings from each language's C++ record. Seven category
topics briefly describe the Radar/Map content: Treasure, Boss, Assault,
Mini-games, Area Quests, collectible Bird Eggs and the game-world Clock. Category
row names have separate hover targets, so help remains reachable when native
labels are hidden by a main-text overlay. Three Scene topics explain their
independent visibility and shared limits. Five height topics explain direction
guidance; the Mini-games label covers Fly, Mole and Wave while preserving the
existing configuration key. Activity points and reward chests remain distinct.
Four short filter explanations distinguish Available from All. Thirteen common
topics retain range, count, four distance modes, language, status/action,
global reset, support, close and Endorse. Range/count help explains their shared
scope across enabled Scene categories, zero hiding markers and the possible
performance cost of higher counts. Aim describes a brief pause while aiming;
Auto describes selecting near the screen center with stable switching, without
exposing implementation percentages or timing constants in the UI.

Range is selection distance. SG-09 uses projection-only +160/+180/+150 cm lifts;
the original compact/map height parameters remain unchanged. Each 640-by-5616
atlas holds 39 tiles and stays below the 8192 dimension bound. One tooltip displays
at 320 by 72 reference units through a clipped native SizeBox/Canvas/Image.
All 352 tooltip tiles preserve complete source text, ASCII tokens and attached Thai
combining marks. The 760 required tooltip codepoints use pinned Droid Sans
Fallback, Liberation Sans and Noto Sans Thai. Font files are build inputs only,
with provenance/licenses in `tools/f6-fonts`.

Fifty-six control owners fit the existing 64-record pool, each retaining a
separate tooltip widget; only the language atlas is shared. Standard reflected ToolTipWidget and Brush ownership
retain its content. Open/language edges perform bounded imports and keep one
attempt state; fallback clears stale custom-language content before native text.
Travel/detach clears weak records. Slate handles actual hover timing/placement:
there is no hover polling, additional provider scan or per-frame resource I/O.
The last seven atlas tiles contain transparent Endorse/title/three-body/Yes/No
text from a separate seven-string localization record. This adds 245 checked
confirmation codepoints and brings the total to 82 strings per language,
902 across all 11. Each action body is one short question without Yes/No
instructions. The 206-by-24 Endorse crop fits every language at the full 26-pixel
font; no label shrinking is needed. Five bounded native consumers share the same atlas. Header
dimensions reject old tooltip-only files before import; verified brush binding
precedes hiding native fallback. Missing confirmation text leaves cancellation
readable and Yes disabled. The existing 26 F6 TGAs plus manifest and six Scene
TGAs plus manifest remain 34 UI files; no runtime font asset is added.
SG-09 uses soft blue-gray surfaces, restrained gradients and active accents.
The material change adds no provider, coordinate, height-threshold, focus-rule,
preference, localization-text, layout or file-count change; the separate
Treasure UI lift and Scene scheduling changes are described above.
Direct tests of the final main TGA include 3,384 body points at alpha 216/255
(15.294% background transmission); current SG-11 gap samples transmit 21.961-23.922%. Tooltip reading
surfaces stay 96-99% opaque. The pixel gate also checks controlled blue-gray tint and at least
4.5:1 small-text contrast over linear-light white; the card-body static minimum for
RGB (247,253,255) is 4.522:1. The 118 main/Endorse/value/popup text-slot states are also
checked against their actual layered control pixels; all small text measures
at least 4.522:1, with a separate 3:1 large-title requirement. Utility buttons,
the language strip and a narrow status plate retain reading protection. Native
status fallback independently passes 4.613:1. SG-08's 62 regressions remain
historical; SG-09 adds 21 focused pixel counterexamples (14 material, seven
Scene-backing) and two native status-color regressions independently of hashes.
Both sRGB and linear synthetic-background previews remain static readability
bounds, not verified in-game compositing or gameplay evidence.

Six procedural skin textures add rounded gradients and fine glass-like edges,
without runtime background blur or third-party artwork. Chip Box nine-slice
margins preserve ten-reference-unit corners across widths; square checks stay
square. Native Border colors decode authored sRGB once, while texture import
keeps its sRGB path. Source preview QA is not gameplay or FPS acceptance.
SG-04's 680-by-896 panel, 41 slots, 42 strings per language, 170 codepoints and
560-by-54 name images remain a source/build-only historical checkpoint.

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
player controller. Its open panel services check boxes and sliders at a fixed
50 ms interval. The closed UI path performs no UObject access, reflection,
allocation or polling. A changed selection publishes all three masks in one
game-thread transaction together with both display modes, height choices,
Scene settings and language preference. Ordinary option changes atomically
replace `config/visibility.ini`; Scene range/count/distance changes instead
apply immediately and debounce disk writes for 300 ms. The owner flushes a
pending write on panel close, so that one flush is an explicit exception to
closed-panel file inactivity. Unchanged samples perform no write. The open-only
service conditionally restores the cursor if gameplay hides it. The file is
read once during native startup, is never hot-polled, and
is preserved by deployment. F8, travel, shutdown, and an explicit second F6
close remove the transient panel and clear its weak runtime handles.

The current 4 KiB-bounded writer emits six complete sections: `[radar]`, `[map]`,
`[scene]`, `[modes]`, `[height_arrows]` and `[interface]`. Boolean keys are named,
mode keys use `available|all`, Scene range/count are bounded integers, distance
mode uses one of its four supported tokens, and interface stores AUTO or an
explicit language token. The reader preserves strict legacy packed documents
and the old three/five-section layouts. Missing new Scene/Boss/Assault fields
use defaults without replacing existing choices; a present Scene section still
requires its original Treasure and Area Quest keys. Current serialization and
public default validation require all six Scene keys and five height keys.
Duplicate, mixed-format, unknown, incomplete legacy-required, malformed or
inconsistent-line-ending input falls back to safe defaults. A real F6 change
upgrades an accepted legacy document atomically. `hotkeys.ini` is a separate
startup-only file and is never rewritten by F6.

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
