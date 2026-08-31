# Project Context

## Ownership

`DragonSwordNativeWorldRadarPostRender` is the active native-only successor to
DragonSword World Radar. Its source root is this directory. Its installed root
is:

`DS/Binaries/Win64/ue4ss/Mods/DragonSwordNativeWorldRadarPostRender`

`DragonSwordWorldRadarObjectState` is a superseded external-renderer baseline
available through Git history, not a current working-tree project. Do not
modify or deploy other Mods from this project.

## Current release

- Version: maintained `2.1.0` with current source/static/native-build,
  Setup `20/20`, manual-copy `2/2`, payload-equivalence, and archive gates
  passed; owner gameplay acceptance was reported on 2026-08-31.
- Runtime label: `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0`.
- Version `1.2.0` was never published; its candidate work is included in
  `2.1.0` and has no separate public artifact line.
- Package shape: UE4SS native `main.dll`, immutable generated catalogs,
  SQLCipher runtime library, configuration, notices, and metadata only.
- The current core gate and all four static source gates pass. The clean native
  `/WX` build passes with current-source `main.dll` SHA-256
  `D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
  That DLL is sealed in the authoritative `dist/final-2.1.0` package set, which
  passed Setup `20/20`, manual `2/2`, payload equivalence, direct-copy layout,
  clean-target, and archive re-extraction. The earlier `4AFE...` and `BDE21...`
  package sets are historical and non-authoritative. A local 2.1.0 diagnostic
  installation produced runtime evidence and the owner accepted current
  gameplay. The exact installed DLL-to-`D4EE...` hash receipt is not recorded.
  Detailed scenario evidence and publication rights remain independent.
- No custom executable, PowerShell host, watcher, C# Overlay, or Lua script is
  part of the installed native mod.
- F6 opens the native visibility Hub. F7 activates. F8 disables, and F8
  followed by F7 explicitly resynchronizes.
  F6 also owns one persisted `AVAILABLE` / `ALL` area-quest display mode.
  `AVAILABLE` preserves prerequisite-proven filtering; `ALL` removes only that
  display filter and still excludes save- or runtime-confirmed completions.
  A separate persisted Assault `AVAILABLE` / `ALL` display mode defaults to
  `AVAILABLE`. `ALL` is a presentation-only static catalog view of all 40
  Assault records and bypasses save readiness, time, defeat, and cooldown only
  for drawing. `AVAILABLE` retains every live gate; actual completion/cooldown
  authority, Boss selection, and area-quest logic are unchanged. Both modes reuse the existing fixed
  selection passes and open-only Hub service, with no added timer, SQL request,
  object scan, or steady-state work.
  F7 re-reads bounded runtime and save-backed state; it does not run Setup,
  replace files, extract PAKs, or regenerate immutable catalogs.
  Non-open-world activities suppress both renderers; open-world interiors
  remain eligible. For task-class mapping, an already-active explicit F7 may
  rearm a failed capture, but that mapping-only action preserves
  exact-completion bits and revision/reactivation fences.
  The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is not
  ordinary activity suppression. It is the save-owner hard boundary: the
  runtime disables, detaches both renderers, clears weak candidates and every
  mutable save-derived/process-local delta, and latches activation off. A later
  save load remains disabled until the player reaches an open world and presses
  F7 explicitly; F7 on the title screen or during an incomplete load is
  rejected.
- The compact host samples viewport size and DPI on the existing one-hertz
  minimap-scale service. Only a real geometry edge updates its top-left origin
  and render scale, so window/fullscreen changes move the radar and clock
  together without touching the 16 ms motion path. Deterministic tests cover
  numeric viewport/DPI inputs including 3440x1440, 3840x1600, 2560x1080,
  ordinary windowed sizes, and DPI changes. They do not prove a 3840x2160
  viewport with an internal 21:9 content rect, native 21:9, windowed client
  geometry, or the game's runtime Slate layout. Expanded-map
  attachment
  samples the live `PlayerIconWidget` alignment pivot through its current Slate
  cached geometry and `LocalToAbsolute`, then through the selected native icon
  Canvas's current cached geometry and `AbsoluteToLocal`. `WorldMapUISize` is
  authored metadata, not the parent extent. Initial attachment has a separate
  bounded three-attempt readiness service and retains only numeric observations.
  Missing, implausible, or unstable geometry fails closed; no centered fallback,
  desktop-resolution substitution, or new polling schedule is published.
- Attach, `SetWorldMapImage`, F7 resume, and exact zoom events arm a finite
  five-deadline settle tail at 100, 250, 500, 1,000, and 1,250 ms. Each due
  game-thread pass takes one fresh parent-local observation; overdue deadlines
  remain due and advance one observation per later pass. The first four passes
  are read-only. The final pass may mutate the
  tree only after exact retained-parent witness and two-sample stability within
  0.5 logical units. Equal parent extents allow the same two retained hosts to
  translate/reparent by the live anchor delta. A stable extent change may
  consume at most one candidate-bound full attach so glyph size is rebuilt in
  the new coordinate basis. No wheel event replenishes that token, and there is
  no marker recollection, atlas rasterization, file access, texture import,
  widget construction, or steady poll on the retained-host path.
- The compact pool owns two independent fixed six-piece height channels. The
  nearest treasure uses its selected treasure-category fill color; the nearest
  visible area quest uses the official cyan accent. Both use the same dark
  outline and may display simultaneously with no per-motion allocation or
  UObject read. The numeric clock uses configured presentation bands beginning
  at 06:00, 12:00, 18:00, and 21:00. These are presentation policy, not proven
  game-native phase semantics; weather remains unavailable and unqueried.
- Final 2.1.0 adds compact-only bird eggs for exact `Bird_Egg01_C` and
  `Bird_Egg02_C` actors. The creation listener publishes only weak identities
  into a fixed 512-slot pool. The existing 250 ms service performs at most
  eight bounded candidate/position queries per control tick. A candidate is
  available only when its Actor-owned `DInteractableComponent` reports both
  `InteractableValue=2` and `InteractTypeValue=2`. Unknown reflection reads stay
  pending for the same bounded service instead of becoming a false negative.
  Both values are one-byte scoped enums and are read only through their
  validated integer underlying properties;
  exact Bird Egg EndPlay retires the matching weak identity immediately. A
  fixed nearest-16 set receives bounded presence probes on the existing 250 ms
  discovery edge while the independent F6 RADAR category is enabled. Its four preallocated pieces use a
  native rounded-box brush and narrow geometry to render a vertical oval; no
  widget or schedule is added.
  The MAP cell is unavailable. There
  is no new polling, UObject enumeration, SQL, retained raw Actor, dynamic
  queue, filesystem polling, or expanded-map work.
- Dev56 adds one native dynamic-quest event hook, exact live-`PROGRESS`
  validation, per-task ten-second numeric completion witnesses with one
  non-replenishing follow-up budget, first-valid-hour task refresh, unique
  positional linking for `MONSTER_ALIVE value1=0`, bounded non-replenishing UMG
  soft-attach retries, and a transient native F6 category Hub. Generic
  interaction never arms task completion, and the closed Hub performs no
  UObject work. Pure numeric task witnesses retain their original ten-second
  deadline across travel, while F8/F7 clears them. Hub cleanup restores input
  through the panel's owning Controller.
- Dev57 makes Hub selections auto-apply only on a real packed-mask change,
  replaces Apply/Cancel with a top-right `X`, compacts the visual hierarchy,
  orders Game-and-UI mode before the cursor write, and conditionally restores
  the cursor only when the game hides it during the existing open-only 50 ms
  service. The closed path remains free of Controller/UObject/file work.
- Dev58 places both expanded-map radar atlas hosts above game-native icons with
  adjacent deterministic radar-internal Z orders. It also collapses the compact
  host on the first failed current-Pawn sample while reading cursor state from
  the same current Controller before Pawn resolution. It adds no polling,
  retry, enumeration, or UObject retention.
- Dev59 changes only the F6 Hub presentation: framed panel, column header chips,
  content surface, alternating rows, toggle contrast, and close-control styling.
  The existing aspect-ratio-aware viewport scale, live UMG DPI conversion,
  centered placement, auto-apply logic, and open-only 50 ms service remain.
- Dev60 fixes the observed high-resolution TextBlock/panel DPI mismatch by
  applying the same open-time unit scale to every text render transform from a
  top-left pivot. It also separates title and column headers and darkens the
  linear-space palette. Both new reflected functions are ABI-gated and run only
  during explicit F6 construction.
- Dev61 shortens Hub column headings to RADAR/MAP and layers explicit
  role-specific text multipliers over the shared viewport/DPI scale. It changes
  no masks, input behavior, persistence, renderer path, or service cadence.
- Dev62 changes confirmed non-open-world compact lifecycle from collapse to
  guarded detach and rearms one fresh compact attachment on the open-world
  return edge. Its fault recovery is limited to a prior runtime-only compact
  renderer fault after clean detach; same-call faults and ABI failures remain
  terminal.
- Dev63 adds one process-lifetime automatic engine-tick recovery budget. An SEH
  fault disables immediately; the next tick performs guarded cleanup and may
  reactivate only when the radar was previously active. A second fault, an
  inactive fault, or failed recovery stops with explicit diagnostics and never
  retries steadily.
- Dev63 also permits explicit F7 recovery of a prior runtime-only world-map
  renderer fault and a faulted area-quest scanner, and explicit F6 recovery
  of a prior runtime-only Hub fault. Every path requires clean detach, no new
  same-call fault, and valid reflected ABI.
- Dev64 keeps the process-session stream and 16-line ordinary batching, moves
  compact/world-map state changes out of synchronous game-thread flushes, and
  reserves immediate flushes for critical lifecycle/fault events. The 1 MiB
  current file plus one previous-file rollover remains bounded. The 2.1.0
  corrected release flow builds one exact unsigned ExperimentalNested Setup
  artifact from the source-bound payload; the four-file installer archive and
  any source archive remain separate publication decisions.
- Dev63 release verification binds every installed file to both the package
  manifest and its allowlisted source hash. The native build validates exact
  FetchContent origins/commits, accepts only the pinned UE4SS `fmt` patch, and
  requires `IconFontCppHeaders` commit `210b5a3` through a source override so
  the upstream floating branch cannot enter a release build.
- Dev64 closes the dungeon-return timing hole with the existing UObject-create
  listener. An exact `DLayerMiniMap` is copied into a fixed weak mailbox and can
  rearm one bounded compact attachment when the newly created layer becomes
  available. A distinct replacement identity can rearm once even if the old
  renderer still reports attached or menu-suppressed; duplicate identity
  delivery cannot create a retry loop. Travel clears the active renderer
  candidate but preserves the one weak mailbox only until transition end can
  validate it against the exact new GameMode world.
- Dev64 removes every production Boss/Assault `FindAllOf` path. The listener
  keeps one weak identity per immutable encounter catalog entry in a fixed
  49-slot array. The 250 ms discovery service considers only catalog positions
  inside 100 metres, while strict exact-class `Destroyed` EndPlay recovery is
  available to both Boss and Assault. No dynamic queue, catalog enumeration, or
  16 ms encounter work is added.
- Dev64 contains all SQLCipher row callbacks behind exception boundaries and
  fixed row ceilings. Any malformed, excessive, or allocation-failing result
  aborts only that one F7 snapshot fail closed. Native logging also disables
  itself after a failed stream open/write/flush instead of repeatedly operating
  on a failed stream.
- Dev65 separates encounter disappearance from treasure disappearance. Boss and
  Assault observation stores the actual live actor position, arms only after at
  least four present samples spanning one second, and accepts a non-exact
  completion only after forty missing 250 ms samples spanning ten seconds while
  the current player remains within 100 metres in a stable open-world,
  cursor-hidden context. An already observed `RemovedFromWorld` Actor is
  released immediately but its numeric observation may continue through that
  same ten-second gate; the EndPlay reason itself is never completion evidence.
  Travel, activity suppression, menus, leaving range, and reappearance cannot
  complete an encounter. Exact
  current `Destroyed` remains the fast path. Range eviction also clears the
  processed weak identity so the same still-live actor can bind again on return.
  Candidate consumption and the final completion write both recheck current
  availability on this conservative disappearance route; any rejected
  completion race clears the identity as well. This recheck does not apply to
  an exact death event already accepted into the R7 handoff.
  Confirmed activity entry clears encounter observations and processed
  identities at the suppression edge without depending on a valid Pawn sample.
- R4 makes the optional
  `/Script/DS.DsFieldCharacter:NetMulticastNotifyDeath` pre-hook the primary
  encounter-completion signal. The receiver must be the exact weak Actor already
  observed for the current activation and epoch, exactly match its immutable
  catalog class, be a `DsMonsterCharacter`, remain currently available and
  visibly seen, and be within 100 metres of the current player. The callback
  only sets its catalog bit in a fixed 49-bit atomic mask. In R7, ordinary
  consumption on the existing 250 ms control service requires a valid context
  but does not recheck the event's already-proved time window. Successful bits
  clear individually after the 120-minute cooldown and render-state application;
  an apply exception retains the affected bit. F7, disable, travel, and
  activity-suppression boundaries may settle pending bits numerically without
  renderer mutation before reset. Only process shutdown and UObject-array
  shutdown hard-clear the mask. This introduces no scan, SQL query, dynamic
  queue, recurring timer, or continuous polling path. Hook lookup or
  registration failure is explicitly optional and cannot make the radar's
  required-runtime latch fail.
- R4 preserves the full dev65 forty-sample/ten-second disappearance fallback.
  A last trusted in-range Actor position is not replaced by a later far pooled
  position, so pool relocation cannot poison fallback proximity evidence.
  Reappearance, range exit, suppression, travel, and unavailable/cooling state
  still reject completion. The R3 moribund approach is not authoritative: the
  target `MonsterCharacterData` records use `UseMoribund=0`, and existing
  diagnostics recorded no moribund completion hit.
- R4 gameplay proved exact Boss and Assault kill completion without F8/F7.
  Maximum-zoom close/reopen nevertheless proved that a game-native child can be
  inserted later at the same `INT_MAX` Z after R4's successful one-shot
  restack, leaving radar content underneath it.
- Historical R5 changed only that expanded-map lifecycle edge. The exact
  `/Script/DSClient.DPanelWorldMap:OnSliderValueChanged` event joins attach,
  `SetWorldMapImage`, and F7 resume as a bounded layering trigger. Each trigger
  started or restarted exactly four settles at 100, 250, 500, and 1,000 ms. Every
  settle re-resolves the current native icon Canvas, then removes and reinserts
  the background and foreground radar hosts in that order at maximum Z. R5
  changes no coordinates, projection, atlas content, marker selection, marker
  geometry, or state-decision logic and added no steady poll. Exact-artifact R5
  acceptance was not completed before the next repair.
- R6 addresses the distinct minimize/restore replacement-layer failure proved
  by the installed R5 log. R5 attached 328 markers to `DLayerMap` serial 3;
  after restore, serial 4 appeared with `map_id` temporarily unavailable while
  a zoom settle still targeted the old retained renderer, producing failure
  103, state 5, detach, and no recovery. R6 requires exact full weak layer
  ownership before zoom or same-layer `SetWorldMapImage` restack work. A layer
  mismatch returns `RetryLater` before same-layer payload validation. The exact
  replacement layer's `SetWorldMapImage` event may consume the existing one-shot
  rearm when the renderer is `Ready` or is still `Attached` to a different old
  layer, retaining the consumed bit and existing bounded three-attempt service.
  A raw incomplete or spurious create-listener candidate cannot detach the old
  renderer. R6 adds no focus hook, poll, timer, enumeration, steady work,
  coordinate, projection, marker, or encounter-state change. Exact-artifact R6
  gameplay and performance acceptance remain `NOT_VALIDATED`.
- R7 preserves the R6 renderer lifecycle and schedules while repairing the
  exact encounter-death handoff described above. It also excludes only
  confirmed nonexistent treasure save ID `11230106` at
  `(182813, 162051, 3150)`. The 1,693-entry render catalog and 1,692-entry actor
  catalog have that ID as their sole set difference; a fail-closed gate binds
  the exception to the exact record and rejects future catalog drift. R7 also
  hardens release and installer boundaries. The handoff repair reuses the
  existing 250 ms service and fixed 49-bit mask and adds no poll, timer, scan,
  SQL, queue, renderer schedule, or steady work. At the historical R7
  checkpoint, source, native-build, installer-matrix, and package gates passed.
  Those older artifact results are not current D4EE evidence; exact-artifact
  gameplay and performance remain `NOT_VALIDATED`, and publication remains
  `BLOCKED`.
- R8 preserves the R7 renderer and lifecycle schedules. It adds the exact
  native `DsFieldCharacter.NetMulticastSetDeathProcess` receiver as a second
  Boss/Assault completion route, accepting only reflected process state `End`
  after the same exact observed weak identity, catalog class,
  `DsMonsterCharacter`, activation/epoch, availability, visibility, and
  100-metre gates. An exact dynamic catalog quest event arms its ten-second
  exact-ID witness without a prior `PROGRESS` requirement, but never completes
  the task by itself. Exact `END` completes the witnessed ID. An unresolved
  expiry may queue its fixed bit on the single below-normal save worker for one
  immediate exact-ID confirmation and at most two retries at 15-second
  intervals, but only when F7 established a known per-ID `COMPLETE_CNT`
  baseline. An absent row is zero only for a valid single-owner query;
  otherwise the baseline is unknown and no confirmation SQL is queued. Only
  strict count growth confirms completion. Three non-confirming attempts lock
  the same task generation until a new F7 or a current `NONE`/`END` followed by
  later `ACCEPTABLE`/`PROGRESS`. The bounded path adds no periodic SQL, dynamic queue,
  save-file poll, object enumeration, or motion-path work; exact-artifact
  gameplay and performance acceptance remain `NOT_VALIDATED`.
- Dev65 makes diagnostics startup-only and fail closed. Setup embeds
  `config/diagnostics.example.ini` with `debug_logging=false` as an
  immutable clean-install default but does not write the example file into the
  target; live `config/diagnostics.ini` is user state. Disabled logging returns before event
  formatting, locking, directory creation, rotation, or file I/O. The local
  gameplay-evidence deployment may explicitly enable it without changing the
  release default.
- The current diagnostics document accepts at most 2,048 bytes, uses one
  commented `[diagnostics]` section with exactly one
  `debug_logging=true|false` key like Auto Pickup, and preserves exact legacy
  one-line `event_log_enabled=true|false` files. Enabled records carry one
  schema-2 session header plus exact capture-time `seq`, `utc_ms`, and
  `elapsed_ms` fields. Enabled events use a fixed 256-record queue with
  non-blocking `try_lock`; contention or saturation drops and counts a record
  instead of waiting on gameplay. A below-normal-priority writer drains at most
  16 records per batch, flushes idle partial batches after 250 ms, performs
  critical flushes after dequeue, and owns all file I/O. Numeric Tick telemetry
  is formatted only on that writer and includes `logger_dropped` and
  `logger_truncated`. A 1 MiB current file rotates to exactly one previous file.
  This adds no game query or event cadence. Public defaults remain off.
- The current startup-only visibility document accepts at most 4 KiB and uses
  readable `[radar]`, `[map]`, and `[modes]` sections. Every category is a
  named `true|false` key; `area_quests` and `assault` modes accept
  `available|all`. Strictly valid legacy schema 1-4 packed-mask files remain
  readable for upgrades. F6 applies changes immediately and replaces the file
  atomically in current format only on a real change; there is no hot polling.
- Dev65 added a binary-package `Install.cmd` one-click installer. The 2.0.0
  public package retires that entry point in favor of one embedded C# Setup;
  `Build-Release.ps1` is the only release builder. Setup discovers
  Steam app 4570720 or accepts an exact Win64 directory, verifies the complete
  manifest and source-bound payload, performs three stopped-game checks, backs
  up the exact prior target and enablement state, normalizes the native radar
  `mods.txt` entry, removes the obsolete predecessor entry while preserving
  encoding and line endings, and preserves both
  live user configuration files independently, and fully rolls back on failure.
  It never launches, terminates, or automatically elevates the game or itself.
  An active external `DragonSwordWorldRadar : 1` entry or exact
  `DragonSwordWorldRadar/enabled.txt` path in any approved Mods root is a
  zero-mutation conflict. An exact disabled `DragonSwordWorldRadar : 0` entry is
  preserved, and Setup never deletes or disables that external mod.
- Dev66 fixes clustered and XY-overlapping treasure completion at the exact
  `Server_RunInteractV2` event. The receiver `DInteractableComponent` owner is
  resolved before optional `ExecuteTarget*` fields, an existing weak Actor
  observation is reused when available, and bounded exact-class fallback
  matching ranks candidates in 3D. Catalog-only ambiguity still fails closed.
  This path is interaction-only and adds no recurring query, enumeration,
  queue, timer, or frame work.
- Dev67 replaces that compatibility path with the exact
  `DsAnimationProp.NetMultiExecuteInteractProp` receiver Actor, accepts the
  event only when its Actor parameter equals the freshly resolved local Pawn,
  and resolves the receiver's reported `ObjectID` through exact class and 3D
  validation before weak-identity or bounded unique-class 3D fallback.
  `SetDeathProcess` remains an exact-receiver fallback. The work remains
  event-only.
- Version 2.0.0 recognizes the game's mounted underwater identity split: the
  fresh local Pawn is the water mount while the interaction Actor is that
  Pawn's exact callback-local `Rider` UObject. Only a mount-only treasure
  receiver within eight metres, exact Rider pointer equality, and a non-null
  same-World relationship can complete immediately. No Rider, Pawn, receiver,
  or other UObject is retained. Any other rejected non-Pawn event keeps the
  2.0.0 delayed positive-only fallback: it may arm only an exact nearby
  treasure ID. Fixed arrays retain its catalog index, attempt count, and due
  edge only. The existing below-normal worker checks at most 64 exact treasure
  categories after 15 seconds and once more 285 seconds later if needed; it
  skips encounter and dynamic-quest queries and never becomes an idle poll or
  full treasure scan.
- Dev67 gives exact task-actor and exact quest-event evidence a ten-second
  exact-ID verification window. One query may run every 750 ms for a witnessed
  task, with a global limit of one such query per engine tick. Final `END`
  proves the exact task complete even after a previously published `NONE`,
  `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` itself is not completion. The
  fixed 147-entry queue cannot duplicate a live task or grow. Post-completion
  `NONE` or `END` remains only the repeatable-reactivation boundary.
- Dev67 makes expanded-map runtime deltas visibility-aware. A visibly open
  exact attachment may rebuild in the same session; an attached but hidden
  layer is retired and deferred until the next exact `SetWorldMapImage` event.
  That event can rearm the three-attempt readiness budget once for the matching
  candidate serial, including an exhausted three-probe `map_id` transaction
  that never reached a renderer failure code. Duplicate same-layer delivery
  never replenishes work after that one rearm.
- Dev67 removes the static-catalog-distance prefilter from Boss/Assault weak
  candidate resolution. The 250 ms service reads the current actor position
  first, applies the 100-metre player-to-actor bound, and performs at most eight
  actor-position queries per control tick while walking the fixed 49 slots
  round-robin. No enumeration, dynamic queue, or 16 ms work is added.
- The production owner is native UMG only. It connects no PostRender/Present
  hook, external shared mapping, executable, bridge, or Lua runtime. Historical
  canary sources/configuration remain dormant, uncompiled, unread by `main.dll`,
  and absent from the binary runtime payload.
- Public binary publication is blocked independently of technical acceptance.
  The pinned `UEPseudo`/generated Unreal interface has no repository-level
  license in the audited checkout, and the bundled `e_sqlcipher.dll` lacks an
  exact reproducible source/build chain. Redistribution rights, attribution,
  and provenance for the PAK-derived catalogs and MnMRadar-derived coordinate
  table also require review. See `docs/DEPENDENCY_SOURCES.md`.

## 2.0.0 release contract

- The one-click installer targets the ExperimentalNested layout. It requires
  the exact selected DragonSword shipping-executable path
  plus a bounded executable AMD64 PE32+ application image, but does not use a
  fixed game SHA-256 allowlist. The observed game hash is only confirmed
  transaction identity and install-record provenance. Existing nested loader
  and root proxy DLLs are accepted by bounded x64 PE32+ and directory-structure
  validation, not a fixed hash allowlist. Embedded bootstrap/conversion hashes
  remain package-integrity evidence only.
- The packaged save-owner RVA remains the zero-scan runtime fast path. If a
  game update invalidates it, the existing below-normal save worker may scan
  executable PE sections once per process, requires exactly one bounded
  owner-pointer target, caches only the numeric RVA, and validates the live
  save key. No watcher, updater, timer, recurring scan, or game-thread work is
  added. Static catalog changes still require a separate data release.
- Loader absence triggers a transactional, integrity-verified bootstrap of the
  embedded ExperimentalNested runtime. Root, dual, malformed, or incomplete
  layouts enter the explicitly confirmed, fully backed-up conversion path.
  A structurally complete ExperimentalNested layout keeps its loader, proxy,
  settings, and configured Mods paths unchanged.
- The controlling `mods.txt` for the selected layout is the sole load authority.
  The public payload contains no `enabled.txt`. A recognized legacy marker is
  upgrade input to back up and remove, and the obsolete
  `DragonSwordWorldRadarObjectState` line is removed rather than retained as a
  disabled entry. An active `DragonSwordWorldRadar : 1` entry, malformed
  external-radar entry, or exact `DragonSwordWorldRadar/enabled.txt` path in any
  approved Mods root rejects Setup before backup or mutation. An exact disabled
  `DragonSwordWorldRadar : 0` entry is preserved byte-for-byte; Setup never
  deletes or disables that external renderer.
- The recommended package is installer-first and contains exactly the unsigned
  Setup executable, its SHA-256 sidecar, `INSTALL.md`, and
  `THIRD_PARTY_NOTICES.txt`. The isolated installer gate is exactly 20 passed,
  0 failed, and 0 skipped.
- Two additional manual archives target only ExperimentalNested:
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-No-UE4SS.zip` for an
  existing compatible nested runtime and
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` with the
  pinned nested runtime. Their matrix is exactly 2 passed, 0 failed, and 0
  skipped. Both are Nexus-compatible script-free packages whose roots map
  directly to `DS/Binaries/Win64`. No-UE4SS includes the Radar Mod and a clean
  one-line `ue4ss/Mods/mods.txt`; users copy that file only when the target is
  missing and otherwise merge the Radar line. With-UE4SS is clean-target only
  and includes the pinned nested runtime plus clean load control. StableRoot is
  not restored as a supported payload.
- The final directory is `dist/final-2.0.0` and contains exactly
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Installer.zip`,
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-No-UE4SS.zip`,
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`,
  `release-manifest.json`, and `SHA256SUMS.txt`.
- `Build-Release.ps1` is the supported release entry point and delegates to the
  corrected `Build-Installer.ps1`, the exact 20-case installer matrix, the
  exact two-case manual matrix, and the three-channel archive builder. The retired
  `Stage-Release.ps1` and `Install.cmd` paths create no distributable artifact.
  Setup also exposes one confirmed, token-bound `Uninstall` transaction after
  strict same-product ownership proof. It removes only the exact product tree
  and valid load-control entry, preserves UE4SS, unrelated Mods, `DS/Saved`,
  encoding, and line endings, and rolls back on failure. Setup-embedded
  `.example.ini` resources supply immutable clean-install
  defaults but are not written into the target; live `visibility.ini`,
  `diagnostics.ini`, and `data/defaults/treasure_overrides.txt` are installed or
  preserved. Recognized Update / Repair replaces the bundled DLL and generated
  catalogs, accepts older owned releases, and retains no persistent backup
  after a successful transaction.
- Existing-target replacement requires strict top-level product ownership
  consistent with the package manifest and source-bound payload hashes. Nested
  or unrelated product-name text is not ownership proof and must make zero
  mutation. The proof requires schema-5 release and schema-1 package metadata,
  2-256 unique manifest files, exact size/hash identity for immutable files including
  `metadata/release.json` and a version/label-bearing `dlls/main.dll`, and an
  exact recursive tree containing only manifest files plus the bounded live
  allowlist. Each ownership JSON is at most 4 MiB with depth 16; each owned file
  and the total owned payload are at most 256 MiB. Legacy marker/example files
  are at most 64 KiB, each log 2 MiB, each atlas cache 16 MiB, and an optional
  install record 1-64 KiB with matching version/label. Any extra path or bound
  failure rejects before backup or mutation. The treasure override is the sole
  mutable manifest path and must pass its strict 64 KiB syntax contract.
- Public diagnostics are exactly disabled. A diagnostics-enabled local test is
  an installed-copy-only override whose hash must be recorded separately; no
  installed file or log is a release staging source.
- `SOURCE_VALIDATED`, `BUILT`, `PACKAGED`, `INSTALLER_TESTED`, `DEPLOYED`, and
  `GAMEPLAY_ACCEPTED` are independent. Assigning version 2.0.0 proves none of
  them. Publication is separately `BLOCKED` until all three provenance and
  rights reviews are resolved.

## Implemented foundation

- Dev21-dev29 established compact-map logical coordinates, one preallocated
  moving Canvas root, bounded marker pools, and the nearest-treasure height
  pointer. The current 2.1.0 correction adds a second fixed height group for
  the nearest area quest, allowing both categories to indicate Z at once.
- Dev26-dev30 established the exact expanded-map native parent, map projection,
  texture-brush ABI, inherited pan/zoom/clipping, and F8 removal.
- Dev33-dev44 added the 147 dynamic-area-task catalog, dynamic query source,
  transactional refresh, save completion proof, and fail-closed Main/Group
  prerequisite capture.
- Dev34-dev39 established native treasure/encounter observation and two-hour
  Boss/Assault cooldown. Runtime evidence includes automatic Boss and Assault
  removal. Dev64 replaces the later activation catch-up enumeration with one
  fixed weak slot per encounter catalog entry.
- Dev41 restored one compact host/root after the split encounter tree faulted.
- Dev42-dev48 restored mini-games and refined both-map glyph geometry, layering,
  task styling, height pointer, and clock presentation.
- Dev49 rejected `PROGRESS -> FAIL` as completion evidence.
- Dev50 added one bounded transactional task refresh at each displayed
  game-hour edge.
- Dev52 registered the game-owned task completion event. Dev53 added a bounded
  ID-less confirmation fallback and removed treasure-class game-thread
  `FindAllOf` catch-up.
- Dev54 replaces ID-less inference with one F7/travel reflection of
  `TaskActorClassContainer.DynamicQuestTaskList`. It requires exact one-to-one
  class-full-name coverage for all 147 catalog IDs, hides the mapped ID
  immediately at `OnRecvCompleteQuest`, and retains the generic debounced scan
  only for settled state and repeatable-task reactivation. A fixed per-task
  revision fence prevents an older in-flight scan from reverting a newer exact
  completion. Completion also stays latched through later still-active samples
  until a current scan observes `NONE` or `END`; only a subsequent current scan
  may restore a repeatable task on `ACCEPTABLE` or `PROGRESS`. `FAIL` does not
  arm restoration. Mapping capture has one post-stability attempt and only one
  delayed retry after failure; success or the second failure stops work.
- The asynchronous F7 save snapshot only merges positive dynamic-task
  completion IDs. It cannot clear a newer exact or generic runtime completion;
  only activation lifecycle reset owns full completion-state clearing. A result
  arriving after expanded-map attachment rebuilds once when the exact layer is
  visibly open, or retires a hidden retained atlas until its next exact
  `SetWorldMapImage` edge, rather than preserving pre-reconcile pixels.
- `InitGameState` pre-transition drains any already published exact-completion
  bits into activation-local numeric completion latches and completed quest IDs
  before task-class mapping/bits are cleared. This closes the same-frame
  completion/travel race without accessing or retaining a task actor or any
  other UObject. F8 followed by F7 remains the fresh resynchronization boundary.
- The bounded 49-entry Boss/Assault cooldown map is process-local and is not
  cleared by F8/F7. Save timestamps merge by maximum, so a stale one-shot
  snapshot cannot revive a just-defeated encounter. The existing 250 ms control
  service performs one rate-limited 1 Hz scalar edge check; only a cooldown
  expiry or displayed world-hour change recomputes the fixed 49-bit visibility
  mask and invalidates an atlas when visibility really changes.
- The R7 exact-death handoff owns a second fixed 49-bit pending mask. Ordinary
  consumption requires a valid runtime context and does not recheck an accepted
  event's time window. Applied bits clear independently; an apply exception
  retains its bit. F7, disable, travel, and activity-suppression boundaries may
  settle pending numeric state without renderer mutation. Only process shutdown
  and UObject-array shutdown hard-clear the mask. This reuses the existing
  250 ms service and adds no new scheduling or steady work.

## Current data contracts

- Treasures: 1,693 unique render records and 1,692 unique actor records;
  confirmed nonexistent save ID `11230106` is their sole set difference and
  the only default exclusion. There are 1,506 map-100 render records before
  that exact eligibility filter.
- Encounters: 9 Boss and 40 Assault records.
- Mini-games: 33 Fly, 40 Mole, and 10 Wave records.
- Area quests: 147 records.
- Expanded-map fixed capacity: 1,785 markers.
- Compact marker capacity: 80, with the single nearest chest enlarged and two
  fixed height channels assigned independently to the nearest chest and the
  nearest visible area quest.

## Safety invariants

- Never retain Pawn, Controller, Canvas, Actor, task actor, table, or reflected
  array-element UObjects across frames or worlds.
- Use current game-thread Controller/Pawn reads and scalar/fixed-capacity
  snapshots; retained runtime identities must be weak.
- Never erase accepted encounter-death bits merely because ordinary context is
  temporarily invalid. Clear only a successfully applied bit; retain an
  exception bit; use numeric-only authoritative boundary settlement; reserve a
  whole-mask clear for process or UObject-array shutdown.
- Travel, F8, shutdown, and activity suppression must invalidate or detach state
  fail closed. The UE4SS module-unload callback unregisters hooks/listeners;
  this is runtime cleanup, not a user-facing uninstall tool. Once UObject-array
  shutdown starts, the create listener is removed and pinned callbacks become
  inert instead of calling unsafe late unregistration APIs.
- No recurring global UObject enumeration, periodic SQL worker, accumulating
  queue, or unbounded/steady retry loop.
- Area-task ambiguity must remain hidden or unconfirmed. `FAIL` alone is never
  completion evidence. Missing, ambiguous, duplicate, or unmapped task-class
  identity must never hide a guessed catalog ID.
- Static/build/deployment success is not gameplay acceptance.

## Verified evidence boundary

The latest historical runtime evidence available to this release audit includes
the R4 encounter and expanded-map layering sessions. Earlier dev60 evidence reproduced the dungeon-return compact failure
(`state=5` after the old short attachment window) and 22-32 ms Boss/Assault
class-enumeration spikes. Dev64 removes both historical mechanisms
structurally. Dev65 additionally
closes the runtime-proven encounter false-positive where streaming disappearance
after leaving a live large Assault was accepted as defeat. Dev66 addressed the
separate runtime-proven clustered-treasure event loss. The latest dev66 logs
then exposed the exact task-completion, treasure receiver, first-open world-map,
and moving-encounter candidate defects repaired in dev67. The R3 log then showed
Boss and Assault observations ending as departure eviction, with no direct
completion evidence; F8/F7 worked only through one-shot save reconciliation.
R4 subsequently proved one real Boss and one real Assault completion through
the exact death-notification route without F8/F7. The same R4 gameplay also
proved that a successful one-shot world-map restack is insufficient at maximum
zoom after close/reopen because a native child can be inserted later at the
same Z. R5 addresses only that late insertion with the exact zoom event and
bounded four-settle tail. The installed R5 log then attached 328 markers to
layer serial 3 before minimize/restore published replacement layer serial 4
with `map_id` temporarily unavailable. A settle wrongly refreshed the old
renderer, leading to failure 103, state 5, detach, and no recovery. R6 confines
same-layer work to exact weak ownership and waits for the replacement layer's
exact map-image readiness event. No R6 gameplay session has yet proved the
current correction set. R7 preserves that renderer correction and adds the
fixed 49-bit exact-death handoff repair, exact ghost-treasure eligibility, and
release/installer hardening. Its gameplay and performance matrix is independently
`NOT_VALIDATED`; publication remains `BLOCKED`. Earlier dev47 task scans were approximately 0.6-1.0 ms
total across 147 frames. Keep those historical measurements separate from the
current acceptance claim.

Expanded-map attach has previously measured about 94-97 ms when building and
importing atlases and about 27-37 ms on a process-local atlas-file cache hit.
That work is bounded to explicit attachment transactions but remains a known
hitch source. Do not describe it as accepted steady-state performance.

## Build and deployment boundary

Current automated evidence is complete through the source/static gates, clean
native `/WX` build, Setup `20/20`, manual-copy `2/2`, payload equivalence, and
three-archive re-extraction. The packaged `main.dll` SHA-256 is
`D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
It is sealed in the authoritative `dist/final-2.1.0` output. Existing
`4AFE...` and `BDE21...` package sets are historical and non-authoritative. No
deployment was performed. Gameplay and performance remain independently
unvalidated.

Run core tests, all four static gates, and the pinned clean native build before
deployment. `Build-Native.ps1` validates top-level and nested dependency
identity and emits a source-bound receipt. `Deploy-NativePrototype.ps1` must
refuse a running game, stale DLL or receipt, or source/metadata/DLL version
mismatch; it creates a rollback backup, atomically normalizes only the native
entry and removes valid `DragonSwordWorldRadarObjectState` predecessor entries
from `mods.txt` without normalizing unrelated content, and verifies the
exact installed payload. Do not launch or terminate the game from the deployment
workflow. The script checks before deployment preparation, before backup
creation, and once more immediately before the short mutation transaction.
It intentionally performs no later process check after mutation begins,
because a late failure would otherwise trigger rollback writes while the game
is already running.

Before a public release, build the ExperimentalNested payload and unsigned
single-file Setup only after the native DLL and dependency receipt are current.
`Build-Release.ps1` is the only complete release builder; `Build-Installer.ps1`
is its Setup-only internal stage. Release staging must
begin empty, re-extract and audit the installer archive, and admit exactly four
files: Setup, Setup SHA-256, `INSTALL.md`, and
`THIRD_PARTY_NOTICES.txt`. The runtime contains no `enabled.txt`; Setup migrates
a recognized legacy marker, removes the obsolete
predecessor line, and leaves one authoritative native `mods.txt` entry. It
zero-mutation rejects an active `DragonSwordWorldRadar` external renderer,
preserves its exact disabled `: 0` entry, and never deletes or disables it.
Existing-target replacement also requires the exact bounded metadata,
manifest, hash, recursive-tree, and live-file size contract recorded in the
release and installation documents before backup or mutation. The
release gate must require exactly 20
isolated installer tests passed, 0 failed, and 0 skipped. Packaging and
installer testing remain static/filesystem evidence, not gameplay acceptance.
Legacy `Stage-Release.ps1`, `Install.cmd`, and
`installer/Install-DragonSwordNativeWorldRadar.ps1` packaging paths are
retired.

The source archive is only the allowlisted project source; it excludes all SDK,
RE-UE4SS, `UEPseudo`, FetchContent, Rust-crate, and toolchain sources. Do not call
it a complete dependency-source or Corresponding Source bundle. Technical gates
do not authorize public publication: `UEPseudo`/Unreal redistribution and
GPL-compatibility review plus exact `e_sqlcipher.dll` source/build provenance
and derived-catalog/coordinate redistribution provenance remain unresolved
release blockers.

`Build-Native.ps1` requires a clean `IconFontCppHeaders` checkout at
`210b5a399a64270674560d633638952d1e8d804d` via
`-IconFontCppHeadersRoot`. Every supplied or reused checkout is validated by
canonical origin, exact commit, allowed worktree state, and parent gitlink where
applicable before disconnected reuse and again after a graph-owned clean build.
`metadata/native-build-lock.json` declares the dependency graph and
`dist/work/build/native/native-build-receipt.json` binds the output DLL to the current
compiled inputs and build tools.

## Next runtime gate

Require `START version=2.1.0` and runtime label
`DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0`, provider/hook
readiness, a successful 147-item F7 snapshot, and no renderer or top-level
fault. The activation must also report `AREA_QUEST_TASK_CLASS_MAP ready=true
bindings=147 ambiguities=0` at `attempt=1/2` or `attempt=2/2`, with no third or
  steady retry. The diagnostic test deployment must report
`runtime_diagnostics=startup_config_once config_debug_logging=true log_schema=2`
in the
installed gameplay-test copy; a separate restart with the release default must
prove disabled logging produces no new
formatting-visible output, file creation, append, or rotation. Complete a short
currently visible area task without F8/F7 and require:

- `AREA_QUEST_EVENT_HOOK_READY` for the one native dynamic-event hook;
- the cooking/delivery task to emit
  `AREA_QUEST_EVENT_TRIGGER progress_precondition=not_required`, arm
  `AREA_QUEST_COMPLETION_WITNESS_ARMED window_seconds=10 probe_ms=750
  schedule=exact_id_only`, and hide without F8/F7 through either exact
  `AREA_QUEST_COMPLETION_VERIFIED evidence=exact_task_id_end_state` or a
  baseline-relative exact-ID save confirmation; the fallback requires a known
  valid single-owner F7 `COMPLETE_CNT` baseline and strict count growth. A
  missing ID is zero only for that valid query; an unknown baseline queues no
  SQL. It permits one immediate attempt and at most two 15-second retries, with
  no fourth request or periodic SQL. Three non-confirming attempts lock the
  task generation until new F7 or settled repeatable reactivation. Final
  `END` is completion even after stale `NONE`, `FAIL`, `ACCEPTABLE`, or
  `PROGRESS`, but the event or `NONE` alone is not completion, and all
  simultaneous witnesses share the one-exact-query-per-engine-tick limit;
- task 1110080 and its unique linked Assault to become globally eligible at the
  applicable displayed-hour edge without local trigger discovery;
- F6 immediate selection changes, compact/world category independence, config
  persistence, direct cursor availability, `X`/F6 close, and travel cleanup
  with no cursor or panel ghost;
- the independent compact-only `BIRD EGGS` category for both exact egg classes,
  with no MAP toggle, no expanded marker, availability only at exact owned-
  component values `InteractableValue=2` and `InteractTypeValue=2`, unknown
  reads remaining retryable, exact EndPlay removing one identity immediately,
  at most eight unresolved candidate/position reads per 250 ms control tick, a
  nearest-16 bound, and no new polling, enumeration, or SQL;
- one exact `TitleMap` hard stop followed by a different save load: no stale
  mutable runtime state, no automatic reactivation, no accepted pre-open-world
  F7, and one fresh explicit F7 only after the loaded open world is ready;
- `AREA_QUEST_EXACT_COMPLETION_OBSERVED` with the expected catalog `id`,
  `evidence=task_class_map`, and `action=immediate_numeric_completion`;
- immediate compact removal and absence on the next expanded-map session;
- no revival when an older in-flight scan finishes after the exact completion;
- no revival from any later still-active sample before a current `NONE` or
  `END` inactive boundary;
- one later `AREA_QUEST_STATE_SCAN_REQUESTED
  reason=quest_state_event_debounced` transaction;
- no `AREA_QUEST_COMPLETION_UNMAPPED` for the mapped task;
- for a genuinely repeatable task, one current scan that observes `NONE` or
  `END`, followed by a later current scan that reports `ACCEPTABLE` or
  `PROGRESS`; `FAIL` must not arm reactivation;
- an already-active F7 mapping retry that does not clear exact completion bits,
  revisions, or the reactivation fence;
- an older in-flight F7 save result that cannot erase a newer native task
  completion or its dynamic-prerequisite evidence;
- an exact completion followed immediately by travel before the next normal
  engine sample; post-travel state must preserve the completion and prerequisite
  evidence through the numeric-only pre-transition drain and log
  `action=travel_boundary_numeric_preserve`;
- F8 followed by F7 must still discard the prior activation latch and perform a
  fresh resynchronization;
- a clean session with no `ENGINE_TICK_FAULT`, renderer `Faulted` state, or Hub
  fault. A controlled engine-tick fault may produce at most one
  `ENGINE_TICK_RECOVERED` in the process; a later fault must log
  `ENGINE_TICK_RECOVERY_STOPPED` and remain disabled without retrying;
- explicit F7 recovery only for a prior runtime-only world-map fault after clean
  detach, and explicit F6 recovery only for the equivalent Hub state. Same-call
  faults and ABI failures must remain terminal;
- a bounded native log: current file no larger than 1 MiB, at most one previous
  file, no per-frame event stream, and immediate persistence of critical
  lifecycle/fault lines.

For encounter acceptance, require the `READY` contract to report the fixed
49-slot discovery route with no enumeration, an eight-position-query limit
per 250 ms control tick, and `ENCOUNTER_DEATH_HOOK_READY`. Test the fail-open
capability boundary separately: if lookup or registration is deliberately made
unavailable, require a bounded disabled reason while the rest of the radar
still reaches ready state, but do not count that session as exact-artifact 2.1.0
encounter regression coverage. The R4 gameplay pass is historical evidence and
does not accept the final 2.1.0 bytes. Approach the same large Assault for more than one
second, then leave the 100-metre range several times without defeating it. Open
the expanded map or another cursor-visible menu nearby as a separate negative
case. None may create `ENCOUNTER_DEFEATED` or a cooldown, and returning must
rebind and show the still-live actor without F8/F7. Then actually defeat one
Assault and one Boss while staying nearby, without pressing F8/F7. Require one
`ENCOUNTER_DEFEATED_NATIVE
evidence=exact_observed_net_multicast_notify_death` and one
`ENCOUNTER_RUNTIME_STATE_APPLIED` for each defeat. Separately prove the fallback
cannot complete before at least forty missing 250 ms samples spanning ten
seconds while all context gates remain valid. An observed
  `RemovedFromWorld` must release its weak UObject and may complete only after
  that full numeric gate. Travel, activity suppression, leaving range, and
  reappearance must reset or reject the candidate.

For the R7 handoff specifically, accept a real exact event at the end of a
time-conditioned window and let that window change before ordinary consumption;
the already accepted bit must still apply. Invalid ordinary context must defer,
not erase, a bit. With multiple bits pending, each successful application must
clear only itself, and an injected apply exception must retain the failing and
all still-pending bits. Exercise F7, disable, travel, and activity suppression with pending bits:
each boundary may settle numeric cooldown/eligibility before reset but must not
mutate the renderer. Whole-mask clearing is permitted only at process or
UObject-array shutdown. Require no new poll, timer, scan, SQL, queue, or steady
work.

Also verify that treasure interactions and Boss/Assault discovery produce no
`NEARBY_CLASS_CATCHUP` and no production `FindAllOf` timing event. Record the
bounded world-map-layer catch-up, clock baseline, task-definition capture, and
expanded-map `attach_total_us` separately. Test F8/F7, travel, map reopen, task completion,
treasure opening, Boss/Assault defeat, mini-game visibility, expanded-map
layering across zoom levels, immediate compact scene-handoff suppression, and
two complete dungeon enter/return cycles without F7/F8. Each return must log a
distinct `COMPACT_LAYER_CAPTURE source=create_listener` serial, a fresh
`COMPACT_GEOMETRY`, and `COMPACT_POOL_STATE state=2` without another fault.
`attach_rearmed=true` is required when the event is consumed outside transition
and activity suppression; `false` is valid only when transition return already
performed the bounded rearm. At maximum zoom, repeatedly close/reopen the map
  and change zoom. Require only the 100/250/500/1,000/1,250 ms five-deadline tail
  for the latest event. Every due game-thread pass must take one fresh numeric
  observation and overdue deadlines must not collapse into multiple
  observations in one pass. The first four passes are read-only; only the final
  pass may mutate after exact retained-parent witness
  and stable parent-local geometry. Equal extents may rebase retained hosts and
  one stable extent change may consume one bounded full attach for the current
  baseline; only a successful fresh attach establishes the next baseline.
  Require radar ordering above late native same-Z children after the final pass
  and no continuing restack or steady poll. The latest historical evidence is
  the R4 Boss/Assault
pass, the R4 maximum-zoom layering failure, and the R5 minimize/restore
replacement-layer failure. With the map visibly attached, minimize and restore
the game. A new incomplete layer must not fault or detach the old renderer;
its exact `SetWorldMapImage` edge may consume at most one existing rearm and
must attach through the bounded three-attempt readiness service without F8/F7
or another map close/reopen. Require no focus hook, focus poll, recurring timer,
or continuing work. Final 2.1.0 runtime acceptance remains `NOT_VALIDATED` until this
exact-artifact session is captured.

For clustered-treasure acceptance, open the known 2.80-metre pair and the
1.815-metre same-level pair in immediate succession, then exercise one
XY-overlapping different-height set. Every physical chest must produce its own
new `TREASURE_OPENED_NATIVE` ID with
`identity=exact_treasure_actor_receiver` and an expected receiver identity
source, then disappear from compact selection on the next normal refresh
without F8/F7. Non-local interaction must be rejected and no unopened neighbor
may be hidden. The interaction
session must contain no treasure enumeration, SQL retry, recurring correlation,
or growing pending state.

After a Boss or Assault defeat, first require
`ENCOUNTER_RUNTIME_STATE_APPLIED`, then use F8 followed by F7 before the save
write and require the process-local cooldown to remain in force after the
one-shot save result.
Also cross one natural cooldown expiry and one time-conditioned Assault hour
edge. A real change must emit `RUNTIME_VISIBILITY_EDGE`, refresh compact state,
and either rebuild one visibly open exact attachment or retire a hidden retained
atlas until its next exact `SetWorldMapImage` edge, without adding 49-entry work
to the 16 ms motion path.

For expanded-map delta acceptance, first attach map 100, return to gameplay,
then open a treasure or finish a linked mini-game and reopen the same live map
layer. The next real session must omit the changed marker instead of reusing a
stale attached atlas. A hidden retained layer must defer to the next exact
`SetWorldMapImage` event; a visibly open exact layer may rebuild in the current
session. A matching serial may rearm the bounded readiness budget only once,
and repeating the same event must not dirty compact selection, replenish the
budget, or trigger another atlas invalidation/rebuild.
