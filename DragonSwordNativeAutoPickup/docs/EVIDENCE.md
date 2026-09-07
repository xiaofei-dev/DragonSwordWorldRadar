# Evidence

## 1.3.1 deferred startup-readiness correction

The first installed 1.3.1 session on Steam build `25076183`, game SHA-256
`B3E0B8CAB6752ACB981E104CA95A0105F76FCDD42EE622A8063AB8DE44FCA94C`,
loaded the exact Mod and passed the pinned UE4SS fingerprint. At 6,220 ms it
recorded `required_reflection_object_missing`: every required class, function,
and property was present except `interactable_cdo=false`. No selector attempt,
F9 event, scan, or injection occurred. This isolates a startup-readiness race
before selector resolution; it is not range-PAK, debug-logging, input, or
pickup-scheduler evidence.

The corrective 1.3.1 source registers one bootstrap EngineTick after the pinned
UE4SS fingerprint gate. Only the exact CDO-not-ready predicate is deferred at
250 ms intervals for at most 30 seconds. Operational EngineTick work rejects
all non-Ready states. Recovery reruns the complete reflection validation and
dual-anchor selector consensus before registering F9 and publishing `READY`.
Static/source/core/native-artifact, installer 11/11, deterministic archive,
exact-entry, and checksum validation passed. The corrective DLL is 944,640
bytes / `44FFCECD0CCC4CB1BA30502F147E1E439F919D229A4B9DD5D72B66C1155D1B64`.
The unsigned Setup is 13,027,328 bytes /
`8B3B883EFB8BF1E269643D98A0B0E5270682E17157FF92321D8110335BAF4B8D`.
The installer, manual-without-UE4SS, manual-with-UE4SS, and unchanged range ZIP
hashes are respectively
`B9243CFD00AE87574923CFD986EC01CD9E06A6228C15DEDCE319D968C7E0553A`,
`0955697D4F83B91CDBF103188CEEB1958114C5959148E7FB22BC8E6222E5F3B8`,
`630ECBA521836A2F4F3C216E809068D328EF21D51BA582B642DB95A643DADEC0`,
and `A346C4F20CF85C60FD2965FCC583129AFD8EB2B805CF4E20A80C1B20B79B49FE`.
Deployment and owner gameplay acceptance remain separate until recorded below.

## 1.3.1 high-range drop-list correction

Version 1.3.1 preserves the accepted 1.3.0 native pickup logic and changes the
separate range-PAK policy only. The 50 authored gather/animal targets retain the
selected 15x or 20x multiplier; the 19 short-lived type-7 drop overlap targets
are capped at 10x in both high-range variants. The exact 1.3.0 3x/5x/10x PAK
containers are reused unchanged.

The category-aware range build and independent release verifier passed with 69
targets, 138 entries, zero treasure targets, and all 38 packed drop entries in
each high variant equal to the corresponding 10x entries. Current range hashes:

- 3x: `6BB99A1E35C06EB0284370B9D7BD2F34E90CB6DCA7479CF10A477C68EA0103E8`;
- 5x: `DB9E129D8F8FCCA025864EC908C13C70F950AD779C37CF13A41164476587CECD`;
- 10x: `6A1ADB7592BA0C70A17984DB3AC01348086AABE196F0FDAF914B3F52C7A395F1`;
- 15x with 10x drops: `F8330CEA2F127319887FD3718BC66E2825D39DFE21D635404FC7535C7FAA37EC`;
- 20x with 10x drops: `81214319100646CD5663940CACE3AFA8F3523F5E319AB7C4AE8D935443FB93D2`;
- standalone range ZIP: `A346C4F20CF85C60FD2965FCC583129AFD8EB2B805CF4E20A80C1B20B79B49FE`.

These are static/package results, not gameplay acceptance. The exact 1.3.1
package build has passed; deployment and the high-speed 10x/15x/20x gameplay
comparison remain separate gates.

## 1.3.0 release boundary

Version 1.3.0 preserves the game-owned selector behavior, saved-binding
Enhanced Input action, target policy, on-foot and mounted receiver routes,
World/session guards, and no-scan safety boundary. It removes the active fixed
selector RVA and replaces two deployed 1.2.0 scheduling policies:

- only one automatic injection may be in flight globally;
- one exact `Server_RunInteractV2` post observer compares the armed raw receiver
  and publishes only an atomic action marker; it does no logging, reflection,
  UObject read, game call, or state mutation;
- EngineTick consumes a matching marker, releases the global slot, and records
  dispatch evidence with `target_match_unproven=1` and
  `pickup_success_claim=0`; the same Component has a 750 ms re-entry delay;
- existing exact weak-identity/state evidence remains an alternate terminal
  signal inside the same fallback window;
- missing dispatch and exact confirmation retain a 750 ms fallback window, one selector-represented
  retry after 200 ms, and a 1500 ms self-expiring second-result backoff; no
  timeout record lasts for the whole activation;
- interaction-owner replacement resets pending/retry state even when the
  `UWorld` identity remains stable;
- one physical F9 press causes at most one transition, regardless of operating-
  system key repeat;
- normal and mounted modes retain exact LocalPlayer/controller/current-Pawn
  identity validation; and
- action and performance diagnostics are compact and bounded.

The compatibility resolver uses policy
`runtime_reflection_dual_caller_rel32_consensus_fail_closed`. After the exact
UE4SS hash and loaded-path gate, it validates the loaded PE32+ image, x64
`.pdata`/`CHAININFO` runtime-function bounds, and decoded instruction
boundaries. The reflected `Server_RunInteractV2` exec thunk identifies one
virtual slot; its interactable-CDO implementation must contain one structurally
valid selector call. The complete reflected `SetInteractUIV2` exec wrapper must
contain one unique terminal `E8 rel32` native-implementation call; that bounded
implementation must contain one structurally valid UI selector call. The two
asymmetric paths must identify the same selector address. No fixed selector RVA
or game-hash address table exists; failure, ambiguity, disagreement, a
non-executable address, or a guarded call fault keeps automation Off for the
process.

Deterministic resolver fixtures establish compatible relocation, Server
virtual-path/UI direct-wrapper consensus, malformed/ambiguous input rejection,
and PE/runtime-function bounds.
They do not prove resolution in an actual game process. The policy can support
relocation while the required code contracts remain compatible, but it does
not guarantee arbitrary recompiles.

Independent static review caught one P1 assumption before the final package:
`SetInteractUIV2::GetFuncPtr` is a `.pdata`-bounded direct rel32 native wrapper,
not a virtual-dispatch thunk. The resolver and fixtures were corrected before
any final artifact, deployment, or runtime-acceptance claim.

The earlier pre-compatibility 1.3.0 pinned ExperimentalNested build and offline
release pipeline passed. Its native DLL was 422,912 bytes with SHA-256
`BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188`.
Its unsigned Setup executable was 11,704,832 bytes with SHA-256
`570E9E81FA31DFE242999C8352311DF6E566D0B18DAEC2E103734C2A4D3447DB`.
Those pre-compatibility 1.3.0 archives were:

- installer ZIP: 8,295,733 bytes, SHA-256
  `77997522E754C19201D608D88F22E8BC182D27BB888EA40BA1B5E071A5838BF1`;
- manual Mod-only ZIP: 194,614 bytes, SHA-256
  `E45102AA75F149F72DFF33F1303E5D90D92AA3F32FD8F0B1251CDC329305F372`;
- manual Mod plus UE4SS ZIP: 8,279,208 bytes, SHA-256
  `304CE33C2606665FDFE0D8FF2D6B64F47CFAB533E3D705181D880C359973CF55`;
- standalone range PAK ZIP: 3,120,870 bytes, SHA-256
  `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.

The standalone archive republished the exact reviewed 1.2.0 range bundle under
the 1.3.0 release filename without changing its bytes. Every DLL, Setup, and
Auto Pickup archive hash above predates the compatibility repair and is retained
only as historical candidate evidence.

## 1.3.0 pre-lifecycle-correction offline release

The compatibility-repair pipeline passed static, source, core,
built-artifact, package-layout, manifest, first-party ASCII, exclusion,
deterministic ZIP, exact-entry, and complete-checksum gates. The isolated
installer state/conversion/collision matrix passed 10/10 with zero failed or
skipped fixtures.

- native DLL: 836,096 bytes, SHA-256
  `6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1`;
- unsigned Setup: 12,118,016 bytes, SHA-256
  `2E2D3F69114751FCBB14A5E972F994A619736FC54CB9C6CE9B524D787B50DDCD`;
- installer ZIP: 8,420,756 bytes, SHA-256
  `B9344F5E519A29BF8CE5F560805C0040EFA409ADADE047EE92FDA9234C47A502`;
- manual without UE4SS ZIP: 319,886 bytes, SHA-256
  `3E0C4656F64C472A86C5C498EAD04C7E38A773C3E4F003B2E93DDCDB2226E261`;
- manual with UE4SS ZIP: 8,404,482 bytes, SHA-256
  `2EFFFF81BA2CD936BD73F15F37A2C518D301A3E8E5558E138D327EC6AAFB293E`;
- standalone range ZIP: 3,120,870 bytes, SHA-256
  `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.

These exact values establish final offline build and package provenance. No
post-audit final artifact has yet been resolved in the game process, deployed,
gameplay accepted, or owner smoke-tested; those evidence classes remain
`PENDING`.

### 1.3.0 diagnostic evidence contract

Initialization emits one `SELECTOR_RESOLVED` record only after the Server
virtual path and UI direct-wrapper path agree on the same executable selector
target. `SELECTOR_UNAVAILABLE` records a fail-closed resolution status before F9
can enable automation.
`SELECTOR_FAULTED` invalidates the process capability after a guarded native
fault and forces automation Off. The logged selector RVA is diagnostic output
derived from the resolved address; it is not a configured input or fallback.

The User log emits `PICKUP_ACTION_INVOKED` after a successful input injection.
A matching post observer is consumed by EngineTick as
`PICKUP_DISPATCH_OBSERVED`; both records share scalar activation, action,
request, actor, component, and tick correlation. Dispatch observation means
only that the injected action reached the game interaction UFunction on the
armed receiver. It cannot prove that the game reselected the same target or
that inventory changed, so the record must include
`target_match_unproven=1 pickup_success_claim=0`. A no-dispatch fallback emits
`PICKUP_UNCONFIRMED` with elapsed time, attempt ordinal, bounded-retry status,
and expiring backoff state. No hook callback emits these logs directly.

With Debug enabled, `PLAYER_CONTEXT_CHANGED`, `SELECTOR_PAIR_OBSERVED`, and
`AUTO_SCAN_DEFERRED` are change-only and independently capped at 32, 64, and 32
records per activation. `ACTION_TRACE` is emitted once per bounded automatic
invocation. `PERF_AGGREGATE` exposes action, in-flight, dispatch, re-entry/
backoff, no-dispatch, debug-emitted/suppressed, and logger queue/drop/failure counters;
`PERF_TIMING` separates scheduler, scan, context, selector, validation, action
resolution, subsystem, injection, and logger-flush timings. Identity fields are
packed scalars for correlation only; no live UObject pointer or instance name is
retained by the diagnostic state.

## Deployed 1.2.0 runtime rejection evidence

The exact 413,696-byte 1.2.0 DLL with SHA-256
`9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6`
was deployed. In the bounded diagnostic session beginning
`2026-08-29T09:55:18Z`, the final reviewed aggregate at
`2026-08-29T10:14:39Z` recorded 21,191 automatic scans, 250 selector pairs,
250 Enhanced Input injections, 17 exact weak-identity confirmations, 30
timeouts, 38 confirmation supersedes, 27 transient selector-state mismatches,
zero selector faults, zero automatic fault disables, zero World resets, and
zero logger drops. One `Herb_01_C` identity received 61 injections over about
31 seconds. A separate 1.2.0 session injected one `Bird_Egg01_C` identity 35
times and one `Onion_01_C` identity 16 times.

Both reviewed sessions also showed an accepted disable transition, one
debounced repeat, and a later accepted enable transition. The logs cannot count
physical presses, but they prove the 500 ms rate limiter did not guarantee one
transition per physical press. These results reject 1.2.0 observational
confirmation, 250 ms same-target suppression, and rate-only F9 debounce as the
next release policy. They do not reject the selector or Enhanced Input action
route.

The same diagnostic session measured approximately 20 microseconds average per
scan and 2 microseconds average for the injection call, with one 1.885 ms slow
scan. That evidence does not attribute sustained CPU stutter to the selector
loop, but it also does not measure downstream game work triggered by repeated
injections or deferred log-file flush I/O. The owner-reported intermittent
manual-F unavailability is therefore mechanistically consistent with the Mod
repeatedly injecting the same live interaction action, but the logs contain no
physical-F event and do not prove the exact downstream rejection stage.

## Historical 1.2.0 release evidence

Version 1.2.0 preserves the owner-accepted selector, Enhanced Input action,
mounted Rider, fish/drop, treasure-exclusion, and bounded-scan route. Its new
source contract forces automatic pickup Off on main-menu/save/World
initialization, adds installer Install/Upgrade/Uninstall ownership states, and
adds independent 15x and 20x range resources.

The tested native ABI remains UE4SS v3.0.1 Beta #0 commit `1c1a1497` in the
ExperimentalNested layout. Game executable hashes are diagnostic only. The
1.2.0 release contract contains four archives: installer, manual without
UE4SS, manual with UE4SS, and the standalone five-choice range PAK bundle.

The isolated 1.2.0 installer matrix passes 10/10 fixtures. It verifies the
lifecycle and 15x/20x contract, exact-runtime install and owned upgrade,
unknown same-name rejection, owned uninstall, absent/unknown uninstall
rejection, all six mutually exclusive range states, and confirmed conversion
with Mod migration and stable backup-name collision suffixes. This is offline
installer evidence only.

The final ExperimentalNested DLL is 413,696 bytes with SHA-256
`9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6`.
The unsigned Setup executable is 11,695,104 bytes with SHA-256
`79F7D70C5F7D7A1F84AE4B75F415B362A4C30CEEB4A7B1AE5967D19053A9D323`.
Deterministic ZIP rebuilding, exact entry sets, checksum coverage,
first-party English/ASCII text, pinned upstream byte preservation, and
StableRoot/unrelated-third-party exclusion all pass. These hashes remain valid
historical release evidence. The deployed runtime result above supersedes the
former "not deployed" boundary and motivates the 1.3.0 corrective candidate.

The final 1.2.0 archives are:

- installer ZIP: 8,291,262 bytes, SHA-256
  `33721FF41A608DE8703F481CC2802698D47C3747001D86CCE887D18DF06BEA85`;
- manual Mod-only ZIP: 189,476 bytes, SHA-256
  `37CCA548CD44DEBD259C1898922AA2E44AACBE5DED6DDCA1A1EE5BCA9F8E25DA`;
- manual Mod plus UE4SS ZIP: 8,274,076 bytes, SHA-256
  `4DBB48559D3C6EF7A6D061FDEE19747FD1CB4A551F2CD541E36050B8BFDA9A31`;
- standalone range PAK ZIP: 3,120,870 bytes, SHA-256
  `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.

## Feature-complete runtime baseline

Automatic pickup is owner-accepted as functionally complete. The accepted
runtime chain was established incrementally:

- 1.6.7 visibly collected two independent NormalGather targets through the
  live Enhanced Input interaction action and confirmed both exact weak
  identities;
- 1.6.8 established persistent on-foot automatic pickup;
- 1.6.10 confirmed mounted Rider routing and two fish pickups;
- the accepted performance session covered 1,739 scans at approximately
  18-22 microseconds average and 378 microseconds maximum, with zero slow
  scans, selector faults, automatic disables, UObject scans, or logger drops.

The complete rejected-route and acceptance history is retained in
`ATTEMPT_LEDGER.md`. ProcessEvent return, build success, package creation, or
installation alone is never counted as pickup success.

## Historical public 1.1.1 implementation

Version 1.1.1 preserves the accepted selector and Enhanced Input action route.
It starts Off, uses F9 by default, supports fresh Pawn or mounted Rider
resolution, accepts NormalGather type 2, Animal type 5, and class-proven
DropItemActor type 7, and rejects TreasureBox type 4.

`interaction_key=AUTO` reads the saved semantic `INTERACT`
`ActionInputType=91` keyboard binding once per enable session. If that binding
is missing, ambiguous, chorded, unsupported, or unreadable, the configured
concrete `interaction_key_fallback` is used. A concrete `interaction_key`
remains a troubleshooting override.

The release performs no UObject or Actor enumeration, collision mutation,
overlap hook, direct pickup RPC, Windows synthetic input, or worker-thread
scan. The game executable hash is diagnostic only. The exact UE4SS file hash is
the mandatory post-load automation gate in 1.1.1; package and installer layout
controls are the pre-load compatibility boundary.

## Historical 1.1.1 build and package evidence

The ExperimentalNested native DLL is 404,992 bytes with SHA-256
`3A2BA3B251910439E7625863EC4981A34AA692F62B136B51054349CDC4E29A92`.
It targets UE4SS v3.0.1 Beta #0 commit
`1c1a1497f942c707f47ba668db75b25e86f6c08a`.

The historical 1.1.1 release archives are:

- installer ZIP: 8,172,484 bytes, SHA-256
  `F0AE39E42BB9ADDF6F4CA07A07439D43D08138E5F30095EDA29D2D7ACB96B123`;
- manual Mod-only ZIP: 184,461 bytes, SHA-256
  `B79A5247789431512FC332472B23D759FC07BFE1AD42B3ACA418E7D4E1D65F1A`;
- manual Mod plus UE4SS ZIP: 8,270,320 bytes, SHA-256
  `47BD40CA3DF269EB14A2DB12D5519FC438E3F7688C493F47D53FF7B988AFC5CE`;
- clean UE4SS compatibility runtime ZIP: 8,087,122 bytes, SHA-256
  `AB765EF93BD0DB109D7224C0E2487C68A1CE8748F20B597128D43E247B4AFA77`.

Strict native compilation, source and package checks, archive inspection,
unknown-game-hash installation, complete conversion backup, old-layout
removal, Mod migration, pinned-runtime installation, and `_2`/`_3` backup-name
collision handling pass offline.

## Inspected local installation

The inspected local game installation contains the exact 1.2.0 DLL: 413,696
bytes with SHA-256
`9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6`.
It is enabled through the authoritative `mods.txt` entry, starts Off, uses F9,
and resolves `interaction_key=AUTO` to the saved F binding in the reviewed
sessions. The second bounded session used a local Debug override. This is the
deployment that produced the rejection evidence above; it is not a 1.3.0
artifact.

## 1.3.0 reflection-fix diagnostic candidate

The first deployed exact final 1.3.0 artifact passed the pinned UE4SS gate but
disabled itself before the dynamic selector resolver ran. Its counters remained
zero for key events, scans, and selector attempts. That separates the startup
regression from range PAK selection, game-hash diagnostics, UE4SS identity, and
runtime address resolution.

The isolated candidate removes only the unused `SetInteractUIV2` parameter
schema from the required startup contract and emits bounded per-predicate
reflection metadata. It does not invoke that UI function, loosen action-bearing
reflection validation, introduce a fixed RVA, use the game hash for an address,
or publish a selector without exact Server/UI structural consensus.

- Candidate DLL: 912,896 bytes, SHA-256
  `4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F`.
- Candidate unsigned Setup: 12,196,352 bytes, SHA-256
  `4BEC3DCA4211E577A47E5DAC8CEBF106D4CB3FDED292CE06A164BA68438826C7`.
- Static/source/core/built-artifact gates: passed.
- Current installer lifecycle matrix: 10/10 passed with cleaned fixtures.
- Current exact owned 1.3.0 state: Repair and Uninstall enabled; Upgrade
  disabled; selected owned 20x range detected.
- Older exact owned releases: Upgrade; absent Mod: Install; unknown or modified
  same-name target: all mutation controls disabled.
- Deployment and exact-artifact gameplay: not performed and not validated.

The subsequent owner log moved the evidence boundary forward: startup
reflection, dual-path selector consensus, saved `INTERACT=F` resolution, F9,
and supported selector pairs all passed. The first action-resolution gate then
rejected `EnhancedActionKeyMapping.Action` before any injection with the
composite metadata reason. The bounded session recorded zero selector faults
and zero Enhanced Input attempts, separating this regression from the selector,
game hash, UE4SS runtime, range PAK, and the rejected Windows-F route.

The source correction explicitly accepts exact `FObjectProperty` and exact
`FObjectPtrProperty` for that mapped Action field, reads through
`GetObjectPropertyValue`, and preserves the `InputAction` class and all existing
layout/lifecycle gates. It does not accept the broad object-property family or
weak, soft, interface, or unknown storage. Static, build, package, deployment,
and owner-observed pickup evidence remain separate until the new exact artifact
passes their respective gates.

The final offline release produced the following exact artifacts:

- Native DLL: 913,920 bytes, SHA-256
  `01A6E1358FBFE0B9DB35B4ECAAB2F1F08425265F47D6B1873872AA56D9E002AC`.
- Unsigned Setup: 12,197,376 bytes, SHA-256
  `5E9B2672879596805644AAA5A5DD2E9B574BBA93A421D38E69AF1FB9A2EA3487`.
- Installer ZIP: SHA-256
  `349AD8E660E834D0992B67960203CE38ADB15D0335AB254C3593F745ADBA98E1`.
- Manual without UE4SS ZIP: SHA-256
  `28825D0ED2BD1329100A525EB23C7B77E92C2459CB917161D7DFA153E7324D49`.
- Manual with UE4SS ZIP: SHA-256
  `44220DE36189B9447AA3ED85AB731A57000A6C0A162376C1A1350E2E27DEF7BA`.
- Standalone range bundle ZIP: SHA-256
  `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.

The strict source and artifact gates passed, the current installer lifecycle
matrix passed 10/10, and all four archive classes passed exact-entry, checksum,
and deterministic-rebuild validation. The installer retains exact ownership of
the installed 1.3.0 reflection candidate so it can be repaired in place while
preserving configuration and unrelated Mods. No final artifact was deployed and
no exact-artifact gameplay acceptance was performed.

The owner subsequently installed the exact Action-property artifact and
confirmed that Auto Pickup works. Runtime evidence recorded dynamic dual-path
selector consensus, saved `INTERACT=F`, physical F9 acceptance, exact
`ObjectPtrProperty` Action storage, 11 injections, zero injection failures, zero
selector faults, and 7 exact weak-identity confirmations. Four mounted
normal-gather actions reached the full 1500 ms Actor-live timeout. Their pending
windows, rather than selector or scan cost, account for the observed slower
sequence; measured scans remained approximately 25-32 microseconds with no slow
scan events.

The next conservative candidate reduces only the terminal confirmation window
to 1000 ms. One global pending action, no timeout retry, exact-candidate
quarantine, 33 ms scan scheduling, exact reflected Action storage, dynamic
selector consensus, and fail-closed behavior remain unchanged. Its strict
native build, core tests, built-artifact validation, installer lifecycle matrix
10/10, and deterministic four-archive release gates pass. Exact tuning DLL:
913,920 bytes / SHA-256
`09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B`;
unsigned Setup: 12,197,888 bytes / SHA-256
`999ADF4D527A00947F56E747A40318DCF494B99D5710DF3CEE0CEAE8BEAB5EC2`.
It is not deployed and remains pending owner runtime acceptance.

Owner logs for that timing candidate then established a new failure mode. The
selector still resolved dynamically with zero selector faults, and scan work
remained in the tens of microseconds, but one Actor-live timeout permanently
excluded a candidate for the activation. The same logs showed travel replacing
the interaction-owner weak identity while retaining the same `UWorld`
identity. Those observations reject single-timeout permanent quarantine and
`UWorld`-only action lifecycle reset as the current policy.

The current corrective source adds exact component-state confirmation, one
game-selector-represented retry after a 200 ms cooldown, terminal quarantine
only after the second timeout, and interaction-owner lifecycle reset with a new
enforced 1500 ms settle interval. Retry cooldown and quarantine are keyed to
the exact returned Component and checked before live action mapping or
subsystem resolution. It also detects exactly one recognized range
PAK and applies its multiplier at `DropItemActor` BeginPlay to the exact
reflected `SphereOverlapComp`. Extracted ordinary-meat, aged-meat, coin, nut,
crystal, and other drop subclasses all inherit this component; the patch does
not touch their serialized physics or hit spheres. Fresh strict compilation,
core-test, built-artifact, installer 10/10, deterministic ZIP, exact-entry,
checksum, and provenance gates pass for this source. Its DLL is 925,696 bytes /
SHA-256
`5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08`;
the unsigned Setup is 12,210,176 bytes / SHA-256
`3EF1D37E771123CC8DCC192EC18CB89F62EE5823F245E78E39B7BF3CA0570898`.
The installer, manual-without-UE4SS, manual-with-UE4SS, and range archives are
SHA-256 `6B333A0ECDF9A7AF125E614DFC7A64BF03610BD83D1B5A4AB5FF080DE3EB1149`,
`70AAFB9B4B49FEAF3ED698FBC32D37FC78F4690AD76598BEED2DFB5CEA1DB2DB`,
`FF39DD1339CC9C2CF39F1578CDD70C0F606250150EF30EC83C55E2B647CD4014`,
and `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.
Deployment and gameplay acceptance remain separate gates.

The first 200 ms Setup omitted the immediately preceding legitimate 1.3.0 DLL
SHA-256
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
from its exact historical ownership contracts and therefore misclassified that
installed payload as unverifiable. The corrected Setup and installer ZIP are
the `3EF1D37E...` and `6B333A0E...` artifacts recorded above. Their 10-case
installer matrix includes an explicit Repair regression assertion for the exact
1.3.0 version/DLL/Lua tuple and retains zero-mutation rejection for changed or
foreign same-name files. The superseded first 200 ms Setup must not be used.

The older `Test-Installer.ps1` 20-case wrapper is not a valid 1.3.0 gate because
it expects the removed internal `InstallerEngine.Install` method. The current
release script uses `Test-Installer110.ps1`; its 10-case matrix is the retained
installer evidence.

## Historical 1.3.0 validation boundary (superseded)

This section preserves the evidence boundary as it stood before the canonical
1.3.1 offline release was built. It is not the current package status.

The 1.3.0 corrective and compatibility-repair history, including preceding
offline artifact provenance, remains preserved. The dispatch-observer repair is
a later source candidate and has no exact artifact receipt or runtime acceptance.
Selector resolution with the repaired binary, deployment, gameplay acceptance,
and owner smoke testing remain open evidence classes. The
smoke test must first
confirm Server virtual-path/UI direct-wrapper `SELECTOR_RESOLVED` consensus on
the reference build and fail-closed Off behavior on any incompatible or
ambiguous build. It must then cover true
F9 edge behavior under a held key, main-menu forced-
Off behavior, one on-foot pickup, one mounted pickup, fish or a drop, treasure
exclusion, one matching dispatch that releases global in-flight while preserving
the 750 ms same-Component re-entry delay, one first no-dispatch timeout followed
by at most one selector-represented retry after 200 ms, a second no-dispatch
1500 ms expiring backoff, automatic recovery, continued handling of unrelated
candidates, manual F during backoff,
same-`UWorld` travel, clean exit, one ordinary/aged meat or other monster drop
at the selected range, and a brief performance
comparison. It had to use the exact final 1.3.0 package and retain the historical
1.2.0 build and runtime results as separate evidence classes.

Rebound keyboard input, concrete gamepad-key override, Treasure Radar Overlay
coexistence, and additional game builds remain separate compatibility checks.
Game-build hashes never authorize an address: an unlisted build must either
resolve the same compatible code contracts or remain Off. Automatic selection
of the saved gamepad binding and Authenticode signing remain optional future
enhancements only if explicitly requested.

Build, package, installer, deployment, and historical runtime evidence remain
separate evidence classes.

## 2026-08-30 local final: timing, structured drops, and filename ownership

Current installed logs attribute the long second-pickup gap to the 1500 ms
confirmation window: selector and scan work remained in the microsecond range,
with zero selector faults. The scoped source change reduces that window to
750 ms and retains the 33 ms scan cadence, one global pending action, 200 ms
retry cooldown, two-attempt maximum, and second-timeout exact-Component
quarantine. The worst unconfirmed same-target bound is approximately 1700 ms;
positive invalidation/state evidence may confirm earlier.

All five range variants passed independent pack/list/unpack and semantic checks
with 69 targets / 138 entries: 50 reviewed gather/animal packages and 19 exact
`/Script/DS.DropItemActor` child packages. The structured patch authors only
`SphereOverlapComp.RelativeScale3D`; physics and hit spheres are protected and
treasure assets are absent. Native runtime range multiplication is compile-time
disabled. The deterministic standalone range archive is 3,919,600 bytes /
`504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.

The installer owns the five supported range PAK filenames and legacy canary
filename for replacement/removal. No historical PAK hash is required to
reconcile those exact names, while newly written embedded bytes are still
hash-verified. The final isolated installer matrix passed 10/10 fixtures.

The local final native DLL is 919,552 /
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
unsigned Setup is 13,000,704 /
`3027460CE5BC5EDA7F6DC8735245E14455A7851E9D990C1D027119269699E6AB`;
installer ZIP is 8,556,264 /
`4263919A5AC55389D9C100DD93BBBB808025E48E878ACC6F7875A0BB65A17634`;
manual-without-UE4SS is 362,076 /
`9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`;
and manual-with-UE4SS is 8,446,667 /
`55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`.
Static, core, native-artifact, PAK, installer, deterministic ZIP, exact-entry,
and checksum gates passed. The installed game was not changed; exact-package
deployment, gameplay, performance, and owner smoke testing remain
`NOT_VALIDATED`.

## 2026-08-30 durable ownership and deployment cleanup

Setup ownership schema 2 binds stable product ID
`8F4282F9-25C4-4EFC-A150-1C4A812C85B4` to the exact installed DLL, Lua, and
notices hashes recorded at installation. This removes the need to manually add
each valid predecessor DLL to the next installer. The closed file/directory
allowlist remains enforced, and a fixture that changes the installed DLL after
record creation is rejected with zero mutation. Legacy schema-1 tuples remain
only as one-time migration inputs; the actual installed
`5490BCC4...CDA08` predecessor has an exact assertion.

The canonical rebuild passed source/core, native-artifact, installer 10/10,
deterministic ZIP, exact-entry, checksum, and provenance gates. The unsigned
Setup is 13,001,728 /
`2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`;
installer ZIP is 8,556,743 /
`70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`.
Manual-without, manual-with, and range ZIP hashes remain respectively
`9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`,
`55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`,
and `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.

Before cleanup, that new Setup inspected the actual installed 1.3.0
`5490BCC4...CDA08` Mod as owned, Repairable, and uninstallable with approved
20x range. Its transactional Uninstall then removed the old Mod directory, the
old 20x PAK, and the authoritative `mods.txt` entry. The post-state was absent
with no range. No new package was deployed or launched during that cleanup
operation.

## 2026-08-31 historical owner acceptance and integration observation

A later local 1.3.0 diagnostic installation ran in the game and produced
selector, binding, action-invocation, mounted-context, timeout, and quarantine
evidence. The owner subsequently reported completed gameplay testing and
accepted the current version.

One session with native Radar also enabled showed a temporary loss of the
native `F` prompt and no exact AutoPickup confirmations during mounted flight.
Input injection itself did not report failure, the semantic interaction binding
resolved without a reported conflict, and normal behavior later recovered.
The run also contained Pawn/`ClientRestart` lifecycle changes and another
mount-speed modification. It therefore does not prove a direct Radar hook
conflict. See the repository-level `docs/INTEGRATION_STATUS.md` for the shared
evidence boundary.

Status: `PRE_OBSERVER_GAMEPLAY_ACCEPTED = OWNER_ACCEPTED_2026_08_31`.

Status: `INSTALLED_ARTIFACT_HASH = NOT_RECORDED`.

## 2026-08-31 dispatch-observer recovery candidate

The later log review separated two facts that the former confirmation model had
combined. A successful Enhanced Input injection can reach the game's
`Server_RunInteractV2` dispatch even when the exact selector Component remains
live, especially for type-2 gatherables. Waiting for identity invalidation or a
state transition therefore held the one global slot unnecessarily. A second
timeout then created activation-long exact-Component quarantine and repeated
500 ms scan deferrals, which could starve unrelated selector-presented targets.

The repair observes only the exact reflected `Server_RunInteractV2` post call.
The record is armed before injection and remains correlatable after the call
returns until matching dispatch, existing exact confirmation, timeout, or reset.
While that record is armed, the callback compares only the raw receiver
address and publishes one atomic action token. It performs no logging,
formatting, reflection, UObject read, game call, or state-machine transition.
The next actual EngineTick pulse consumes the token, correlates it to the same
action/request, releases the global in-flight slot, and logs
`target_match_unproven=1 pickup_success_claim=0`. Because that pulse normally
arrives roughly one game frame after injection, the existing 25 ms active/post
due has already been satisfied; consumption does not add another 25 ms wait and
the same tick may continue scanning. The exact Component retains a 750 ms
re-entry delay.

If no matching dispatch is observed, the fallback remains conservative: a
750 ms window, one retry after 200 ms, then a 1500 ms self-expiring Component
backoff. There is no activation-long quarantine and no 500 ms quarantine scan
loop. Engine/active/post due remains 25 ms and idle remains 33 ms.

The corrective DLL was built and passed source, manifest, core, native, and
built-artifact gates, then was locally deployed while the game was closed:

- file: `dist/dispatch-recovery-audit/main.dll`;
- size: 927,744 bytes;
- SHA-256: `AC86CF2FA26047CF713B567C1CA63D4AD424C86A3FF9C020B80CAD07B4211F5D`;
- deployed UTC: `2026-08-31T17:26:37.0305139Z`;
- installed DLL hash and ownership-schema-2 record: matching;
- `config.ini` and `mods.txt`: byte-preserved.

It has not yet been launched or exercised. Historical logs motivate it but do
not accept this exact artifact.

Status: `DISPATCH_OBSERVER_CANDIDATE = RUNTIME_PENDING`.

## 2026-08-31 native status-card polish candidate

The F9 status card was refined without changing selector, target validation,
action scheduling, retry, confirmation, or Enhanced Input behavior. The native
UMG tree now uses a 72% opaque deep-blue outer glass layer, 32% inner glass,
28% shadow, 13% highlight, and state-colored low-opacity glow/rule layers.
The pure display timeline uses smoothstep fade/slide easing; the host adds a
bounded 98.5%-to-100% reveal scale. Every widget remains hit-test-invisible and
the guarded renderer still fails independently with `pickup_unaffected=1`.

Source, manifest, selector/isolation, package-layout, core, native-build, and
built-artifact gates passed. The exact DLL was deployed while the game was
closed:

- file: `dist/status-toast-polish-audit/main.dll`;
- size: 943,104 bytes;
- SHA-256: `FA5B5485EAD59726AD80A027C2139083DD1C1C1D1FF9F294891320296F2CC8F7`;
- deployed UTC: `2026-09-01T00:00:48.2361864Z`;
- installed DLL hash and ownership-schema-2 record: matching;
- `config.ini` and `mods.txt`: byte-preserved;
- rollback: `runtime/rollback/installed-1.3.0-pre-status-toast-polish-20260831-170048`.

Static and hash evidence does not validate readability, animation quality, or
gameplay behavior. Those remain owner visual/runtime smoke-test items for this
exact artifact.

Status: `STATUS_TOAST_POLISH = DEPLOYED_RUNTIME_PENDING`.
