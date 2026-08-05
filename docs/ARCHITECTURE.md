# Architecture

## Runtime processes

DragonSwordWorldRadar uses a hidden WScript watcher and a transient Windows PowerShell process while the game is running. PowerShell compiles the WinForms overlay source in memory.

```text
UE4SS Lua -> runtime/launch.request -> hidden watcher
                                      -> in-memory C# compile
                                      -> WinForms overlay
```

No custom radar `.exe` is built or distributed.

## Layer boundaries

```text
TreasureDataProvider -> generated/treasures.lua -> treasure bridge points
                                              -> TreasureSaveState filter
                                              -> treasure renderer

BossDataProvider     -> generated/bosses.lua    -> boss_tracker.lua
                                              -> boss bridge points
                                              -> boss renderer
```

The boss layer does not use `tb_treasure_box`, treasure overrides, or permanent opened-bit filtering. Runtime actor observation owns boss visibility.

## Boss state lifecycle

1. Boss catalog entries start visible with an `unknown` runtime state.
2. A streamed actor matching the configured UIDName and spawn coordinates confirms `alive`.
3. A reflected dead flag hides the marker immediately on the next one-second scan.
4. If a previously observed actor disappears while the player remains near its spawn, two consecutive scans confirm the hidden state.
5. A newly spawned matching actor restores the marker.

This deliberately avoids guessing a server-global state from static `FieldBossListData`. Off-screen server changes are reconciled when the relevant actor streams in.

## Runtime directories

```text
runtime/
  bridge/       transient Lua-to-overlay state
  logs/         installer, watcher, host, Lua, and overlay logs
  launch.request
  reinstall-required.json
```

## Data lifecycle

```text
Install.cmd
  -> resolve local game layout
  -> compile installer C# in memory
  -> execute TreasureDataProvider and BossDataProvider
  -> write data/generated/*
  -> write metadata/datasets.json
  -> write metadata/install-state.json
```

## Configuration lifecycle

- `scripts/config.default.lua` is version controlled.
- `scripts/config.lua` is created on first installation and merged without replacing user values.
- `show_treasures` and `show_bosses` independently control their layers.
- `data/treasure_overrides.txt` remains treasure-only and is preserved on upgrades.
