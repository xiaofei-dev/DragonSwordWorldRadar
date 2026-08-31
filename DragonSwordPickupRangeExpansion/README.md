# DragonSword Pickup Range Expansion

This project builds optional pure-resource PAKs that expand the game's native
interaction-trigger range to 3x, 5x, 10x, 15x, or 20x for reviewed gather and
interactable-animal assets. It does not modify the original `pakchunk*.pak`
files and adds no runtime polling, UObject scan, hook, worker, or collision
proxy.

## Release artifact

Each production variant overrides the same 50 cooked packages:

- `DS_PickupRangeX3_P.pak`;
- `DS_PickupRangeX5_P.pak`;
- `DS_PickupRangeX10_P.pak`;
- `DS_PickupRangeX15_P.pak`;
- `DS_PickupRangeX20_P.pak`.

Install exactly one. The standalone
`DragonSwordPickupRangeExpansion-v1.3.0.zip` contains all five choices and an
English installation README.

The reviewed inventory contains:

- 45 production gather packages, including explicitly serialized `NormalGather`
  assets and three `DsAnimationProp` gather assets that inherit the same
  interaction contract; this covers herbs, the blue flower, crops, eggs,
  mushrooms, water plants, conches, shellfish, and related collection props;
- 5 `DsInteractableAnimal` packages for salmon, trout, silverwing, sand crab,
  and shrimp.

The dedicated authored interaction capsule is scaled by exactly the selected
multiplier. For example, the common `(10, 10, 5)` cohort becomes
`(30, 30, 15)`, `(50, 50, 25)`, or `(100, 100, 50)`. Six conch/shellfish
packages use their own `(18, 18, 18)` baseline. Intrinsic capsule dimensions,
movement/root collision, visual components, and treasure assets are not
changed.

The build is manifest-driven and fails closed on source hashes, duplicate or
missing scale signatures, unsupported multiplier values, treasure-like paths, unexpected byte
changes, PAK inventory drift, unpacked hash drift, or semantic component drift.
The release bundle additionally preserves the historical three-target x3
canary unchanged and verifies a deterministic ZIP rebuild.

## Relationship to Native Auto Pickup

The PAK and `DragonSwordNativeAutoPickup` remain independently removable, but
their type-7 coverage differs:

- With this PAK installed, the native `F` prompt uses the expanded range even
  when Auto Pickup is disabled with `F9`.
- Removing this PAK and restarting the game restores the original authored
  range.
- Auto Pickup retains its selector, target policy, mounted route, fish support,
  resolved interaction action, and treasure exclusion.

Each variant contains 69 reviewed targets and 138 PAK entries: 50 authored
gather/animal interaction capsules plus the exact `SphereOverlapComp` in 19
class-proven type-7 drop packages. The structured asset patch adds only
`RelativeScale3D` to that overlap component and verifies that physics and hit
components are unchanged. Ordinary meat, aged meat, animal meat, coins, nuts,
crystals, minerals, grain, and the other reviewed F-pickable drops are included.
Treasure/type-4 packages remain excluded. The PAK works independently of Auto
Pickup; Auto Pickup's native runtime multiplier is disabled to prevent double
application.

## Installation and rollback

Install one production PAK under `DS/Content/Paks/~mods` while the game is
closed. Never install multiple range variants together, and do not install one
alongside the historical `DS_PickupRangeX3Canary_P.pak`. Removing the selected
production PAK and restarting the game is the complete rollback.

## Acceptance boundary

Each artifact passes source-hash, byte-difference, two-build entry-content, PAK
inventory, and independent-unpack gates. The existing x5 artifact also has a
semantic reparse record. These checks do not prove gameplay behavior. Owner
acceptance should test each intended choice independently with Auto Pickup Off,
confirm an earlier native prompt for one normal herb/egg, one conch/shellfish,
and one fish, then verify treasure behavior, world travel, and clean exit.

Extracted assets, staged packages, built PAKs, and runtime records are ignored and
must not be committed.

## License

First-party work is licensed under `GPL-3.0-only`; see [`LICENSE`](LICENSE),
[`THIRD_PARTY_NOTICES.txt`](THIRD_PARTY_NOTICES.txt), and the repository
[`LICENSE_SCOPE.md`](../LICENSE_SCOPE.md). The license does not grant rights to
game assets or generated asset overrides.
