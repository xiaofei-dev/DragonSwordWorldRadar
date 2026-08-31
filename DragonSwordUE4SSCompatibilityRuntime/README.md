# UE4SS Compatibility Runtime for DragonSword

Unofficial, clean UE4SS runtime package for DragonSword: Awakening.

This package uses the exact ExperimentalNested runtime tested with:

- DragonSword Native Map and Radar Enhancer
- DragonSword Auto Pickup

It contains UE4SS and its upstream default Mods only. The Radar and Auto
Pickup Mods are not included.

## Runtime version

- UE4SS `v3.0.1 Beta #0`
- Source commit `1c1a1497f942c707f47ba668db75b25e86f6c08a`
- ExperimentalNested layout

## Installation

1. Close DragonSword: Awakening.
2. Open the folder containing `DSClient-Win64-Shipping.exe`.
3. If another UE4SS installation is present, back it up and remove its active
   loader before continuing. Do not merge two different UE4SS runtimes.
4. Extract the contents of this ZIP into that `Win64` folder.
5. Confirm that `dwmapi.dll` is beside the game executable and that
   `ue4ss/UE4SS.dll` exists.
6. Install the Mod-only package for Radar, Auto Pickup, or both.
7. Merge each Mod's supplied entry into `ue4ss/Mods/mods.txt`.

The one-click Radar installer is safer when converting an existing UE4SS
installation because it creates a rollback backup and migrates existing Mods
and configuration.

## Included content

- Exact tested `UE4SS.dll`
- Exact tested `dwmapi.dll` proxy
- DragonSword UE mapping used by the tested runtime
- Tested DragonSword UE4SS settings
- Upstream default UE4SS Mods from the pinned source commit
- UE4SS MIT license and upstream dependency notices
- Complete SHA-256 manifest

## Not included

- DragonSword Native Map and Radar Enhancer
- DragonSword Auto Pickup
- User Mods or personal configuration
- UE4SS logs or backups
- `UE4SS_SDK_Backends`

`UE4SS_SDK_Backends` is used for SDK-generation workflows and is not required
to load or run the two supported Mods. A backend from a different UE4SS commit
is intentionally not mixed into this package.

## License and attribution

UE4SS is Copyright (c) 2022 Narknon and is distributed under the MIT License.
See `ue4ss/LICENSE` and `ue4ss/licenses`.

Source:
https://github.com/UE4SS-RE/RE-UE4SS/tree/1c1a1497f942c707f47ba668db75b25e86f6c08a

This is an unofficial DragonSword compatibility package. It is not an
official UE4SS release and is not affiliated with or endorsed by the UE4SS
project.
