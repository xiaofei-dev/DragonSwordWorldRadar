# Nexus Feedback Audit for 2.0.0

Audit date: 2026-08-28

Source: [DragonSword Native World Radar posts](https://www.nexusmods.com/dragonswordawakening/mods/254?tab=posts) and the public Bugs tab available on the audit date.

This audit separates implementation and package evidence from gameplay evidence. A source change, passing build, or installer matrix does not prove the original reporter's environment is fixed.

This file records the historical public 2.0.0 audit state. Post-2.0 corrective
source, current build identity, and unvalidated owner acceptance are tracked in
`RUNTIME_FEEDBACK_AUDIT_2_1_0.md`; later source changes must not be inferred from
the 2.0.0 package evidence below.

| Feedback | 2.0.0 disposition | Remaining acceptance |
| --- | --- | --- |
| Installer rejects an unknown game build or hash | Removed as a compatibility gate. Setup uses structural PE/layout validation; observed hashes are provenance only. | At the 2.0.0 audit checkpoint, Setup passed `20/20`; reporter-side compatibility remained untested. |
| Users cannot identify the required UE4SS build, Stable-root is rejected, or an existing runtime hash differs | Setup can bootstrap or convert to the pinned ExperimentalNested runtime. A separate With-UE4SS manual archive carries that exact runtime. Existing compatible loader/proxy hashes are not allowlists. | At the 2.0.0 audit checkpoint, Setup `20/20` and manual `2/2` passed; the old crash report had no reporter-side retest. |
| Manual install starts but clock, treasure, or quest markers are absent | Both manual archives now use a direct `DS/Binaries/Win64` copy root and document exact `mods.txt` merge behavior. | At the 2.0.0 audit checkpoint, manual copy passed `2/2`, including direct-copy layout and clean-target checks; reporter-side retest remained outstanding. |
| F7 appears to do nothing after installation | The final packages carry one exact enabled `mods.txt` line, the native DLL, catalogs, configuration, and runtime metadata in the selected layout. | No original diagnostic log was supplied. Treat as package/load-path addressed but not gameplay-verified in that environment. |
| Only the clock appears while all marker categories are absent | The clock proves that the native draw path loaded, but it does not identify whether the installed catalogs, save reconciliation, visibility configuration, or runtime ownership state failed. The final three package variants and Setup repair path are checked for complete catalog/config payloads. | This report appeared after the earlier audit and has no 2.0.0 diagnostic log or reporter-side retest. Package completeness can be verified automatically; the original runtime symptom remains `NOT_VALIDATED`. |
| Uninstall procedure is unclear | Setup exposes an owned-product uninstall transaction; manual removal is documented separately. | At the 2.0.0 audit checkpoint, Setup including uninstall/rollback passed `20/20`; reporter-side retest remained outstanding. |
| Windowed mode loses compact markers or the clock | Compact geometry now samples viewport size and DPI on the existing one-hertz geometry service and moves the radar and clock together only on a real edge. | Exact windowed/fullscreen/DPI gameplay matrix is `NOT_VALIDATED`. |
| 21:9 expanded-map markers are offset | Historical 2.0.0 disposition: the report remained unresolved in reporter gameplay. Post-2.0 source now uses witnessed parent-local geometry and live parent extents, but that correction belongs to the 2.1.0 audit and is not accepted by this package record. | Exact native 21:9, internal-black-bar, 16:10, and windowed gameplay remain `NOT_VALIDATED`. |
| Native map icons shift, markers appear in menus, maximum zoom changes layering, or Alt-Tab loses the expanded layer | Historical 2.0.0 disposition: world/activity ownership, TitleMap shutdown, zoom-event restack, replacement-layer ownership, and bounded rearm were implemented without focus polling. Later five-deadline geometry work is post-2.0 and tracked separately. | Maximum zoom, menu, Alt-Tab, and replacement-layer gameplay matrix is `NOT_VALIDATED`. |
| Some of the 147 area quests are missing, a Bounty Hunter task is not shown, or a cooking/delivery task remains after completion | F6 provides `AREA QUEST MODE: AVAILABLE / ALL`. `ALL` exposes every unfinished catalog task while still hiding saved or exact runtime completions. An exact dynamic catalog event arms without prior `PROGRESS` but is not completion; exact `END` is preferred. F7 records exact per-ID `COMPLETE_CNT` baselines, and an unresolved witness may use immediate/15-second/15-second exact-ID checks only when that baseline is known. Only strict count growth confirms the current completion; three misses lock that task generation instead of repeatedly querying. No periodic SQL is added. | The full 147-task gameplay matrix, cooking/delivery completion, and prerequisite accuracy of `AVAILABLE` remain `NOT_VALIDATED`. |
| The time-gated Assault and its companion task are absent outside the authored time window | F6 now provides independent `ASSAULT MODE: AVAILABLE / ALL`. `ALL` bypasses only the Assault time filter; state readiness and future 120-minute cooldown remain authoritative. Area-quest prerequisite logic remains strict. | Exact AVAILABLE/ALL gameplay and cooldown behavior are `NOT_VALIDATED`. |
| Mounted or underwater treasure remains visible after collection | The exact current mount/Rider interaction path can hide the matching treasure immediately. A bounded positive-only save confirmation remains as a 15-second fallback. | The fallback was owner-observed; a fresh underwater treasure for the immediate path was unavailable, so that route remains `NOT_VALIDATED`. |
| Long-term FPS or stutter concern | The reporter later stated the result was acceptable. 2.0.0 adds no global UObject enumeration, periodic SQL, dynamic queue, save-file watcher, focus poll, or expanded-map steady schedule. | External frame-time and owner gameplay acceptance remain `NOT_VALIDATED`. |
| Compatibility with a third-party Treasure Box Respawn mod | No compatibility claim is made. The Radar intentionally treats authoritative completion/cooldown state as persistent and may conflict with a mod that changes respawn semantics. | `UNVERIFIED`; test separately before claiming support. |

## Release conclusion

The historical 2.0.0 automated evidence passed the core and four static source gates,
the native `/WX` build, Setup `20/20`, manual installation `2/2`, payload
equivalence, direct-copy layout, clean-target checks, and three-archive release
build. This closes the automated install-policy, package-layout, load-control,
and uninstall gates. No deployment was performed. Exact windowed, 21:9, 16:10,
area-task, Assault-mode, underwater-immediate, maximum-zoom, Alt-Tab, gameplay,
and external frame-time matrices remain separate owner acceptance work. It is
not evidence for the current 2.1.0 source or build.

Public upload authority and third-party redistribution/provenance clearance are also separate from technical readiness; see `docs/RELEASE.md` and `docs/DEPENDENCY_SOURCES.md`.
