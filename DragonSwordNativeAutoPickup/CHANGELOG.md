# Changelog

## 1.3.0 - Pending-action, physical-edge, and runtime selector correction

- Tunes the local runtime confirmation window from 750 ms to 650 ms and the
  first-timeout retry cooldown from 200 ms to 100 ms. The latest Debug-on owner
  session confirmed successful actions in 542-584 ms, so 650 ms retains a
  bounded margin while reducing a two-timeout cycle from about 1700 ms to
  about 1400 ms. A follow-up lowers only engine/active and post-pickup cadence
  from 33 ms to 25 ms while retaining the 33 ms idle cadence. One-global-
  pending policy and the two-attempt limit remain unchanged.
- Replaces recurring manual predecessor-DLL ownership with installer record
  schema 2. The stable product ID and recorded DLL/Lua/notices hashes make each
  valid installed release self-describing for the next Repair or Upgrade, while
  immutable-file tampering and unknown same-name directories remain fail-closed.
  The exact installed `5490BCC4...CDA08` payload is retained only as a one-time
  legacy schema-1 migration input and has an explicit regression assertion.
- Standardizes generated work under `out/`, immutable releases under
  `dist/releases/<version>/`, and game installation to the single
  ExperimentalNested Mod directory plus at most one exact range PAK.
- Tunes only the confirmation window from 1500 ms to 750 ms after current logs
  showed approximately 1500-1530 ms between otherwise microsecond-scale selector
  actions. The 33 ms scan cadence, one global pending action, 200 ms retry
  cooldown, two-attempt maximum, and second-timeout exact-Component quarantine
  remain unchanged.
- Moves monster-drop range ownership entirely into the five PAK variants. Each
  now contains 50 reviewed gather/animal packages plus 19 structured type-7
  drop packages, including ordinary and aged meat; treasure assets are excluded.
  Native runtime range multiplication is compile-time disabled.
- Makes the five supported range PAK filenames and legacy canary filename
  product-owned. Installer replacement/removal no longer requires a historical
  PAK hash, while each newly written embedded PAK remains hash-verified.
- Completes the local-only final release: native DLL 919,552 bytes /
  `10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
  unsigned Setup 13,001,728 /
  `2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`;
  installer, manual-without-UE4SS, manual-with-UE4SS, and range ZIP SHA-256
  `70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`,
  `9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`,
  `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`,
  and `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.
  Static/package/installer gates pass; deployment and gameplay acceptance were
  not performed.
- Preserves the accepted game-owned selector behavior, live saved-binding
  Enhanced Input action, target policy, normal/mounted receiver routing, and
  lifecycle safety boundaries while removing the active fixed selector RVA.
- Resolves the Server side through the reflected `Server_RunInteractV2` exec
  thunk, its unique virtual slot, the interactable CDO entry, and the bounded
  native implementation.
- Resolves the UI side without assuming virtual dispatch: the complete reflected
  `SetInteractUIV2` exec wrapper must contain one unique terminal `E8 rel32`
  implementation call; that implementation is runtime-function bounded and
  must satisfy the UI selector structural contract.
- Requires the Server and UI paths to identify the same selector address before
  publishing one process-lifetime capability.
- Names this policy
  `runtime_reflection_dual_caller_rel32_consensus_fail_closed`.
- Parses the loaded PE32+ image, bounds each reflected implementation through
  the x64 `.pdata` runtime-function table and bounded `CHAININFO`, and decodes
  only real instruction boundaries. Missing, malformed, ambiguous,
  inconsistent, or non-executable contracts keep automation Off.
- Uses no game-hash address table and has no fixed-RVA fallback. The game hash
  remains diagnostic-only. Compatible code-contract relocation can resolve;
  arbitrary future recompiles are not guaranteed.
- Records a P1 static-review correction made before the final package: the
  `SetInteractUIV2` reflected `GetFuncPtr` is a direct rel32 native wrapper, not
  a virtual-dispatch thunk. No final artifact or runtime acceptance predates
  this correction.
- Allows only one globally pending automatic action. A pending candidate blocks
  every later automatic invocation until it reaches a terminal result.
- Counts success only when the exact candidate weak identity becomes invalid.
  An unconfirmed timeout quarantines that exact candidate for the current
  activation and cannot retry it until a deliberate F9 Off/On cycle; unrelated
  candidates may still proceed.
- Replaces the 500 ms rate limiter with a true physical F9 edge: the first
  keydown may transition state, all key-repeat events remain latched out, and a
  release is required before another transition.
- Retains strong LocalPlayer, trusted controller, bidirectional current-Pawn,
  and mounted Rider identity validation without caching gameplay UObjects.
- Requires exact reflected object, class, struct, and array parameter classes
  and bounded layouts. UObject and UClass parameters use the pinned SDK object
  property accessors; incompatible weak, soft, interface, or other storage
  variants fail closed before ProcessEvent.
- Corrects the runtime action-resolution regression caused by requiring the
  `EnhancedActionKeyMapping.Action` field to be only an exact
  `FObjectProperty`. The field now accepts the explicit exact UE5 allowlist of
  `FObjectProperty` and `FObjectPtrProperty`, still reads through
  `GetObjectPropertyValue`, and still requires the resolved object to be an
  `InputAction`. Weak, soft, interface, and unknown storage remain rejected.
- Splits the former composite action-mapping metadata failure into named field
  failures and records the accepted Action storage kind in bounded action
  diagnostics.
- Records owner runtime acceptance of the exact Action-property release: the
  dynamic selector, saved `INTERACT=F` binding, F9 edge, `ObjectPtrProperty`
  Action storage, on-foot and mounted actions, and visible pickup all executed.
- Conservatively reduces the single pending-action confirmation window from
  1500 ms to 1000 ms. The 33 ms scan schedule, one-global-pending invariant,
  exact-candidate timeout quarantine, and zero automatic retry remain unchanged.
- Supersedes that timing-only policy after owner logs showed false terminal
  timeouts permanently excluding otherwise valid persistent gather actors and
  same-`UWorld` travel replacing the interaction owner without resetting the
  action activation.
- Confirms a pending action when the exact actor or component weak identity is
  invalidated, or when the exact still-live component changes out of
  `InteractableValue=2`. Component, outer, type, and active-World identity are
  revalidated under a guarded probe before accepting that signal.
- Restores the 1500 ms upper confirmation bound, but normally releases pending
  work as soon as the component-state signal arrives. A first unconfirmed
  timeout enters a 200 ms cooldown; the same exact candidate receives at most
  one retry and only if the game selector presents it again. A second timeout
  quarantines that identity for the current activation.
- Treats a changed interaction-owner weak identity as an action-context reset
  even when the `UWorld` identity is unchanged. Pending evidence and retry
  records are cleared and the normal 1500 ms settle delay is applied; changing
  only the mounted Rider receiver does not trigger this reset.
- Extends the selected 3x, 5x, 10x, 15x, or 20x option to every class-proven
  type-7 `DropItemActor`, including ordinary meat, aged meat, and other monster
  drops. The adapter detects exactly one recognized PAK, validates the exact
  reflected `SphereOverlapComp`/`SetSphereRadius` contract, and applies the
  multiplier once at BeginPlay. It never resizes `CapsulePhysicsComp` or
  `SphereHitComp`, performs no object scan, and disables only this range bridge
  on ambiguity or reflection drift.
- Completes the exact offline release for that conservative timing change.
  Static/source/core/artifact gates, installer lifecycle tests 10/10, and the
  deterministic four-archive release checks pass. Native DLL: 913,920 bytes /
  `09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B`;
  unsigned Setup: 12,197,888 bytes /
  `999ADF4D527A00947F56E747A40318DCF494B99D5710DF3CEE0CEAE8BEAB5EC2`.
  Deployment and exact timing acceptance remain separate owner tests.
- Corrects the exact-package startup regression observed after the first final
  1.3.0 deployment. The runtime had rejected initialization before either
  selector path ran because the unused `SetInteractUIV2` parameter schema was
  treated as a required action contract.
- Keeps the UI reflection details as bounded diagnostics instead of an
  activation gate. The UI function is never invoked and its parameters are
  never read; selector publication still requires executable PE bounds, one
  unique terminal `E8 rel32` implementation call, structural validation, and
  exact agreement with the independently resolved Server path.
- Adds `REFLECTION_CONTRACT_DETAIL` logging with a named required-failure clause
  and the actual reflected parameter sizes, flags, classes, offsets, element
  sizes, array dimensions, and return-property state.
- Compacts action diagnostics and keeps performance output interval-aggregated
  so routine operation no longer emits the 1.2.0 seven-line action chain.
- Records the deployed 1.2.0 rejection evidence: 250 injections, 17 exact
  weak-identity confirmations, 30 timeouts, and 38 supersedes in the bounded
  session, with one target receiving 61 injections. Two separate F9 sequences
  also returned to Enabled after a debounce rejection.
- The earlier pre-compatibility 1.3.0 release candidate completed the strict
  pinned ExperimentalNested build and offline release pipeline. DLL: 422,912
  bytes /
  `BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188`;
  unsigned Setup: 11,704,832 bytes /
  `570E9E81FA31DFE242999C8352311DF6E566D0B18DAEC2E103734C2A4D3447DB`.
  Those hashes predate the compatibility repair and are retained as historical
  evidence only.
- Completed the post-audit final offline release. Static, source, core,
  built-artifact, isolated installer 10/10, deterministic ZIP, exact-entry, and
  checksum gates pass. Final native DLL: 836,096 bytes /
  `6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1`;
  unsigned Setup: 12,118,016 bytes /
  `2E2D3F69114751FCBB14A5E972F994A619736FC54CB9C6CE9B524D787B50DDCD`.
- Records the four final public archives: installer ZIP 8,420,756 /
  `B9344F5E519A29BF8CE5F560805C0040EFA409ADADE047EE92FDA9234C47A502`;
  manual without UE4SS 319,886 /
  `3E0C4656F64C472A86C5C498EAD04C7E38A773C3E4F003B2E93DDCDB2226E261`;
  manual with UE4SS 8,404,482 /
  `2EFFFF81BA2CD936BD73F15F37A2C518D301A3E8E5558E138D327EC6AAFB293E`;
  range ZIP 3,120,870 /
  `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`
  bytes / SHA-256.
- Keeps in-process selector resolution, deployment, gameplay acceptance, and
  owner smoke testing pending; offline release success does not satisfy them.
- Builds a separate reflection-fix diagnostic candidate without replacing the
  prior release or local game installation. Candidate DLL: 912,896 bytes /
  `4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F`;
  unsigned Setup: 12,196,352 bytes /
  `4BEC3DCA4211E577A47E5DAC8CEBF106D4CB3FDED292CE06A164BA68438826C7`.
  Static/source/core/artifact gates and the current installer matrix pass
  10/10; exact in-game validation remains pending.
- Adds Radar-style installer state labels: an exact owned 1.3.0 installation
  exposes `Repair`, an older recognized release exposes `Upgrade`, and an
  absent installation exposes `Install`. Repair preserves `config.ini`,
  refreshes only installer-owned files and the selected owned range PAK, and
  retains strictly owned Uninstall. Unknown, modified, duplicate, or ambiguous
  same-name targets remain blocked with zero mutation.
- Recognizes both retained pre-compatibility and released-final 1.3.0 DLL hashes
  as exact historical owned payloads so the reflection-fix installer can repair
  either package without weakening ownership checks.
- Records the later runtime failure evidence from the installed 913,920-byte
  tuning DLL: dynamic selector resolution succeeded with zero selector faults,
  while five of ten actions timed out, four candidates remained quarantined
  until an Off/On cycle, and the interaction owner changed inside a stable
  `UWorld`.
- Keys pending, retry cooldown, and terminal quarantine to the exact returned
  interaction Component rather than the Actor. Cooldown/quarantine preflight
  now runs before live action mapping and subsystem resolution.
- Prevents the generic scheduler failure backoff from overwriting the 1500 ms
  settle deadline created by an interaction-owner change. Actor destruction is
  also accepted before probing a briefly surviving Component.
- Adds source and unit gates for the read-only action preflight, one 200 ms
  retry delay, two-attempt maximum, exact-Component quarantine, owner-settle
  ordering, and preflight-before-resolution ordering.
- Keeps the owner-accepted 1500 ms confirmation bound and two-attempt maximum,
  but reduces only the first-timeout retry cooldown from 500 ms to 200 ms. A
  normal confirmed pickup still releases pending immediately; this tuning only
  shortens recovery before the game selector may re-present the same exact
  Component. It does not add another retry or relax quarantine, selector,
  reflection, foreground, World, or ownership gates.
- Completes the exact offline release for the 200 ms retry-cooldown tuning.
  Native DLL: 925,696 bytes /
  `5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08`;
  unsigned Setup: 12,210,176 bytes /
  `3EF1D37E771123CC8DCC192EC18CB89F62EE5823F245E78E39B7BF3CA0570898`.
  Installer, manual-without-UE4SS, manual-with-UE4SS, and range ZIP SHA-256:
  `6B333A0ECDF9A7AF125E614DFC7A64BF03610BD83D1B5A4AB5FF080DE3EB1149`,
  `70AAFB9B4B49FEAF3ED698FBC32D37FC78F4690AD76598BEED2DFB5CEA1DB2DB`,
  `FF39DD1339CC9C2CF39F1578CDD70C0F606250150EF30EC83C55E2B647CD4014`,
  and `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.
  Static/core/native/installer/package gates pass; deployment and exact
  gameplay acceptance remain separate and were not performed.
- Corrects the 200 ms Setup ownership matrix so the immediately preceding
  final action-lifecycle DLL
  `38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
  is recognized as an exact owned 1.3.0 payload and exposes Repair. The match
  still requires the exact version, DLL hash, and Lua hash; modified or foreign
  same-name files remain blocked with zero mutation.
- Recognizes the exact installed 1.3.0 tuning DLL SHA-256
  `09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B`
  for in-place Repair. Ownership remains an exact version/plugin/Lua contract;
  mutated and unproven same-name payloads remain blocked.
- Records the now-historical 1500 ms / 500 ms final action-lifecycle release
  and its exact static/core/native/installer/package audit. Its DLL was 925,696
  bytes /
  `38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`;
  unsigned Setup: 12,210,176 /
  `F8279604CEB9AC900857DCF8EBB4F87E54B1DEF619E02ABE62BB899C508FA766`.
  Installer, manual-without-UE4SS, manual-with-UE4SS, and range ZIP SHA-256:
  `F41457CF7368CAB13E641089288061920FE527DCA01647F8684CFEC74A67F57E`,
  `4A719AB80607C69A34140D36CFB98F4DAEFA9BF121AC08A3F3371AF44DE30D1F`,
  `68D69D3DF2C59B10F8440CF80E4AA9CD9A958F85363E4E5B79D6832A13BDCDBF`,
  and `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.
  These hashes were superseded by the 200 ms retry-cooldown release above;
  deployment and gameplay acceptance were not performed for this historical
  package.

## 1.2.0 - Session lifecycle, installer state, and range expansion

- Disables Auto Pickup whenever the game returns to the main menu or
  initializes a new save or World; the configured toggle must be pressed again
  after a playable World loads.
- Generation-binds hotkey events to game-thread-published playable readiness,
  accepts toggles only while DragonSword is foreground, verifies the fresh Pawn
  World before each selector call, and revalidates foreground immediately
  before confirmation-state mutation and Enhanced Input injection.
- Adds explicit Install, Upgrade, and Uninstall states to the one-click
  installer. Exact-runtime upgrades preserve `config.ini` and use temporary
  rollback data instead of a persistent conversion backup.
- Restricts uninstall to the owned Auto Pickup Mod, its authoritative
  `mods.txt` entry, and owned approved range PAKs. UE4SS and unrelated Mods are
  preserved.
- Expands the mutually exclusive range choices to Original, 3x, 5x, 10x, 15x,
  and 20x.
- Retains the tested UE4SS v3.0.1 Beta #0 commit `1c1a1497`
  ExperimentalNested ABI.
- Keeps the game executable hash diagnostic-only; no game-build hash blocks
  installation or activation.
- Publishes three package classes only: one-click installer, manual without
  UE4SS, and manual with UE4SS.
- Completes the exact offline release: core tests and strict native build pass,
  the isolated installer matrix passes 10/10, and all three deterministic ZIPs
  pass exact-entry, checksum, provenance, and exclusion audits. Native DLL
  SHA-256: `9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6`;
  unsigned Setup SHA-256:
  `79F7D70C5F7D7A1F84AE4B75F415B362A4C30CEEB4A7B1AE5967D19053A9D323`.
- Preserves the accepted selector, mounted Rider, fish/drop support, treasure
  exclusion, and no-object-scan policy.
- Static, build, installer, and package verification remain distinct from an
  owner-observed gameplay smoke test of the exact 1.2.0 artifacts.

## 1.1.1 - Game-version gates removed

- Removes the exact game executable SHA-256 gate from both the installer and
  the native runtime.
- Keeps the game executable hash in diagnostics only; an older or newer game
  build no longer blocks installation, F9 activation, or automatic pickup.
- Retains the tested UE4SS hash as a post-load automation gate; package and
  installer controls remain the pre-load compatibility boundary.
- Preserves the 1.1.0 selector, mounted Rider, fish/drop support, treasure
  exclusion, conversion backups, Mod migration, and no-object-scan policy.
- Produces installer, manual without UE4SS, and manual with UE4SS packages.
- Offline native build, package verification, installer conversion, migration,
  unknown-game-hash, and backup-collision tests pass. Exact 1.1.1 gameplay
  acceptance remains owner-controlled.

## 1.1.0 - Single-compatible-runtime release

- Narrows the public native plugin to the tested UE4SS v3.0.1 Beta #0 commit
  `1c1a1497` ExperimentalNested ABI; StableRoot payloads are no longer included.
- Adds a confirmed conversion path for any existing or mixed UE4SS layout.
- Creates and SHA-256 verifies a complete Win64-relative UE4SS backup before
  removing the old active layout.
- Migrates existing unrelated Mods, their configuration, and `mods.txt` into
  the pinned runtime.
- Uses `_2`, `_3`, and later suffixes when a timestamped backup name already
  exists.
- Produces installer, manual without UE4SS, manual with UE4SS, and clean UE4SS
  runtime ZIPs.
- Preserves the accepted selector, mounted Rider, fish/drop support, semantic
  interaction binding, treasure exclusion, and no-object-scan policy.
- Treats the game executable SHA-256 as diagnostic evidence instead of a hard
  activation gate, so older or newer game builds are not forced into passive
  mode solely because their hash is not yet listed. The tested UE4SS native ABI
  fingerprint remains a fail-closed post-load automation gate.
- Offline compile, package, conversion, migration, and collision tests pass;
  exact 1.1.0 gameplay acceptance remains owner-controlled.

## 1.0.1 - Superseded internal candidate

- Reads the game's saved semantic `INTERACT` binding once whenever AutoPickup
  is enabled, so the live action resolver no longer assumes physical F.
- Adds `interaction_key=AUTO` as the recommended primary mode and a separate
  `interaction_key_fallback=F`. The installer exposes the concrete fallback;
  it is consumed only when semantic binding detection fails.
- Preserves the accepted selector, mounted Rider, fish/drop support, treasure
  exclusion, no-object-scan policy, and existing scheduling. Binding resolution
  adds no timer, worker, disabled-state query, or cross-World object cache.
- Adds resolution diagnostics for mode, configured key, resolved action,
  current active binding keys, ignored mappings, and ambiguity.
- Core tests and both pinned native ABI builds pass. Runtime keyboard rebind and
  controller acceptance remain owner-controlled and are not implied by builds.
- Replaces the installer checkbox with mutually exclusive Original, 3x, 5x,
  and 10x native interaction-range choices. Range expansion remains a separate
  PAK resource and is Off by default; 10x is documented as the aggressive
  option.
- Defines three release downloads: a one-click installer ZIP, a manual package
  for the exact pinned experimental-nested UE4SS layout, and a standalone ZIP
  containing the 3x, 5x, and 10x PAK alternatives.
- Documents the exact compatibility target as DragonSword Awakening 1.0.10 on
  Steam build 24831799. Unknown game or UE4SS binaries continue to fail closed.
- Records the three current public feedback topics. E rebinding is implemented
  in source but still requires exact-artifact runtime acceptance; controller
  behavior and Treasure Radar Overlay coexistence remain `NOT_VALIDATED`.

## 1.0.0 - First public release candidate

- Starts automatic pickup Off after every game launch; the configured toggle
  key enables or disables it, with F9 as the public default.
- Accepts documented function, letter, number, numpad, navigation, and Space
  key names while rejecting unsupported or chord syntax.
- Preserves the owner-accepted 1.6.10 no-range selector/action route, mounted
  Rider support, fish, drops, treasure exclusion, World settling, and bounded
  performance behavior.
- Adds separate stable-root and experimental-nested UE4SS ABI builds.
- Adds an installer that selects the exact game executable, detects the UE4SS
  layout, bootstraps verified official stable UE4SS when absent, creates
  rollback backups, and preserves unrelated Mod settings.
- Makes `mods.txt` authoritative, normalizes AutoPickup to one entry, and
  removes the legacy `enabled.txt` bypass.
- Offers the independent verified 5x native-range PAK as an optional installer
  component, Off by default. The native DLL still performs no range mutation.
- Keeps public Debug Off; the developer install may use a config-only Debug
  override. Exact-artifact gameplay acceptance remains owner-controlled.
- Excludes `ZeroKarya_PartySwitch` and every other third-party Mod from the
  AutoPickup source manifest, installer, and public archive.
- Records successful dual-ABI builds and the offline installer matrix while
  keeping exact-artifact gameplay status `NOT_VALIDATED`.

## 1.8.0 - Internal stable no-range candidate

- Restored the exact owner-accepted 1.7.0 no-range source body, which preserves
  the 1.6.10 selector, F9, mounted Rider, fish, drop-item, treasure-exclusion,
  confirmation, lifecycle, and performance behavior.
- Removed all experimental 1.7.x runtime range machinery: overlap hooks,
  proxy/component construction, collision mutations, proof/restore state,
  multiplier configuration, and related recurring diagnostics.
- Kept range as an independent optional pure-resource PAK. Auto Pickup does not
  bundle or stack a multiplier and works at either original or PAK-expanded
  native range.
- Pinned build, core tests, no-range source gates, and exact four-file package
  pass. Exact-artifact gameplay acceptance remains pending.

## 1.7.5 - Plugin-owned overlap-proxy 3x canary

- Retained the exact 1.7.3 Begin/End four-identity proof. While Begin is live, the existing selector's actor/`DInteractableComponent` pair must pass the complete supported-target, `InteractableValue=2`, Outer, and World predicate and correlate by receiver plus Actor; the actual Begin `OtherComp` supplies bidirectional channel proof. Only scalar evidence survives exact End, so later proxy application does not resolve target weak identities and NPC/unrelated-trigger observations fail closed.
- Added one deferred, plugin-owned, non-root `UCapsuleComponent` configured inert before registration, `QueryOnly`, and exact 3x the proven root capsule's unscaled radius and half-height. It copies the root object type and every root/main channel already set to `ECR_Overlap`; all other responses remain `Ignore` and none may `Block`.
- Added a single-source receiver delegate handoff: bind `OnBeginOverlap` and `OnEndOverlap` to the inert proxy, remove and verify only those two receiver/function entries on the main capsule, then enable the proxy while preserving unrelated delegates.
- Explicitly prohibited permanent dual binding because the receiver keys candidates by interaction Actor plus target `OverlapComponent`, not player producer. Root and proxy deliver the same pair, so duplicate Begin does not create per-producer ownership and either End can remove it. Activation has no synthetic migration; cleanup retains proxy-End/root-Begin reconciliation.
- Added transactional rollback and restoration for F9 Off, same-World receiver changes, hard faults, and normal unload: snapshot members, disable the still-bound proxy to produce/supplement End, unbind it, restore main delegates, replay current-main Begin, prevalidate lifecycle identity, then issue `K2_DestroyComponent` as the final proxy operation with no later proxy dereference. This records a destruction command, not synchronous destruction proof. World travel discards weak state without outgoing-World UObject access.
- Preserved the no-UObject-scan policy and the accepted 1.6.10 selector, target policy, mounted Rider, fish, and live physical-F Enhanced Input action. Full overlap-channel copying preserves other normal manual F interactions while the automatic selector continues to exclude treasure boxes.
- Kept public diagnostics off. The pinned `515584`-byte DLL (`ECDE66501E490FD7954DDE6CFFAC6D8CF42CBCA89BD4B04A4FE02866A932DA0A`), deterministic four-file package, and `230855`-byte archive (`15BAE11D78A5A4F5273B8C71AD24AE9ED7E1E48999B78F60114954CA242F2157`) are verified and deployed. Only the installed config uses a Debug override; owner runtime, gameplay, stability, and performance acceptance remain pending.

## 1.7.4 - Rejected dedicated interaction-box hypothesis

- Proposed enabling `bCreateInteractCollision`, changing `BoxExtent`, and invoking `Server_RegenerateInteractComponent` to request a dedicated UBox.
- Rejected the route before build after disassembly proved the reflected regeneration chain changes interaction state and does not access the collision-creation fields.
- Identified RVA `0x42C2E00` as a lifecycle override that is unsafe to call manually on an already registered component.
- Restored the exact pre-experiment 1.7.3 source. No 1.7.4 DLL, package, deployment, or gameplay claim exists.

## 1.7.3 - Rejected receiver-shape 2x canary

- Rejected and removed the 1.7.2 target seed/constructor/retry/lease subsystem after 332,258 scanned slots and about 19.129 seconds produced zero applies and leases.
- Added exact Begin/End overlap hooks that capture weak identities only and require a matching four-identity pair before adoption.
- Added strict current receiver, World, owner Outer, QueryOnly, overlap-event, root, main-capsule, and Box/Sphere/Capsule guards.
- Added exact unscaled 2x shape writes with overlap updates, readback, reversible weak leases, and no outgoing-World UObject access.
- Added pre-write provisional restoration evidence, 500 ms restore retry, and fail-closed selector suppression after any shape-write fault.
- Added game-thread-only overlap capture and in-flight callback lifetime leases to prevent mixed ring records and unload use-after-free.
- Preserved the accepted selector and live physical-F Enhanced Input automatic action.
- Runtime captured 172 Begin and 172 End observations with 168 matching pairs, but all 105 adoption attempts stopped at the RootComponent guard; applies and leases remained zero.

## 1.7.2 - Target-collision 2x range canary

- Removed the ineffective player/Rider `InteractCollision` lease, its `range_multiplier` setting, transition state machine, counters, tests, and release assertions.
- Added one bounded incremental seed pass for already-loaded supported target components.
- Added an event-driven post-construction queue for newly created supported targets; stable operation performs no full UObject scan.
- Expands and verifies the target component's own `BoxExtent` and live `InteractCollision` exactly once per weak identity.
- Restores owned extents on F9 Off, guarded disable, and normal unload; World travel performs no outgoing-World UObject write.
- Preserved the accepted native selector, mounted Rider ownership, fish policy, treasure-box exclusion, and live physical-`F` Enhanced Input action.

## 1.7.1 - Reversible 2x interaction-range canary

- Doubled all three axes of the live player or mounted Rider `InteractCollision` extent.
- Applied the range lease only once per receiver/collision identity and verified the live readback.
- Added transactional, ownership-checked, readback-verified restoration on F9 Off, receiver change, and guarded disable, with a 500 ms disabled retry only after a failed restore.
- Restricted every range mutation to the captured game thread, removed unload-thread UObject access, and mirrored lease state atomically for cross-thread diagnostics.
- Replaced outgoing-World weak leases with scalar transition guards, preventing persistent/seamless receivers from treating an existing 2x extent as a new baseline.
- Added a post-settle restore-only cleanup state for F9 Off, guarded disable, and normal unload; it resolves only the fresh current receiver, performs no selector/input work, and never expands a partially restored collision before shrinking it.
- Preserved queued F9 key/toggle parity across World reset, so an Off request made during travel cannot be silently discarded.
- Promoted weak/object restore faults to scalar ownership guards, blocking re-enable and preventing a half-restored 2x field from becoming a later 4x baseline.
- Distinguished same-World restore-fault guards from World-transition guards: temporary Pawn/Rider identity changes cannot discard fault ownership, and a World transition restores any single surviving receiver/collision field before releasing the guard.
- Added ownership-aware restoration for a live receiver or collision whose weak peer expired; only two confirmed-expired identities may be discarded without a write.
- Made failed same-identity transition reattachment transactional: any failed 2x rewrite/readback immediately rolls back through the normal restore state machine and disables automation instead of leaving a misleading active lease.
- Retained the lease when an ownership conflict still contains either saved expanded value, blocking re-enable instead of forgetting and re-doubling a half-owned range.
- Normal UE4SS unload always performs a bounded game-thread cleanup handshake before callback invalidation, covering a range apply racing the unload request; process-shutdown abandonment publishes atomics only.
- Kept the accepted selector, target whitelist, physical-F action injection, chest exclusion, and no-UObject-scan policy unchanged.
- Corrected the selector third-argument declaration to the proven `ExcludeActor` pointer type while continuing to pass `nullptr`.
- Added bounded range configuration, pure scaling and restoration-ownership tests, and interval apply/restore performance attribution.

## 1.7.0 - Native Auto Pickup release candidate

- Preserved the owner-tested 1.6.10 runtime logic without changing selector cadence, target policy, input injection, lifecycle, or confirmation behavior.
- Changed the public package default to `debug_logging=false`; local diagnostic installs can opt in without changing the distributable.
- Replaced diagnostic/owner-specific runtime identity with a public release identity.
- Added a release archive builder, independent built-artifact verification, rollback-safe local deployment, package/install config separation, and sidecar SHA-256 manifest.
- Reorganized the README, project context, acceptance matrix, release procedure, architecture, evidence, and failure model around the current implementation.

## 1.6.10 - Fish policy and performance diagnostics

- Added selector-returned Animal type `5` for fish and explicitly excluded TreasureBox type `4`.
- Added interval-only stage timings and batched Debug-file writes.
- Owner testing confirmed mounted trout pickup and low scan cost with no selector fault or object scan.

## 1.6.7 - Accepted physical-F action route

- Replaced guessed action resolution with allocator-aware reflected traversal of the current Enhanced Input mappings.
- Owner testing visibly collected two independent NormalGather targets without manual F input.

Earlier rejected experiments and their evidence are preserved in `docs/ATTEMPT_LEDGER.md`.
