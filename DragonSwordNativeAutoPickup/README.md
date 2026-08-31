# DragonSword Native Auto Pickup

Native UE4SS automatic pickup for DragonSword: Awakening.

## Project status

The selector and Enhanced Input pickup route is functionally established for
on-foot and mounted play, including fish and drops. Deployed version `1.2.0`
also exposed a correctness problem under sustained automation: one runtime
session recorded 250 injections, 17 exact weak-identity confirmations, 30
timeouts, and 38 confirmation supersedes, with one target receiving 61
injections. The same sessions showed that the 500 ms F9 debounce could accept a
later operating-system repeat and re-enable automation after an apparent
disable.

Version `1.3.0` is the corrective release. It keeps the accepted
game-owned selector semantics and Enhanced Input route, but no longer calls a
fixed selector RVA. The process resolves one selector capability from two
independent reflected code paths before automation can be enabled. It also permits
only one globally pending automatic action keyed to the exact returned
interaction Component. Success requires exact Actor or Component invalidation,
or an exact same-Component transition out of the live
  interactable state. The confirmation window is 650 ms. A first unconfirmed
  timeout enters a 100 ms cooldown; the
game must present the same exact Component again before one bounded retry is
admitted. A second timeout quarantines only that Component for the current
activation. F9 is a true physical press edge, so key repeat cannot create
another transition until release.
The latest action-lifecycle source passed the fresh exact static, core,
built-artifact, installer 10/10, deterministic ZIP, exact-entry, and checksum
gates. A local 1.3.0 diagnostic installation produced in-process selector and
action evidence, and the owner reported completed gameplay testing and accepted
the current version on 2026-08-31. The repository does not independently prove
that the tested installed DLL is byte-identical to the sealed release hash.

## Behavior

- Resolves the native interaction selector once per process from the reflected
  `Server_RunInteractV2` virtual path and the reflected `SetInteractUIV2`
  direct-wrapper path, then uses one bounded selector call per due scan.
- Uses a bounded 25 ms engine/active cadence, retains a 33 ms idle cadence,
  waits 25 ms after confirmed pickup, and allows only one globally pending
  automatic interaction.
- Confirms exact Actor/Component invalidation or the exact pending
  component leaving the live interactable state.
- Allows at most one retry of the same exact Component after a 100 ms cooldown,
  and only when the game selector presents it again. A second timeout
  quarantines that Component; unrelated candidates may still proceed.
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
- Retains no enabled state or gameplay UObject across main-menu, save, or World
  initialization boundaries.
- Performs no UObject/Actor scan, overlap hook, root/physics collision
  mutation, direct pickup RPC, Windows synthetic input, or worker-thread scan.
- The optional 3x-20x range PAKs author the exact `SphereOverlapComp` scale for
  all 19 class-proven type-7 drop packages. Native runtime multiplication is
  compile-time disabled, preventing a future reflection change from applying
  the selected range twice.

## Supported runtime

The 1.3.0 release supports one tested UE4SS ABI only:

- UE4SS v3.0.1 Beta #0 commit `1c1a1497`
- ExperimentalNested layout: `Win64/ue4ss/UE4SS.dll`
- Native DLL marker: `DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_0`

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

The recommended package is `DragonSwordAutoPickup-v1.3.0-Installer.zip`.
Close the game, run Setup, and confirm the automatically detected
`DSClient-Win64-Shipping.exe` path. If Steam discovery is unavailable, use
Browse to select it manually.

Setup exposes one safe action for the inspected installation state:

- **Install** when Auto Pickup is absent;
- **Upgrade** when the existing Auto Pickup installation is owned and
  recognized; the installed `config.ini` is preserved;
- **Uninstall** when the owned installation can be removed safely.

An exact-runtime Upgrade uses temporary transactional rollback data and does
not create a persistent conversion backup. If another or mixed UE4SS layout
must be converted, Setup requires confirmation, creates a complete verified
backup inside `Win64`, installs the tested runtime, and migrates unrelated Mods
and configuration. Unknown same-name Mod ownership fails closed. Exact supported
range-PAK filenames and the legacy canary filename are product-owned by name;
new embedded PAK bytes remain hash-verified after writing.

Four release packages are generated:

1. `DragonSwordAutoPickup-v1.3.0-Installer.zip` - recommended one-click Setup.
2. `DragonSwordAutoPickup-v1.3.0-Manual-No-UE4SS.zip` - Mod only for an existing
   exact compatible runtime.
3. `DragonSwordAutoPickup-v1.3.0-Manual-With-UE4SS.zip` - complete direct-paste
   package containing the Mod and tested runtime.
4. `DragonSwordPickupRangeExpansion-v1.3.0.zip` - standalone manual range bundle
   containing 3x, 5x, 10x, 15x, and 20x PAK choices.

See [docs/INSTALL.md](docs/INSTALL.md) for manual installation and removal.
See [docs/WORKSPACE_LAYOUT.md](docs/WORKSPACE_LAYOUT.md) for the one authoritative
source, build, release, and installed-layout contract.

## Optional interaction range

The installer can select Original, 3x, 5x, 10x, 15x, or 20x native interaction
range. Range expansion is a separate PAK resource and works independently from
Auto Pickup. Install at most one range option. Manual Auto Pickup packages do
not include range PAKs; use the standalone range bundle published beside them.
The 15x and 20x choices are intentionally aggressive and should be tested
separately in dense areas.

Each PAK expands 50 reviewed gather/animal interaction capsules and the exact
overlap sphere in 19 class-proven type-7 drop packages: 69 targets and 138 PAK
entries total. Ordinary meat, aged meat, coins, nuts, crystals, minerals, grain,
and the other reviewed F-pickable monster drops therefore use the selected
range even when Auto Pickup is absent. Treasure/type-4 assets remain excluded.

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

Version 1.2.0 was built, packaged, deployed, and exercised in the game. Its
runtime evidence proves the selector/action route still executes, but rejects
its observational confirmation and rate-limited F9 policies as the next
release baseline because they permitted repeated action storms and repeated
toggle transitions.

Version 1.3.0 implements the bounded corrective policy and the fail-closed
runtime selector policy described above. The earlier pre-compatibility 1.3.0
release-candidate DLL was 422,912 bytes with SHA-256
`BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188`;
its unsigned Setup executable was 11,704,832 bytes with SHA-256
`570E9E81FA31DFE242999C8352311DF6E566D0B18DAEC2E103734C2A4D3447DB`.
Those hashes are historical and are not the final action-lifecycle artifacts.
The current offline artifacts retain the exact-Component and owner-settle audit,
use a 750 ms confirmation window and 200 ms retry cooldown, include the 19
structured monster-drop packages in every range PAK, and disable native runtime
range multiplication:

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

These values establish exact offline build and package provenance. Runtime logs
establish that a local 1.3.0 diagnostic build resolved and exercised the action
route, and the owner accepted current gameplay on 2026-08-31. Exact installed
artifact identity remains a separate unrecorded receipt; do not infer that the
tested DLL has the sealed hash solely from its runtime version label.

The corrected Setup recognizes the immediately preceding owned 1.3.0 DLL
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
for in-place Repair when the exact version and Lua hash also match. The first
200 ms Setup omitted that contract and is superseded. Modified or foreign
same-name payloads remain blocked with zero mutation.
