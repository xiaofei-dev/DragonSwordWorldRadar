# Project Context

Current local packages and evidence: `docs/RELEASE_STATUS.md`.
The 2026-09-07 complete release supersedes the installer-only checkpoint, not
the separate runtime-acceptance boundary. No deployment or publication is
implied by packaging. Nexus copy is indexed in `assets/nexus/README.md`.

## Role

`DragonSwordNativeAutoPickup` owns automatic pickup only. The current release
is `1.3.1` with runtime label
`DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1`.

Version `1.6.10-ue4ss-f9-fish-perf-diagnostics` established the selector,
Enhanced Input, on-foot, mounted Rider, fish, and bounded-scan route. Deployed
1.2.0 runtime evidence subsequently rejected two scheduling policies: one
session produced 250 injections, 17 confirmations, 30 timeouts, and 38
supersedes, including 61 injections for one target, and F9 repeat could cross
the 500 ms debounce and create another transition. Version 1.3.0 preserves the
accepted target/action route, replaces the fixed selector RVA with one
process-lifetime capability resolved by two reflected structural paths, and
replaces those scheduling policies with one global
pending action keyed to the exact returned Component, exact
invalidation/state-transition confirmation, one selector-represented bounded
retry before exact-Component quarantine, and a
true physical F9 edge.
Version 1.3.1 preserves that native pickup behavior. It changes the separate
high-range PAK policy so 15x/20x gather and animal targets retain their selected
range while short-lived drops are capped at 10x. It also corrects one startup
readiness race: if the reflected interactable class exists before its CDO, the
Mod remains fail-closed and retries that dependency every 250 ms for at most 30
seconds before running the unchanged full reflection and selector contracts.
The latest corrective source includes the strict owner-settle and preflight
ordering audit and passed a fresh exact static, core, native-artifact,
installer 11/11, and deterministic four-package audit. A local 1.3.0
diagnostic installation produced in-process evidence, and the owner reported
completed gameplay testing and acceptance on 2026-08-31. The exact installed
DLL hash was not independently recorded in this repository.

## Runtime contract

- Automatic pickup is Off after every game start.
- Returning to the main menu or initializing a new save or World forces Off,
  clears session-only interaction state, and requires a new toggle after load.
- `toggle_hotkey` controls the On/Off key; the public default is `F9`.
- One physical key press produces at most one transition. Operating-system
  key-repeat events are ignored until the key has been released.
- `interaction_key=AUTO` reads the saved semantic `INTERACT` keyboard binding
  once per enable and caches the resolved Unreal key.
- `interaction_key_fallback=F` is a concrete fail-closed fallback used only
  when semantic detection fails; a concrete `interaction_key` remains a
  troubleshooting override.
- Process initialization resolves one selector address only when the reflected
  `Server_RunInteractV2` virtual-slot/CDO path and reflected `SetInteractUIV2`
  direct-wrapper path reach the same selector target inside validated PE32+
  runtime-function bounds. One due scan then calls that resolved selector once.
- If only the interactable CDO is temporarily unavailable during startup, a
  bootstrap EngineTick retries initialization every 250 ms for at most 30
  seconds. It cannot scan, accept F9, or inject until the full contract reaches
  `Ready`; every other reflection or selector failure remains immediately
  fail-closed.
- Only one automatic action may be pending globally. No later candidate is
  invoked while it remains pending.
- Exact Actor/Component weak-identity invalidation or the exact pending
  component leaving `InteractableValue=2` is the automatic success signal. The
  confirmation window is 650 ms. A first timeout enters a 100 ms cooldown and
  permits one retry only if the game
  selector presents the same exact Component again. A second timeout
  quarantines that Component for the current activation.
- A changed interaction-owner identity resets the action activation even when
  the `UWorld` identity remains stable. This covers travel/context replacement
  observed in runtime logs without treating mount Rider changes as travel. The
  reset enforces a new 1500 ms settle deadline before the next scan.
- Supported targets are NormalGather type 2, Animal type 5, and class-proven
  `DropItemActor` type 7. TreasureBox type 4 is always excluded.
- The LocalPlayer, exact trusted controller, bidirectional current Pawn, and
  mounted `Rider` receiver are resolved and identity-validated fresh. No
  gameplay UObject is retained across World changes.
- The adapter performs no UObject/Actor scan, overlap hook, root/physics
  collision mutation, direct interaction RPC, Windows synthetic input, or
  worker-thread loop.
- Each optional range PAK authors the exact `RelativeScale3D` of
  `DropItemActor.SphereOverlapComp` in all 19 reviewed type-7 child packages.
  Native runtime range multiplication is compile-time disabled, so the PAK is
  the only range owner and can never be applied twice.

A successful build or installation is not a smoke test of the exact artifact,
and lifecycle source evidence is not gameplay acceptance.

## Installation boundary

The 1.3.1 release uses one tested native ABI: UE4SS v3.0.1 Beta #0
commit `1c1a1497` in the ExperimentalNested layout. If UE4SS is absent, the
installer installs that runtime. If another or mixed UE4SS layout is present,
the installer asks for confirmation, creates a verified Win64-relative backup,
converts the active runtime, and migrates existing Mods and configuration.

The installer distinguishes these product actions:

- Install for an absent Auto Pickup Mod;
- Update for one recognized older owned installation and Repair for the current
  owned version; both apply confirmed toggle/fallback key edits, preserve all
  other configuration, and use only temporary rollback data when no runtime
  conversion is required;
- Uninstall for one recognized owned installation, removing only Auto Pickup,
  its authoritative `mods.txt` entry, and approved owned range PAKs.

Unknown same-name Mod content fails closed. Exact supported range-PAK filenames
and the legacy canary filename are product-owned by name; replacement bytes are
still hash-verified after writing. The installer merges one
authoritative `DragonSwordNativeAutoPickup : 1` entry into the active
`mods.txt`; `enabled.txt` is forbidden because it bypasses that control.

The installer requires a file named `DSClient-Win64-Shipping.exe`. Its SHA-256
is diagnostic evidence only and never an activation or installation gate. The
package and installer enforce the exact nested runtime layout before load. Once
the plugin is loaded, it verifies the nested UE4SS hash and the actual loaded
module path before allowing automation. That post-load passive/Off gate cannot
prevent a crash that occurs before plugin code executes, so it is not described
as a pre-load ABI shield.

After that exact UE4SS gate succeeds, the adapter parses the loaded PE32+ image.
The Server path derives a virtual slot from the reflected
`Server_RunInteractV2` exec thunk, reads the slot from the interactable CDO,
follows bounded direct jumps, and bounds the native implementation through x64
`.pdata` and `CHAININFO`. The UI path bounds the entire reflected
`SetInteractUIV2` exec wrapper, requires exactly one terminal `E8 rel32`
implementation call, bounds that implementation through its runtime metadata,
and validates its UI selector structural call. The two paths must agree on one
selector address. The policy identifier is
`runtime_reflection_dual_caller_rel32_consensus_fail_closed`. No fixed selector
RVA or game-hash address table is present. Failure, ambiguity, disagreement, or
a guarded selector fault keeps automation Off for the process. This policy
supports relocation only while the required code contracts remain compatible;
it does not promise compatibility with an arbitrary recompile.

## Range boundary

The installer may offer the separate `DragonSwordPickupRangeExpansion`
Original, 3x, 5x, 10x, 15x, or 20x choices. Each PAK changes 50 reviewed
gather/animal capsule packages plus the exact overlap component in 19 reviewed
type-7 drop packages, for 69 targets and 138 entries. Ordinary meat, aged meat,
coins, nuts, crystals, minerals, grain, and the other reviewed F-pickable drops
are included; treasure/type-4 assets are not. At most one expanded variant may
be installed. Gather/animal targets use the selected multiplier. Short-lived
drop targets use `min(selected multiplier, 10x)`, so the 15x and 20x variants
avoid extending drop overlap beyond the stable 10x range.

## Public and local diagnostics

Repository and public package defaults use `debug_logging=false`. A local
developer deployment may change only the installed `config.ini` to
`debug_logging=true`. Version 1.3.0 keeps routine action output compact and
emits change-only context/selector/deferred attribution with independent
per-activation caps, one scalar action trace per bounded invocation, and
interval-aggregated performance diagnostics with suppression and logger-health
counters. It does not restore the 1.2.0 seven-line User-log chain per injection.

## Distribution and validation boundary

`ZeroKarya_PartySwitch` is a separate third-party Mod. A local compatibility
patch is not part of this repository or any Auto Pickup installer/archive and
must never be redistributed as Auto Pickup content.

The 1.3.1 distribution contract contains the installer, manual-without-UE4SS,
manual-with-UE4SS, and standalone range-PAK archives. The earlier
pre-compatibility release-candidate DLL was 422,912 bytes /
`BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188`;
its unsigned Setup was 11,704,832 bytes /
`570E9E81FA31DFE242999C8352311DF6E566D0B18DAEC2E103734C2A4D3447DB`.
Those are historical candidate hashes, not final compatibility-repair hashes.
The current 750 ms confirmation-window / 200 ms retry-cooldown DLL is 919,552 bytes /
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
the unsigned Setup is 13,001,728 bytes /
`2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`.
The final installer, manual-without-UE4SS, manual-with-UE4SS, and range ZIPs are
8,556,743 / `70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`,
362,076 / `9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`,
8,446,667 / `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`,
and 3,919,600 /
`504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`
bytes / SHA-256 respectively. Exact offline build/package provenance is
accepted. Runtime diagnostics and owner gameplay acceptance are also recorded,
but the installed artifact-to-release hash receipt remains `NOT_RECORDED`.

The corrected 200 ms Setup explicitly recognizes the preceding owned
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
DLL only as part of its exact 1.3.0 version/DLL/Lua ownership tuple. This makes
Repair available without weakening the fail-closed policy for changed or
foreign same-name files. The first 200 ms Setup omitted this contract and is
superseded.
