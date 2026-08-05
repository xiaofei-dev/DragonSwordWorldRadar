# Architecture

## Runtime processes

DragonSwordWorldRadar uses one hidden Windows PowerShell process. The process remains as a low-overhead watcher and hosts the WinForms overlay in the same process while the game is running.

```text
UE4SS Lua -> runtime/launch.request -> hidden watcher
                                      -> in-memory C# compile
                                      -> WinForms overlay
```

No custom `.exe` is built or distributed.

## Runtime directories

```text
runtime/
  bridge/       transient Lua-to-overlay state
  logs/         installer, watcher, host, Lua, and overlay logs
  launch.request
  reinstall-required.json
```

All runtime files are disposable. User configuration and generated datasets are stored outside `runtime`.

## Data lifecycle

```text
Install.cmd
  -> resolve local game layout
  -> compile installer C# in memory
  -> execute registered IDataProvider implementations
  -> write data/generated/*
  -> write metadata/datasets.json
  -> write metadata/install-state.json
```

At game launch, the watcher checks the stored game fingerprint before starting the overlay. A mismatch produces a reinstall prompt.

## Configuration lifecycle

- `scripts/config.default.lua` is version controlled.
- `scripts/config.lua` is created on first installation and preserved on upgrades.
- `data/defaults/treasure_overrides.txt` is version controlled.
- `data/treasure_overrides.txt` is user controlled and preserved on upgrades.
