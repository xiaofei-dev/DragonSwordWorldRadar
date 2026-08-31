# DragonSword Pickup Range Expansion

This optional pure-resource add-on expands the game's native interaction range.
It works independently of DragonSword Auto Pickup: the selected range applies
even while Auto Pickup is disabled.

## Choose one variant

- `DS_PickupRangeX3_P.pak` — 3x native interaction range.
- `DS_PickupRangeX5_P.pak` — 5x native interaction range.
- `DS_PickupRangeX10_P.pak` — 10x native interaction range.
- `DS_PickupRangeX15_P.pak` — 15x native interaction range.
- `DS_PickupRangeX20_P.pak` — 20x native interaction range.

Install **exactly one** variant. Do not install multiple range PAKs together,
because they override the same 69 reviewed game packages.

Each variant contains 50 gather/animal interaction-capsule targets and 19
class-proven type-7 monster-drop overlap-sphere targets (138 `.uasset`/`.uexp`
entries). Ordinary meat, aged meat, coins, nuts, crystals, minerals, grain, and
the other reviewed F-pickable drops are included. Treasure/type-4 packages are
excluded.

`SHA256SUMS.txt` records the exact SHA-256 hash of each included PAK and this
README. You can compare a selected PAK with PowerShell output from
`Get-FileHash .\DS_PickupRangeX3_P.pak` (or the selected 5x/10x/15x/20x
filename) before installation.

## Manual installation

1. Close DragonSword: Awakening.
2. Open the game installation folder.
3. Go to `DS\Content\Paks\~mods` and create `~mods` if it does not exist.
4. Copy one selected `DS_PickupRangeX*_P.pak` file into `~mods`.
5. Remove any other `DS_PickupRangeX*_P.pak` or old
   `DS_PickupRangeX3Canary_P.pak` from `~mods`.
6. Launch the game.

To uninstall, close the game, remove the selected range PAK, and restart the
game. No save data is modified.

## Scope and safety

Each production variant overrides the same reviewed inventory of 50 cooked
packages: 45 gather packages and 5 interactable-animal packages. Treasure
chests are intentionally excluded. The add-on changes only the dedicated
authored interaction-capsule scale and adds no runtime hook, polling loop,
UObject scan, or background process.

The 3x, 5x, 10x, 15x, and 20x variants differ only by the selected scale multiplier.
Higher values are more convenient but can make the native prompt select an
object from farther away.

## Compatibility

These PAKs are designed for the reviewed DragonSword: Awakening game build.
After a game update, remove the add-on until compatibility is reconfirmed.

Static package checks do not prove gameplay behavior on every system. Test the
selected variant with Auto Pickup disabled, confirm normal gathering and fish
interaction, confirm treasure chests remain manual, travel between areas, and
exit the game cleanly.
