# Threat and Failure Model

## Primary failures

- stale or cross-World gameplay UObject use;
- wrong receiver while mounted;
- unsupported or treasure target selection;
- missing or ambiguous interaction action mapping;
- repeated action against the same live target;
- a second target replacing or extending an unresolved pending action;
- an unobserved dispatch retrying indefinitely or monopolizing the live
  interaction action used by manual F;
- a broad or re-entrant UFunction observer doing reflection, logging, UObject
  work, or state mutation inside game dispatch;
- treating game dispatch as proof that one exact selector target was collected;
- operating-system F9 key repeat creating more than one state transition for a
  physical press;
- enabled state leaking across the main menu, save load, or World initialization;
- action attempts while the game is not foreground;
- a historical fixed selector address being called after game-code relocation;
- malformed PE32+ or x64 runtime-function metadata;
- missing, ambiguous, or inconsistent selector evidence after a game recompile;
- selector or reflection faults during initialization or transition;
- a valid reflected class appearing before its class default object and causing
  a permanent one-shot startup rejection;
- per-scan logging, object enumeration, workers, or collision lifecycle work.

## Active mitigations

The adapter resolves fresh current Pawn/Rider context, validates component Outer
and World, uses a closed type policy with treasure type 4 excluded, resolves
the exact live interaction action independently of its physical binding (or an
exact manually configured Unreal key), rejects conflicting action mappings,
permits one global in-flight action and registers a post observer only for the
exact reflected `Server_RunInteractV2` UFunction. While an injection is armed,
the observer compares the raw receiver pointer and publishes one atomic action
token; it does not log, reflect, read a UObject, call the game, or mutate the
state machine. EngineTick consumes the marker, releases the global slot, records
`target_match_unproven=1` and `pickup_success_claim=0`, and applies a 750 ms
same-Component re-entry delay. The armed record is created before injection and
persists after the injection call returns until matching dispatch, existing
exact weak/state confirmation, timeout, or reset. If neither dispatch nor exact
confirmation occurs, only one selector-represented retry follows after 200 ms;
a second no-evidence result applies a 1500 ms
self-expiring Component backoff instead of activation-long quarantine. The
adapter resets action state when the interaction
owner changes inside a stable `UWorld`, applies world-settle backoff, latches F9 until
physical release,
disables on guarded hard faults, forces Off on main-menu/save/World
initialization, and writes only interval-aggregated optional diagnostics. A
reset clears playable readiness and advances the session generation; key events
are accepted only after the game thread marks that generation playable and only
while the DragonSword window is foreground. Every active scan compares the
fresh Pawn World identity with the enabled World before calling the selector.
Foreground is checked again after action/subsystem resolution and before any
confirmation-state mutation or Enhanced Input injection. Normal and mounted
paths require exact LocalPlayer/controller/current-Pawn identity, including
bidirectional Pawn/controller ownership. Routine action logs are compact and
detailed context, selector, and deferred attribution remains change-only,
per-activation capped, scalar-only, and Debug-only. One action trace is emitted
per bounded invocation, while periodic aggregates expose suppression and logger
queue/drop/failure counters.

The exact UE4SS hash and loaded-path gate runs before gameplay reflection. The
only retryable startup observation is a present `DInteractableComponent` class
with its CDO not yet ready while every other required reflected object and
property is present. A bootstrap EngineTick retries at 250 ms for at most 30
seconds and returns before pickup work until initialization is `Ready`. Success
reruns the complete contract; timeout or any other mismatch remains fail-
closed. The
selector resolver then validates the loaded PE32+ image and executable `.text`,
bounds native code through x64 `.pdata` and bounded `CHAININFO`, and decodes only
real instruction boundaries. The reflected `Server_RunInteractV2` exec thunk
must yield one virtual slot whose interactable-CDO implementation contains a
valid Server selector call. The complete reflected `SetInteractUIV2` exec
wrapper must yield one unique terminal `E8 rel32` implementation call; that
implementation must contain a valid UI selector call. The asymmetric paths must
resolve the same selector address. The game hash never selects an address, and
no fixed-RVA fallback exists. Missing, malformed, ambiguous, inconsistent, or
non-executable evidence reports `SELECTOR_UNAVAILABLE` and leaves automation
Off. A guarded native fault reports `SELECTOR_FAULTED`, clears the process
capability, and forces Off.

This fail-closed policy supports relocation only while the required reflected,
runtime-function, and machine-code contracts remain compatible. It converts
unknown recompile drift into unavailable automation rather than a call to a
historical address; it does not prove arbitrary future-build compatibility.

Reflected action parameters use an exact property-class and bounded-layout
whitelist. Object and class values are set and read through the pinned SDK
property accessors; weak, soft, interface, or unknown storage variants are not
treated as raw pointers. Any metadata drift disables the action path before
ProcessEvent.

The adapter intentionally has no overlap hooks, collision component ownership,
restoration lease, UObject scan, direct interaction RPC, `SendInput`, continuous
injection, unbounded timeout retry, or worker. The five range PAKs author the
exact inherited `DropItemActor.SphereOverlapComp.RelativeScale3D` in 19 reviewed
type-7 child packages. Native runtime range multiplication is compile-time
disabled, so reflection drift cannot double-apply the selected multiplier. Root,
physics, and hit collision components remain untouched.

## Status-card isolation

The optional native status card consumes completed lifecycle outcomes only. It
does not own a hook, key callback, selector, candidate, action, retry, or
confirmation state. Its tree is hit-test-invisible and does not call any input
mode or cursor API. UI reflection is validated independently; construction and
updates are guarded. A missing schema, stale weak widget, controller replacement,
or UI exception disables and clears the renderer without changing automation.
World travel drops every weak UI handle. Thus the residual status-card risk is
visual failure or small game-thread presentation overhead, not a new pickup
authority. Gameplay observation is still required to validate appearance and
runtime cost for the exact DLL.

## Offline release evidence boundary

The preceding 750 ms fallback-window / 200 ms retry-delay 1.3.0 DLL is
919,552 bytes with SHA-256
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`.
The corrected unsigned Setup is 13,001,728 bytes with SHA-256
`2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`.
Static, source, core, built-artifact, installer 10/10, deterministic ZIP,
exact-entry, and checksum gates passed for all four final archives. That evidence
closes offline artifact-integrity and packaging threats only for that preceding
artifact. It predates the dispatch-observer repair. The new source candidate is
`RUNTIME_PENDING`; exact build identity, in-process observer behavior,
deployment, gameplay, performance, and owner smoke-test threats remain open.

The Setup recognizes the immediately preceding owned `38DA6C...` DLL only
through its exact version/DLL/Lua tuple. This permits Repair without turning
unknown-file protection into destructive replacement; modified and foreign
same-name files remain blocked with zero mutation.

Ownership schema 2 removes the recurring manual-predecessor allowlist failure.
The installer-created record binds a stable product ID to the exact immutable
DLL, Lua, and notices hashes. Later Setup builds verify those recorded hashes
and the closed file/directory layout. A changed immutable file fails closed;
configuration and owned runtime logs remain intentionally mutable. Legacy
schema-1 records still require a known tuple for their one-time migration.

## Residual performance evidence boundary

The reviewed 1.2.0 diagnostic session measured about 20 microseconds average
per scan and 2 microseconds average for the injection call, so it does not
prove that the selector loop caused sustained CPU stutter. It did prove 250
injections, including 61 for one identity. Native timing ends at the injection
call and therefore does not include downstream game interaction work. Deferred
log-file flush I/O is also outside the EngineTick timing aggregate. Version
1.3.0 bounds both causes through single-in-flight policy and compact logs. The
observer repair should release that slot on matching game dispatch, but exact
runtime behavior and performance remain unaccepted until measured.

## Independent PAK risk

The installer-offered 3x, 5x, 10x, 15x, and 20x resource PAK alternatives
enlarge authored interaction volumes. Their risks are increased overlap density
and broader native prompt competition, not native adapter lifecycle work. Only
one variant may be installed at a time. The 15x and 20x options are the most
aggressive and have the highest prompt-competition risk. Each must be tested
separately with AutoPickup Off first (F9 by default unless customized),
including dense areas, one normal gather, one conch/shellfish, one fish,
treasure behavior, World travel, and clean exit. Selecting Original in Setup,
or removing the selected PAK and restarting, is the rollback.

## Third-party boundary

Local compatibility work for `ZeroKarya_PartySwitch` is outside the AutoPickup
runtime, installer, and release transaction. Redistributing that third-party
Mod would create an ownership and provenance failure; release inspection must
reject it if present.

No static result proves visible pickup, prompt range, stability, or performance
inside the game.
