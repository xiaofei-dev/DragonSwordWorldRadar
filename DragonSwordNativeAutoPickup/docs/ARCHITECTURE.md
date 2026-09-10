# Architecture

## Runtime path

The UE4SS native adapter starts Off. The released ExperimentalNested adapter
parses the configured toggle key (`F9` by default) and publishes only a scalar
toggle request outside the gameplay callback. It uses UE4SS keydown plus
EngineTick callbacks. The first keydown latches the physical press; operating-
system repeat events cannot publish another transition until a Windows key-
state release observation clears the latch. Release polling touches no UObject.
Hotkey ingress does not touch gameplay UObjects and is accepted only while the
DragonSword game window is foreground and the game thread has published the
current session generation as playable.

The same game-thread tick publishes completed toggle outcomes to an isolated
native UMG status-card renderer. The renderer owns only weak widget handles and
a pure display timeline. It runs after pickup work, is excluded from pickup
timing and decisions, never owns a hook or input path, and marks its entire
widget tree hit-test-invisible. World travel clears all widget handles. ABI or
runtime UI failure disables only the renderer and records
`pickup_unaffected=1`; it cannot change automation state.

The renderer builds one 360 x 82 reference-unit card from native `Border` and
`TextBlock` widgets. Its deep-blue outer glass is 72% opaque, with a 32% opaque
inner layer, 28% shadow, 13% top highlight, and state-colored glow/rule layers.
The state accent is gold, mint, slate, or soft red. The pure timeline retains
the existing 120 ms fade-in, 260 ms fade-out, and bounded message lifetimes but
applies smoothstep easing. The host combines the eased opacity with an 8-unit
vertical reveal and a 98.5%-to-100% scale. These values are presentation-only
and cannot feed back into the automation state machine.

Before any gameplay reflection is used, the adapter verifies the exact nested
UE4SS hash and loaded module path. It then parses the loaded game PE32+ image
and its executable `.text` section.

UE4SS may publish the reflected `DInteractableComponent` class shortly before
its class default object becomes available. A bootstrap EngineTick therefore
permits only this exact readiness condition to remain Pending, probing every
250 ms for no longer than 30 seconds. Pending and Failed states return before
all operational pickup work. When the CDO appears, initialization reruns the
complete reflection contract, both selector anchors, observer registration,
and callback gates before publishing `Ready`. No other reflection or machine-
code mismatch is treated as transient.

The Server path treats the reflected `Server_RunInteractV2` exec thunk as a
virtual-dispatch anchor. It requires one unique virtual slot, reads that slot
from the interactable CDO, follows a bounded direct-jump chain, and bounds the
resulting native implementation through the x64 `.pdata` runtime-function table
and bounded `CHAININFO`. That implementation must expose one local rel32
selector call whose receiver, output-pair setup, and post-call writes agree with
the reflected `ExecuteTargetObject` and `ExecuteTargetComponent` offsets.

The UI path is deliberately different. Reflected `SetInteractUIV2::GetFuncPtr`
is a direct rel32 native wrapper, not a virtual-dispatch thunk. Its complete exec
wrapper is bounded through `.pdata` and `CHAININFO` and must contain exactly one
terminal `E8 rel32` call to the native SetInteractUI implementation. That
implementation is independently runtime-function bounded and must expose one
selector call matching the UI receiver/output structural contract.

Both paths must resolve the same unique executable selector address. Instruction
recognition occurs only at real decoded boundaries. Only after consensus is one
scalar selector capability retained for the process. The policy identifier is
`runtime_reflection_dual_caller_rel32_consensus_fail_closed`. There is no fixed
selector RVA, game-hash address table, or fallback address. Missing, malformed,
ambiguous, inconsistent, or non-executable evidence keeps automation Off;
a guarded selector fault invalidates the capability for the process.

When enabled and due, the adapter resolves the current player context, prefers
the mounted `Pawn.Rider` interaction receiver when valid, calls the resolved
native selector once, validates the returned actor/component pair, and injects
one vector action through
`EnhancedInputSubsystemInterface:InjectInputVectorForAction`.
The context must retain the exact LocalPlayer/controller relationship,
`controller.Player == LocalPlayer`, a non-null current Pawn,
`pawn.Controller == controller`, and a fresh same-World identity. An expected
character and a controller-bound alternate mounted Pawn are both supported;
neither is cached across frames.
Immediately before the selector, the fresh Pawn World weak identity must match
the World identity captured when automation was enabled. Immediately before
any confirmation-state mutation or injection, foreground ownership is checked
again.

This resolver can survive address relocation while the reflected Server virtual
path, reflected UI direct-wrapper path, runtime-function metadata, and required
machine-code contracts remain compatible. It does not promise compatibility
with arbitrary recompiles. Structural drift fails closed instead of calling a
historical address.

Only one automatic injection may be in flight globally. Before injection, the
exact candidate weak identity, raw interaction-receiver address, action token,
and scalar World/session identity define the armed record, so synchronous
re-entry cannot create a second action. No later candidate may invoke an action
while that record remains armed.

The adapter registers one post observer on the exact reflected
`Server_RunInteractV2` UFunction. The record is armed before injection and
remains correlatable after the injection call returns until matching dispatch,
existing exact weak/state confirmation, timeout, or context reset. While that
record is armed, the observer compares `context.Context` with the raw receiver
address and, on a match, publishes only the action token through an atomic
marker. It performs no logging, formatting, reflection, UObject dereference,
state-machine mutation, or game call. EngineTick consumes the marker, clears
the global in-flight slot, emits `PICKUP_DISPATCH_OBSERVED`, and gives that exact
Component a 750 ms first-dispatch re-entry delay. Other selector-presented candidates can then
advance. The observation proves only that the injected action reached the
game's interaction dispatch; it does not prove that the selector-returned target
was chosen or collected. Logs therefore carry `target_match_unproven=1` and
`pickup_success_claim=0`.

The unreleased 2026-09-08 candidate checks existing exact Actor/Component
invalidation or exact Component state-change evidence before consuming dispatch.
When both are present in one poll, exact confirmation clears attempt history
and the already elapsed dispatch scan deadline is preserved. No new game call
or discovery method is added by this ordering change.

An unconfirmed dispatch now preserves its attempt ordinal rather than resetting
it. The first dispatch retains one retry opportunity for 1500 ms after the
750 ms re-entry delay. Re-presenting that same exact Component during the
opportunity is attempt two, even when dispatch and timeout outcomes alternate.
When neither matching dispatch nor exact confirmation arrives, the 750 ms
fallback window expires.
The game must present the same exact identity again after a 200 ms delay before
one retry is admitted. A second unconfirmed result applies a 1500 ms self-
expiring Component backoff. There is no activation-long timeout quarantine and
no 500 ms quarantine scan loop. Context reset still clears pending and attempt
records. Neither timeout writes a global scan deadline: after clearing the
in-flight record, that same EngineTick may process a different selector result
while the exact Component record remains cooled.

The follow-on frame-paced candidate removes the 25 ms enabled pulse/active
scan/post-confirmation delay, not the one-global-pending policy. A scope-bound
atomic EngineTick entry guard rejects nested ticks during ProcessEvent. There
are no batches or catch-up loops: at most one input per outer EngineTick.
Empty scans and capacity pressure retain 33 ms spacing; per-Component retry
and world/owner settle rules remain unchanged.

At 128 protected records, an unknown candidate returns `CapacityWait` before
action mapping/subsystem resolution. Existing record owners can retry. Both
read-only admission and mutation recognize expiry; no live retry record is
evicted and saturation does not permanently disable the Mod.

Context/selector debug snapshots are bounded scalars, formatted only after
the decision. Post-injection diagnostic exceptions cannot rewrite the completed
outcome; actual guarded gameplay faults still fail closed. Debug remains off
in public defaults and introduces no extra location/UObject query.

These candidates repair retry accounting and scheduling. They do not clear native
interaction lists or prove that an already stuck manual pickup is restored.

The selector result must prove `InteractableValue=2`, matching component Outer
and World, and one closed target category: NormalGather 2, Animal 5, or
class-proven DropItemActor 7. TreasureBox 4 is always rejected.

The default resolver reads the game's saved semantic `INTERACT` record
(`ActionInputType=91`) once whenever AutoPickup is enabled and caches the
resolved keyboard `FKey` as scalar data for that enabled session. It then
selects the current non-ignored live mapping for that exact key. If the saved
record is missing, ambiguous, chorded, unsupported, or unreadable, the resolver
uses the configured concrete `interaction_key_fallback` for that session. A
configured `interaction_key` other than `AUTO` bypasses semantic detection and
acts as a troubleshooting override. Every path rejects missing, inactive, or
conflicting live mappings.

Both Enhanced Input UFunctions are accepted only when every reflected parameter
has its exact allowed property class, array dimension, element size, and bounded
offset. UObject and UClass parameters are written through the pinned SDK
`SetObjectPropertyValue` accessor and the subsystem return is read through
`GetObjectPropertyValue`; weak, soft, interface, or other object-storage
variants fail closed. The pending action and raw receiver token are established
before the injection ProcessEvent. After it returns, the injection path performs
no gameplay UObject read; the post observer only compares raw addresses and
publishes an atomic token for later EngineTick consumption.

## Lifecycle and performance

Disabled state performs no game selector invocations and no steady-state player-chain
queries. Enabled state uses bounded 25 ms engine/active scheduling, a 33 ms
idle cadence, and a 25 ms post-invocation due. A dispatch marker normally
arrives on a later real EngineTick after that due is already satisfied, so its
consumption adds no second 25 ms delay and the same tick may continue scanning.
The scheduler also retains bounded failure backoff, one
global in-flight action, exact Server dispatch observation, one bounded
selector-represented retry, expiring per-Component re-entry/backoff records,
World-settle delay, and interval-aggregated optional diagnostics. Routine User
logging is one invocation plus one terminal result. Debug context, selector,
and deferred-reason attribution is change-only with independent per-activation
caps; one scalar `ACTION_TRACE` accompanies each bounded invocation. Periodic
performance records include emitted/suppressed attribution and logger queue,
drop, and failure counters. No worker thread is created.

The main-menu/save/World initialization callback forces automatic pickup Off,
clears interaction binding and cross-World session state, and requires a new
toggle after a playable World loads. Reset advances a session generation and
clears playable readiness, so a key event during loading cannot be carried into
the next playable session. Stable viewport World identity is checked before
confirmation evidence is polled. World transitions retain no cross-World
gameplay UObject. The adapter resolves fresh context after settling and fails
closed on guarded faults. Input action resolution reuses the existing
per-candidate mapping traversal; it adds no UObject scan, timer, worker, or
disabled-state work. The key name, fingerprint, timing values, and counters are
scalar or immutable data only.

## Installation architecture

Version 1.3.1 publishes one native ABI only: UE4SS v3.0.1 Beta #0 commit
`1c1a1497` in the ExperimentalNested layout. When that exact runtime is absent,
the installer asks before converting an existing or mixed UE4SS layout. It
creates and verifies a complete Win64-relative backup, removes the old active
layout, installs the pinned runtime, and migrates unrelated Mods and settings.
If UE4SS is absent, the same pinned ExperimentalNested runtime is installed.

The package and installer provide the pre-load compatibility boundary by
shipping or requiring that exact nested runtime layout. After plugin code is
loaded, the adapter compares both the nested `UE4SS.dll` hash and the actual
loaded module path before enabling automation. That post-load passive/Off check
does not claim to prevent an ABI crash that occurs before the plugin can run.

The active Mods root is `Win64/ue4ss/Mods`. StableRoot plugin payloads are not
part of the 1.3.1 release.

`mods.txt` is the only load authority. The installer preserves unrelated lines,
normalizes AutoPickup to one entry, and removes the legacy `enabled.txt` bypass.

Installation state is ownership-bound. An absent Mod exposes Install; one
recognized owned Mod exposes Upgrade and Uninstall. Exact-runtime Upgrade
preserves `config.ini` and uses temporary rollback data without retaining a
persistent backup. Uninstall removes only owned Auto Pickup files, its
authoritative `mods.txt` entry, and approved owned range PAKs. Unknown same-name
Mod content fails closed. Exact supported range-PAK filenames and the legacy
canary filename are owned by filename; embedded replacement bytes remain
hash-verified after writing.

## Offline release evidence boundary

The preceding 750 ms fallback-window / 200 ms retry-delay 1.3.0 native DLL is
919,552 bytes with
SHA-256
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
the corrected unsigned Setup is 13,001,728 bytes with SHA-256
`2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`.
Static, source, core, built-artifact, installer 10/10, deterministic ZIP,
exact-entry, and checksum gates passed for the final four archives recorded in
`docs/EVIDENCE.md`. Those hashes predate the dispatch-observer repair and prove
only the preceding package provenance. The observer candidate remains
`RUNTIME_PENDING`; its exact build identity, deployment, dispatch correlation,
gameplay, performance, and owner smoke-test acceptance remain pending.

Repair remains ownership-bound. The immediately preceding `38DA6C...` DLL is
accepted only with its exact 1.3.0 version and Lua hash; changed or foreign
same-name payloads still fail closed with zero mutation.

## Explicitly absent

The active adapter performs no UObject/Actor enumeration, overlap hook,
collision proxy construction, root/physics/hit collision resize, target
collision polling, direct pickup RPC, `SendInput`, continuous injection, or
unbounded automatic retry. Its one exact UFunction post observer is not a
general `ProcessEvent` hook and owns no gameplay work. It does not alter general
interaction-range producers.

The installer-offered 3x, 5x, 10x, 15x, and 20x range PAK alternatives remain a
separate resource layer. Each variant contains 50 reviewed gather/animal
capsule packages and 19 class-proven type-7 drop packages. The drop patch adds
only `SphereOverlapComp.RelativeScale3D`; physics and hit components are
protected by the structured asset gate. Native range multiplication is
compile-time disabled, so the PAK is the sole range owner. Gather/animal targets
use the selected variant multiplier; short-lived drop targets use
`min(selected multiplier, 10x)`. Treasure/type-4 packages remain excluded.
