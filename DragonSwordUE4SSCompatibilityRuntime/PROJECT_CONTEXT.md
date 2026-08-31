# DragonSword UE4SS Compatibility Runtime

## Purpose

This project packages the exact ExperimentalNested UE4SS runtime used and
tested by the DragonSword Native Map and Radar Enhancer and DragonSword Auto
Pickup projects.

It is a redistribution and packaging project only. It does not modify UE4SS,
and it does not include either DragonSword Mod.

## Runtime identity

- UE4SS version: `v3.0.1 Beta #0`
- UE4SS source commit: `1c1a1497f942c707f47ba668db75b25e86f6c08a`
- Layout: `ExperimentalNested`
- Game proxy location: `Win64/dwmapi.dll`
- UE4SS runtime location: `Win64/ue4ss/UE4SS.dll`
- Mods location: `Win64/ue4ss/Mods`

## Package policy

- Copy the exact tested loader, proxy, DragonSword mapping, and settings only
  after their pinned SHA-256 values pass.
- Copy the upstream default Mods from the exact source commit.
- Include the UE4SS MIT license and upstream dependency notices.
- Exclude logs, user Mods, Radar, Auto Pickup, personal configuration,
  backups, build output, and unrelated game files.
- Do not include `UE4SS_SDK_Backends`. It is not required to run these Mods,
  and no matching backend payload is present in the tested runtime.
- All project-authored public text must be English. Preserve pinned upstream
  UE4SS files byte-for-byte, including upstream examples in default Mods.

## Build

Run `tools/Build-Release.ps1`. The script validates all pinned inputs, creates
an isolated staging tree, writes full-file checksums, archives the package,
extracts it again, and verifies exact archive contents and hashes.

## Acceptance boundary

Passing the packaging gates proves package identity and layout only. Runtime
acceptance remains the owner-observed result from the already tested
ExperimentalNested installation.
