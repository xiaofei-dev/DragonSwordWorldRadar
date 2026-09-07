# DragonSword Native Auto Pickup

Native UE4SS automatic pickup for DragonSword: Awakening.

Current complete package set: [Release status](docs/RELEASE_STATUS.md).
Public-page copy: [Nexus publishing index](assets/nexus/README.md).

## Project status

The selector and Enhanced Input pickup route is functionally established for
on-foot and mounted play, including fish and drops. Deployed version `1.2.0`
also exposed a correctness problem under sustained automation: one runtime
session recorded 250 injections, 17 exact weak-identity confirmations, 30
timeouts, and 38 confirmation supersedes, with one target receiving 61
injections. The same sessions showed that the 500 ms F9 debounce could accept a
later operating-system repeat and re-enable automation after an apparent
disable.

Version `1.3.1` is the current patch release. It preserves the accepted
game-owned selector semantics and Enhanced Input route, but no longer calls a
fixed selector RVA. The process resolves one selector capability from two
independent reflected code paths before automation can be enabled. It permits
only one globally in-flight automatic injection keyed to the exact returned
interaction Component. A narrowly scoped post observer for the exact reflected
`Server_RunInteractV2` UFunction is armed before that injection and remains
correlatable after the injection call returns until matching dispatch, existing
exact confirmation, timeout, or context reset. The observer
compares the raw receiver address and publishes one atomic marker; it performs
no logging, reflection, UObject reads, or game calls. EngineTick consumes the
marker, releases the global slot, and applies a 750 ms same-Component re-entry
delay. This proves that the game reached its interaction dispatch, not that the
selector-returned target was picked up, so diagnostics explicitly record
`target_match_unproven=1` and `pickup_success_claim=0`.

On game build `25076183`, one startup session exposed a readiness race where
the reflected interactable class was present before its class default object.
Version 1.3.1 now waits fail-closed and retries only that dependency every 250
ms for at most 30 seconds. `READY`, F9 handling, scanning, and injection remain
unavailable until the original complete reflection and dual-selector contracts
pass; unrelated contract failures are not retried or relaxed.

If no matching dispatch arrives, the conservative fallback window remains
750 ms. One retry may follow after 200 ms; a second no-dispatch result applies a
1500 ms self-expiring Component backoff. It does not quarantine a target for the
whole F9 activation. F9 remains a true physical press edge, so key repeat cannot
create another transition until release. The exact 1.3.1 offline source, build,
installer, and package gates pass; deployment and gameplay acceptance remain
`RUNTIME_PENDING` and are separate evidence classes.

## Behavior

- Resolves the native interaction selector once per process from the reflected
  `Server_RunInteractV2` virtual path and the reflected `SetInteractUIV2`
  direct-wrapper path, then uses one bounded selector call per due scan.
- Uses a bounded 25 ms engine/active cadence, retains a 33 ms idle cadence,
  retains the 25 ms post-invocation due, and allows only one globally in-flight
  automatic interaction. The next real EngineTick normally satisfies that due;
  consuming a dispatch marker does not add another 25 ms wait and the same tick
  may continue scanning.
- Observes the exact `Server_RunInteractV2` post-dispatch through a callback
  restricted to raw receiver comparison and one atomic marker. EngineTick owns
  all state transition and logging work.
- Treats dispatch as input-route evidence only, never target-level pickup proof.
  The same Component has a 750 ms re-entry delay while other candidates may
  proceed after the global slot is released.
- Retains exact Actor/Component invalidation or exact Component state change as
  alternate terminal evidence inside the same 750 ms fallback window.
- Allows at most one retry after a 200 ms delay when no dispatch is observed.
  A second no-dispatch result applies a 1500 ms expiring backoff, not an
  activation-long quarantine. Both delays belong only to the exact Component;
  they never pause the global scanner for other selector results.
- Treats an interaction-owner identity change inside the same `UWorld` as a
  travel/context reset, clears pending attempts, and enforces a new 1500 ms
  settle interval before resuming.
- Supports plants, materials, drops, supported animals, and fish.
- Works on foot and while mounted.
- Requires strong LocalPlayer/controller/current-Pawn identity in both normal
  and mounted modes.
- Explicitly excludes treasure chests.
- Reads the saved semantic `INTERACT` keyboard binding once per enable.
- Uses the configured fallback key only when automatic binding resolution fails.
- Shows a short, top-center native status card when F9 starts, enables,
  disables, or cannot enable Auto Pickup. It is click-through, does not change
  input mode or cursor state, and automatically fades away.
- Retains no enabled state or gameplay UObject across main-menu, save, or World
  initialization boundaries.
- Performs no UObject/Actor scan, overlap hook, root/physics collision
  mutation, direct pickup RPC, Windows synthetic input, or worker-thread scan.
- The optional 3x-20x range PAKs author the exact `SphereOverlapComp` scale for
  all 19 class-proven type-7 drop packages. Native runtime multiplication is
  compile-time disabled, preventing a future reflection change from applying
  the selected range twice.

## Status card

The F9 lifecycle uses a compact native UMG card at the top center of the
viewport:

| State | Message | Accent |
| --- | --- | --- |
| Enable requested | `STARTING...` | warm gold |
| Automation enabled | `ENABLED` | mint green |
| Automation disabled | `DISABLED` | slate blue |
| Enable rejected | `NOT READY` | soft red |

The visual uses a translucent deep-blue glass layer, a softer inner layer,
subtle shadow and top highlight, plus a low-opacity status glow and rule. It
fades and slides with smoothstep easing and a restrained 1.5% reveal scale.
The card remains readable for a bounded interval and then collapses completely;
it has no persistent per-frame drawing while hidden.

Every widget is hit-test-invisible. The card never changes input mode, cursor
state, selection, injection, scheduling, retry, or confirmation behavior. A UI
ABI mismatch or guarded runtime fault disables only the card and leaves Auto
Pickup operational.

## Supported runtime

The 1.3.1 release supports one tested UE4SS ABI only:

- UE4SS v3.0.1 Beta #0 commit `1c1a1497`
- ExperimentalNested layout: `Win64/ue4ss/UE4SS.dll`
- Native DLL marker: `DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1`

StableRoot and other UE4SS plugin variants are not included in this release.
The game executable SHA-256 is retained for diagnostics only. It never selects
or authorizes a selector address, and there is no hash-to-address table or
fixed-RVA fallback. After the exact UE4SS hash and loaded-path gate succeeds,
the adapter parses the loaded PE32+ image. The Server path derives the virtual
slot from the reflected `Server_RunInteractV2` exec thunk, reads that slot from
the interactable CDO, and bounds the resulting native implementation through
x64 `.pdata` and `CHAININFO`. The UI path bounds the complete reflected
`SetInteractUIV2` exec wrapper, requires one unique terminal `E8 rel32` call to
its native implementation, bounds that implementation separately, and applies
the UI selector structural contract. Both paths must resolve the same selector
address. The policy is
`runtime_reflection_dual_caller_rel32_consensus_fail_closed`. Missing,
ambiguous, inconsistent, or non-executable contracts keep automation Off and
report `SELECTOR_UNAVAILABLE`.

This can tolerate address relocation when the required code contracts remain
compatible. It is not a guarantee for arbitrary future recompiles, and an
unlisted game build is not considered tested merely because its hash is not a
gate.

## Installation

The recommended package is `DragonSwordAutoPickup-v1.3.1-Installer.zip`.
Close the game, run Setup, and confirm the automatically detected
`DSClient-Win64-Shipping.exe` path. If Steam discovery is unavailable, use
Browse to select it manually.

Setup exposes one safe action for the inspected installation state:

- **Install** when Auto Pickup is absent;
- **Update** for a recognized older owned installation;
- **Repair** for the current owned version; both Update and Repair let you
  change the toggle and fallback keys while preserving all other settings;
- **Uninstall** when the owned installation can be removed safely.

Review the loaded keys, edit them if needed, and confirm the selected values
before applying the action. Cancel changes no files. The complete 2026-09-07
release refresh is under `dist/releases/1.3.1`; use its manifest and
`docs/RELEASE_STATUS.md`. The separate `out/installer/unified-keys-20260907/`
EXE and its receipt are retained as an earlier installer-only checkpoint.

An exact-runtime Update or Repair uses temporary transactional rollback data and does
not create a persistent conversion backup. If another or mixed UE4SS layout
must be converted, Setup requires confirmation, creates a complete verified
backup inside `Win64`, installs the tested runtime, and migrates unrelated Mods
and configuration. Unknown same-name Mod ownership fails closed. Exact supported
range-PAK filenames and the legacy canary filename are product-owned by name;
new embedded PAK bytes remain hash-verified after writing.

Four release packages are generated:

1. `DragonSwordAutoPickup-v1.3.1-Installer.zip` - recommended one-click Setup.
2. `DragonSwordAutoPickup-v1.3.1-Manual-No-UE4SS.zip` - Mod only for an existing
   exact compatible runtime.
3. `DragonSwordAutoPickup-v1.3.1-Manual-With-UE4SS.zip` - complete direct-paste
   package containing the Mod and tested runtime.
4. `DragonSwordPickupRangeExpansion-v1.3.1.zip` - standalone manual range bundle
   containing 3x, 5x, 10x, 15x, and 20x PAK choices.

See [docs/INSTALL.md](docs/INSTALL.md) for manual installation and removal.
See [docs/WORKSPACE_LAYOUT.md](docs/WORKSPACE_LAYOUT.md) for the one authoritative
source, build, release, and installed-layout contract.
See [docs/SAFETY_AUDIT_1_2_TO_1_3.md](docs/SAFETY_AUDIT_1_2_TO_1_3.md) for the
retained/removed safety-boundary audit and runtime acceptance requirements.

## Optional interaction range

The installer can select Original, 3x, 5x, 10x, 15x, or 20x native interaction
range. Range expansion is a separate PAK resource and works independently from
Auto Pickup. Install at most one range option. Manual Auto Pickup packages do
not include range PAKs; use the standalone range bundle published beside them.
The 15x and 20x choices keep their full gather/animal range, while the 19
short-lived drop targets use the stable 10x overlap range to reduce stale
native prompt-list entries during fast travel.

Each PAK expands 50 reviewed gather/animal interaction capsules and the exact
overlap sphere in 19 class-proven type-7 drop packages: 69 targets and 138 PAK
entries total. Ordinary meat, aged meat, coins, nuts, crystals, minerals, grain,
and the other reviewed F-pickable monster drops use 3x, 5x, or 10x range; they
remain capped at 10x in the 15x and 20x packages. The range remains active when
Auto Pickup is absent. Treasure/type-4 assets remain excluded.

## Public configuration defaults

```ini
enabled_on_launch=false
automatic_pickup=true
toggle_hotkey=F9
interaction_key=AUTO
interaction_key_fallback=F
debug_logging=false
```

After changing the game's interaction binding, toggle Auto Pickup off and on to
reload it. A concrete Unreal key name can be selected as a fallback during
installation or configured manually.

## Validation boundary

The canonical 1.3.1 offline release is built under `dist/releases/1.3.1` and
passes source, core, native-artifact, 11/11 installer, exact-entry, checksum,
and deterministic-archive gates. The high-range change is limited to the PAKs:
15x/20x keep their full gather/animal range while short-lived item drops use
the reviewed 10x range. Deployment and exact-package gameplay acceptance have
not been claimed.

Version 1.2.0 was built, packaged, deployed, and exercised in the game. Its
runtime evidence proves the selector/action route still executes, but rejects
its observational confirmation and rate-limited F9 policies as the next
release baseline because they permitted repeated action storms and repeated
toggle transitions.

Version 1.3.0 implements the bounded corrective policy and the fail-closed
runtime selector policy described above. The dispatch-observer repair is a new
locally built and deployed candidate with status `RUNTIME_PENDING`: 927,744
bytes, SHA-256
`AC86CF2FA26047CF713B567C1CA63D4AD424C86A3FF9C020B80CAD07B4211F5D`.
It is not a packaged public release. The earlier pre-compatibility 1.3.0
release-candidate DLL was 422,912 bytes with SHA-256
`BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188`;
its unsigned Setup executable was 11,704,832 bytes with SHA-256
`570E9E81FA31DFE242999C8352311DF6E566D0B18DAEC2E103734C2A4D3447DB`.
Those hashes are historical and are not the final action-lifecycle artifacts.
The following hashes identify the preceding offline artifacts. They predate the
dispatch-observer repair and must not be presented as its binaries. They retain
the exact-Component and owner-settle audit, use a 750 ms fallback window and
200 ms retry delay, include the 19 structured monster-drop packages in every
range PAK, and disable native runtime range multiplication:

- native DLL: 919,552 bytes, SHA-256
  `10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
- unsigned Setup: 13,001,728 bytes, SHA-256
  `2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`;
- installer ZIP: 8,556,743 bytes, SHA-256
  `70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`;
- manual without UE4SS ZIP: 362,076 bytes, SHA-256
  `9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`;
- manual with UE4SS ZIP: 8,446,667 bytes, SHA-256
  `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`;
- standalone range ZIP: 3,919,600 bytes, SHA-256
  `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.

These values establish exact provenance only for the preceding artifacts.
Historical runtime logs establish that a local 1.3.0 diagnostic build resolved
and exercised the action route, but they do not validate the new observer
candidate. Its exact local build and deployment are now recorded; dispatch
correlation, multi-target progress, manual-F availability, travel, and
performance checks remain pending.

The corrected Setup recognizes the immediately preceding owned 1.3.0 DLL
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
for in-place Repair when the exact version and Lua hash also match. The first
200 ms Setup omitted that contract and is superseded. Modified or foreign
same-name payloads remain blocked with zero mutation.

## License

First-party work is licensed under `GPL-3.0-only`; see [`LICENSE`](LICENSE),
the installer's [`THIRD_PARTY_NOTICES.txt`](installer/THIRD_PARTY_NOTICES.txt),
and the repository [`LICENSE_SCOPE.md`](../LICENSE_SCOPE.md). UE4SS and game
material retain their separate terms.
