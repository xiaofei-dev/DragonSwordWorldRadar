# DragonSword Native World Radar 2.1.0 Acceptance Checklist

Mark an item only from evidence for the exact recorded bytes. Source, build,
package, installation, gameplay, performance, and publication are independent.

The current 2.1.0 source passed the core gate, all four static source gates, the
clean native `/WX` build, Setup `20/20`, manual-copy `2/2`, payload equivalence,
and three-archive re-extraction. The packaged `main.dll` SHA-256 is
`D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
It is sealed in the authoritative `dist/final-2.1.0` package but has not been
deployed or gameplay-validated. Older `4AFE...` and `BDE21...` development
artifacts are historical and non-authoritative. Installation, gameplay,
performance, and publication remain independent and unaccepted.

The 2.1.0 runtime feedback matrix is maintained in
`docs/RUNTIME_FEEDBACK_AUDIT_2_1_0.md`. In particular, wheel-zoom retained-host
reparenting, windowed/21:9/16:10 alignment, and cooking/delivery completion
remain owner-runtime acceptance items even after their automated gates pass.

## Release identity

- [x] Version is `2.1.0` in source, DLL marker, metadata, Setup, and all three
      archives.
- [x] Runtime label is
      `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_0` everywhere.
- [x] Public diagnostics default to `debug_logging=false`; exact legacy
      `event_log_enabled=true|false` files remain accepted during updates.
- [x] The only public runtime layout is ExperimentalNested.

## Compatibility and installer behavior

- [x] The selected game path is the exact DragonSword shipping executable and
      passes bounded executable AMD64 PE32+ validation.
- [x] A structurally valid changed game hash is accepted; the observed hash is
      used only for transaction identity and install-record provenance.
- [x] Existing ExperimentalNested `UE4SS.dll` and root `dwmapi.dll` pass
      bounded AMD64 PE32+ DLL validation and the required settings, Mods, and
      controlling `mods.txt` structure exists.
- [x] Structurally valid changed loader/proxy hashes are accepted and those
      existing bytes are not overwritten during Update / Repair.
- [x] Embedded bootstrap/conversion resources and the Radar payload retain
      exact SHA-256 integrity validation.
- [x] Missing UE4SS selects Bootstrap Install.
- [x] Root, dual, incomplete, or malformed UE4SS selects confirmed Conversion
      Install with a complete verified retained backup.
- [x] Compatible UE4SS plus no owned Radar selects Install.
- [x] Compatible UE4SS plus a strictly owned Radar, including an older owned
      version, selects Update / Repair.
- [x] A compatible game or UE4SS update never prompts for a rebuild based only
      on a changed hash.

Status: `INSTALLER_COMPATIBILITY = HISTORICAL_20_CASE_MATRIX_PASSED_CURRENT_RESEAL_PENDING`

## Ownership and user state

- [x] Existing-target ownership requires top-level schema-5 release metadata,
      schema-1 package metadata, a bounded unique manifest, exact immutable
      path/size/hash identity, a runtime-label-bearing `dlls/main.dll`, and an
      exact recursive tree limited to the declared live-state allowlist.
- [x] Unknown same-name ownership, unsafe paths, reparse points, duplicate
      entries, overflows, or immutable mismatches fail before mutation.
- [x] `config/visibility.ini` is preserved byte-for-byte after validation.
- [x] Current-source gate: the 4 KiB startup-only visibility parser accepts the
      complete named `[radar]`, `[map]`, and `[modes]` format plus strict legacy
      schema 1-4 documents, fails malformed input to safe defaults, performs no
      hot polling, and atomically upgrades format only on a real F6 change.
- [x] `config/diagnostics.ini` is preserved byte-for-byte after validation.
- [x] `data/defaults/treasure_overrides.txt` is preserved byte-for-byte after
      bounded UTF-8 and unique positive `ignore <save-id>` validation.
- [x] Clean install creates the three live user files from embedded defaults.
- [x] No `.example.ini` or `enabled.txt` file is installed.
- [x] Update / Repair refreshes the Radar DLL, immutable generated catalogs,
      release metadata, and mapping file without regenerating PAK data locally.

Status: `USER_STATE = HISTORICAL_SETUP_AND_MANUAL_MATRICES_PASSED_CURRENT_RESEAL_PENDING`

## Load control and transaction safety

- [x] Setup performs all required stopped-game checks and never launches or
      terminates the game.
- [x] The selected ExperimentalNested `mods.txt` is the sole load authority.
- [x] Exactly one `DragonSwordNativeWorldRadarPostRender : 1` entry remains.
- [x] Every valid `DragonSwordWorldRadarObjectState` entry is removed without
      writing a redundant disabled entry.
- [x] Unrelated `mods.txt` bytes, encoding, BOM, comments, and line endings are
      preserved.
- [x] Active or malformed `DragonSwordWorldRadar` load control rejects with
      zero mutation; exact disabled `DragonSwordWorldRadar : 0` is preserved.
- [x] Normal Install and Update / Repair use a temporary rollback journal and
      leave no persistent backup after a verified commit.
- [x] Bootstrap/Conversion retains one complete verified original-layout
      backup.
- [x] Every injected failure restores all recorded mutations byte-exactly.

Status: `TRANSACTION_SAFETY = HISTORICAL_20_CASE_MATRIX_PASSED_CURRENT_RESEAL_PENDING`

## Static and installer gates

- [x] Native and installer sources compile through their checked-in toolchains.
- [x] The clean native `/WX` build passes and records `main.dll` SHA-256
      `D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
- [x] Native lifecycle, UObject retention, bounded-work, fail-closed, renderer,
      and performance static gates pass.
- [x] The current Setup is resealed as an unsigned .NET Framework 4.8 x64
      WinForms executable with an
      administrator manifest.
- [x] The current-build isolated installer runner reports exactly:

```text
expected=20
passed=20
failed=0
skipped=0
release_gate=PASSED
sources_unchanged=true
fixtures_cleaned=true
```

- [x] The recorded evidence identifies the exact current Setup SHA-256 as
      `EFACFD9B59B0DBB29A61CF7FA053F369C2452638CCC7D0EC8212D2FC64226337`
      in `dist/final-2.1.0/release-manifest.json`.
- [x] The current-build isolated manual runner reports exactly 2 passed, 0 failed, and 0
      skipped for the No-UE4SS and With-UE4SS ExperimentalNested archives.

Status: `SOURCE_STATIC_NATIVE_BUILD_INSTALLER_MANUAL = PASSED_CURRENT_CORE_PLUS_4_STATIC_NATIVE_WX_PLUS_20_PLUS_2`.

## Package

- [x] Staging begins from an empty project-owned non-reparse directory.
- [x] The installer archive is exactly
      `DragonSwordNativeWorldRadarPostRender-v2.1.0-Installer.zip`.
- [x] It contains exactly:
  - [x] `DragonSwordNativeWorldRadarPostRender-Setup-2.1.0.exe`
  - [x] `DragonSwordNativeWorldRadarPostRender-Setup-2.1.0.exe.sha256`
  - [x] `INSTALL.md`
  - [x] `THIRD_PARTY_NOTICES.txt`
- [x] The sidecar matches the exact Setup bytes.
- [x] The two manual archives are exactly
      `DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-No-UE4SS.zip` and
      `DragonSwordNativeWorldRadarPostRender-v2.1.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`.
- [x] Both manual archives contain the same source-bound Mod payload and one
      clean single-product `ue4ss/Mods/mods.txt`; only With-UE4SS contains the
      pinned verified ExperimentalNested runtime. Both are script-free and map
      directly to `DS/Binaries/Win64`. The No-UE4SS instructions prove both the
      missing-file copy route and the existing-file manual-merge route without
      overwriting unrelated entries. Neither contains StableRoot.
- [x] Fresh extraction of all three ZIPs matches staging byte-for-byte.
- [x] `dist/final-2.1.0` contains exactly those three ZIPs,
      `release-manifest.json`, and `SHA256SUMS.txt`.
- [x] No source, log, backup, runtime state, unrelated Mod, StableRoot payload,
      or local debug override is included.

Status: `PACKAGED = PASSED_CURRENT_D4EE_THREE_ARCHIVES`; existing 4AFE/BDE21
outputs are historical and non-authoritative.

## Deployment

- [ ] Deployment is explicitly authorized and the game is closed.
- [ ] The selected deployment path and transaction are recorded.
- [ ] The post-install release identity, native DLL, and load-control state
      verify.
- [ ] Public diagnostics differ only through a separately recorded installed
      testing override.
- [x] No deployment or public Setup conversion was performed for the current
      bytes.

Status: `DEPLOYED = NOT_PERFORMED`

## Gameplay and performance

- [ ] Cold launch, F7 activation, F8 disable, and F8/F7 resynchronization work.
- [ ] F6 visibility settings apply and persist independently for radar/map;
      bird eggs have one independent RADAR toggle and an unavailable MAP cell.
- [ ] F6 `AREA QUEST MODE` persists independently: `AVAILABLE` retains strict
      prerequisite filtering, while `ALL` shows unfinished catalog tasks and
      still hides saved or exact runtime completions on both maps.
- [ ] F6 `ASSAULT MODE` persists independently: `AVAILABLE` retains the authored
      Assault state, time, and future 120-minute cooldown filters, while `ALL`
      shows all 40 static Assault records regardless of defeat/cooldown state.
      Switching back to `AVAILABLE` restores the live filters immediately.
      Boss and area-quest visibility remain unchanged in both modes.
- [ ] Compact treasures, Bosses, Assaults, mini-games, area tasks, bird eggs,
      clock, and height pointers render and move correctly. Treasure and area-
      quest height channels must remain independent, color-distinguished, and
      simultaneously visible when both targets qualify.
- [ ] During one F7 activation, switch fullscreen to windowed and back (including
      a DPI-changing resolution) and verify compact markers and clock reflow on
      the next one-hertz sample without F8/F7, duplication, or accumulating
      layout work.
- [ ] At native 21:9, a 4K viewport with internal 21:9 black bars, 16:10, and
      windowed 16:9, open, pan, zoom, close, and reopen the expanded map. Every
      category must stay aligned through player-icon `LocalToAbsolute` into the
      exact witnessed native-parent `AbsoluteToLocal` space, with X/Y scaled by
      that parent's live local width/height. A deterministic 3840x1600 unit input
      is not runtime acceptance. Initial attach may use only its bounded three-
      attempt readiness service. A later trigger must produce only the five
      deadlines at 100/250/500/1,000/1,250 ms, one observation per due pass,
      no overdue multi-observation collapse, and no tree mutation in the first
      four passes. The final pass may rebase equal extents or consume one
      bounded full attach for the current stable extent-change baseline; only a
      successful fresh attach establishes the next baseline. No sampled UObject wrapper or
      `FGeometry` may cross passes; no centered/desktop fallback or steady poll
      is allowed.
- [ ] Both exact bird-egg classes use only the fixed 512-slot weak pool, at most
      eight unknown-position queries per 250 ms control tick, and a nearest-16
      active bound. They never render on the expanded map or invoke enumeration
      or SQL.
- [ ] Expanded map categories render through first open, close/reopen, pan,
      zoom, maximum zoom, minimize/restore, and state-delta rebuild.
- [ ] Clustered and vertically overlapping treasures all clear correctly.
- [ ] An uncollected underwater mount-only chest emits one exact current-Rider
      interaction and clears immediately without F8/F7. The log identifies
      `local_mounted_rider_net_multi_execute_interact_prop`; an unrelated or
      unverified non-Pawn event remains visible and may use at most two exact-
      category positive-only requests. There is no third request, unrelated
      removal, poll, or full treasure query.
- [ ] Boss and Assault live departure does not clear them; real defeat does,
      without requiring F8/F7.
- [ ] Immediate, cooking/delivery, time-gated, and repeatable area tasks update
      without requiring F8/F7. The cooking/delivery path emits
      `progress_precondition=not_required`, treats the dynamic event as arming
      rather than completion, and hides through exact `END` or a positive
      baseline-relative exact-ID save result. F7 must establish the exact
      per-ID `COMPLETE_CNT` baseline; missing means zero only for a valid
      single-owner query, while an unknown baseline queues no SQL. The fallback
      accepts only strict count growth, runs immediately, and may retry twice at
      15-second intervals; no fourth attempt or periodic SQL is allowed. After
      exhaustion, repeated same-generation events remain locked until new F7
      or current `NONE`/`END` followed by later `ACCEPTABLE`/`PROGRESS`.
- [ ] Dungeon/travel/open-world return restores the radar without stale
      objects, repeated recovery, or a crash.
- [ ] Long-session owner observation and frame-time capture show no cumulative
      work, unbounded retry, unexpected auto-disable, or new stutter source.
- [ ] A diagnostic-enabled restart reports `config_debug_logging=true` and
      `log_schema=2` plus monotonic `seq`, `utc_ms`, and `elapsed_ms`. Ordinary
      records enter the fixed 256-record non-blocking queue, writer batches
      contain no more than 16 records, idle partial batches flush within 250 ms,
      and `ENGINE_TICK_PROFILE` reports zero or explained `logger_dropped` and
      `logger_truncated` counts. The current file stays at or below 1 MiB, and
      exactly one previous file is retained. A public-default
      `debug_logging=false` restart must return before formatting, locking,
      directory creation, rotation, or file I/O.

On 2026-08-31 the owner reported completed gameplay testing and accepted the
current version. Unchecked fine-grained rows above mean that no independent
retained artifact was recorded for that prescribed scenario; they do not
override the owner acceptance decision. The tested installed DLL was not
independently hash-matched to the sealed `D4EE...` artifact.

Status: `GAMEPLAY_ACCEPTED = OWNER_ACCEPTED_2026_08_31`

Status: `INSTALLED_ARTIFACT_HASH = NOT_RECORDED`

## Publication

- [ ] Unreal Engine/UEPseudo authorization and license compatibility cleared.
- [ ] Exact `e_sqlcipher.dll` source/build provenance cleared.
- [ ] Generated catalog and derived coordinate redistribution rights cleared.
- [x] User explicitly authorizes publication/upload of the workspace source.

Status: `SOURCE_PUBLICATION_AUTHORIZED = PASSED`

Status: `BINARY_AND_DERIVED_DATA_PUBLICATION = BLOCKED`
