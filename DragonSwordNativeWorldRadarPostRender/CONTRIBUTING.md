# Contributing

- Keep repository source, identifiers, tests, documentation, metadata, and
  commit messages in English.
- Preserve the native-only package boundary. Do not reintroduce the external
  executable, C# Overlay, watcher, Lua control chain, motion files, or a second
  live-state IPC path.
- Do not reconnect the retired shared-memory, PostRender, or late-Present
  diagnostics to the production target or binary runtime package. Historical
  canary files remain source-only audit records.
- Preserve F7 activation, F8 disable, explicit F8-then-F7 resynchronization,
  fail-closed travel, and non-open-world activity suppression. Never retain
  Pawn, Controller, Canvas, Actor, task actor, task class, data table, or
  reflected array-element UObject across callbacks or worlds.
- Preserve bounded recovery semantics. Engine-tick SEH recovery may consume at
  most one automatic attempt for the entire process and must perform UObject
  cleanup outside the exception handler. World-map and Hub runtime-only faults
  may recover only on a later explicit F7/F6 after clean detach and valid ABI.
  Same-call faults, ABI failures, second engine faults, and inactive engine
  faults remain terminal; never add a watchdog or replenishing retry budget.
- Keep the compact 16 ms path allocation-free and root-only. New work must not
  add child-widget mutation, file access, logging, UObject lookup, SQL, or
  enumeration to that path.
- Do not add broad or recurring UObject enumeration. Treasure completion must
  remain interaction/lifecycle driven. Encounter discovery must preserve the
  fixed 49-slot weak creation cache, 100-metre bound, and no-`FindAllOf` policy.
- Preserve the 147-entry task-class identity contract: one F7/travel capture
  from `TaskActorClassContainer.DynamicQuestTaskList`, exact class full names,
  one-to-one catalog coverage, no retained UObjects, and fail-closed missing,
  ambiguous, duplicate, or unmapped identities.
- Exact mapped completion must hide only its catalog ID immediately. Keep the
  generic debounced transactional refresh so repeatable tasks can reactivate;
  `FAIL` alone is never completion evidence.
- Do not claim a native `debug_logging`, `diagnostic_verbose`,
  `high_resolution_timer`, or expanded-map polling setting unless code and
  configuration actually implement it. This module does not call
  `timeBeginPeriod`.
- Add aggregate diagnostics for bounded work; never add per-frame log lines.
  Route native events through the persistent logger. Preserve the 1 MiB current
  file plus one previous rollover, at-most-16-line ordinary batching, and
  immediate critical-event flush unless runtime evidence justifies a reviewed
  successor policy.
- For every candidate, synchronize the source version, START literal, README,
  project context, changelog, release metadata, and static gates. Reject a stale
  DLL or mismatched embedded version before deployment.
- Run core tests, all four static gates, and the pinned native build before an
  authorized deployment. Treat build, package, deployment, and gameplay
  acceptance as separate evidence.
- Supply `IconFontCppHeaders` at commit `210b5a3` explicitly. Preserve the
  exact FetchContent origin/commit checks and the single allowed deterministic
  UE4SS `fmt` patch; never accept an arbitrary dirty or partial dependency
  cache for a release build.
- Build public releases only through `Build-Release.ps1`; the retired
  `Stage-Release.ps1`, `Install.cmd`, and
  `installer/Install-DragonSwordNativeWorldRadar.ps1` paths are not
  distribution channels.
  Re-extract and verify the Setup-embedded package manifest, source-bound hashes,
  exact ten generated catalogs, and bundled licenses/notices, and reject scratch
  artifacts or a runtime data generator. Preserve live `visibility.ini` and
  `diagnostics.ini` independently as user state. Their `.example.ini` defaults
  are immutable Setup resources and must not remain as duplicate installed
  files.
- Keep every generated build, test, staging, deployment-backup, and release
  artifact under `dist`. Use `dist/work` for disposable workspace state and
  `dist/final-<version>` for the authoritative package set; do not recreate
  `build-*`, `staging`, or `runtime` directories at the project root.
- Existing-target ownership must be proven by strict top-level product metadata
  consistent with the manifest. Never accept nested product-name text, an
  unknown same-name directory, or a jointly altered manifest/payload as
  ownership proof. Preserve the exact recursive-tree contract: schema-5 release
  plus schema-1 package metadata, 2-256 unique manifest entries, exact file
  size/hash identity including release metadata and the version/label-bearing
  DLL, 4 MiB/depth-16 JSON limits, 256 MiB per-file and total-owned limits,
  64 KiB legacy/config-example limits, 2 MiB per log, 16 MiB per atlas cache,
  and a 1-64 KiB matching install record. Any extra path or bound failure must
  reject before backup or mutation.
- Treat `DragonSwordWorldRadar` as an external renderer, not an owned
  predecessor. An active `DragonSwordWorldRadar : 1`, malformed same-name
  entry, or exact `DragonSwordWorldRadar/enabled.txt` path must cause a
  zero-mutation reject. Preserve exact disabled `DragonSwordWorldRadar : 0` and
  never delete or disable the external mod. The separately owned
  `DragonSwordWorldRadarObjectState` predecessor may still be removed by the
  documented migration.
- Preserve the exact R7 ghost-treasure quarantine: render IDs 1,693, actor IDs
  1,692, sole difference `11230106`, exact coordinates
  `(182813, 162051, 3150)`, and exactly one active default ignore. Regenerated
  data must fail the gate until that exception is reviewed; never broaden it by
  coordinate radius or class.
- Preserve the R7 encounter-death handoff invariant. Ordinary consumption
  requires valid context but never rechecks an accepted event's time window;
  clear only successfully applied bits, retain an apply-exception bit, and let
  F7/disable/travel/suppression boundaries settle numeric state without renderer
  mutation. Only process or UObject-array shutdown may hard-clear the mask. Do
  not add a poll, timer, scan, SQL query, queue, or new steady schedule.
- Preserve runtime evidence chronology. Never extend an acceptance claim beyond
  the newest observed `START` and corresponding gameplay logs.
