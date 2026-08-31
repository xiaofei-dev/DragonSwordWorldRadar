# DragonSword Native All-Mount Free Flight

This pure-resource mod enables the native free-flight dash entry for every mount.

## Behavior

- Preserves every mount's own 11-entry combo list and existing Shift skills.
- Changes combo index `10` (`GLIDE_FREE_DASH`) to `NONE_CONDITION` for all 30
  current mount IDs.
- Preserves the native three-second cooldown and authored state assets.
- Does not broadcast the Dragon familiar reward to unrelated mounts.
- Does not modify mount base speed, movement components, or state speed caps.

## Installation

Copy `dist/DS_NativeAllMountsFreeFlight_P.pak` into:

```text
DS/Content/Paks/~mods/
```

Remove that one PAK to uninstall the mod. Never replace an original
`pakchunk*.pak` file.

## Source publication boundary

The repository publishes the behavior description, validation metadata,
reproducible build logic, and the optional native asset inspection utility.
Extracted game assets, decoded reference copies, staging trees, backups, and
built PAK files are local-only and intentionally excluded from Git.

## Acceptance boundary

Static verification proves the intended table delta and PAK contents. In-game
testing is still required to confirm animation and handling on representative
ground, swimming, gliding, and flying mounts.

## License

First-party work is licensed under `GPL-3.0-only`; see [`LICENSE`](LICENSE),
[`THIRD_PARTY_NOTICES.txt`](THIRD_PARTY_NOTICES.txt), and the repository
[`LICENSE_SCOPE.md`](../LICENSE_SCOPE.md). Extracted and cooked game assets
are outside that grant.
