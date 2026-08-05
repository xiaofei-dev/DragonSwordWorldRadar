## 0.3.2c

- Fixed malformed release metadata (`0.3.0bb`) and synchronized all installer/runtime version constants.
- Retained the `ooz.exe` fallback data-injection pipeline from 0.3.0b.
- Added release consistency validation to prevent mixed-version packages.

## 0.3.2c
- Fixed DragonSword Oodle decoding: use the established ooz.exe decoder instead of looking for a non-existent oo2core DLL.
- Discovers ooz.exe from DragonSwordWorldRadar tools or the UE4SS Mods root.

## 0.3.2c

- Fixed Oodle discovery for installations where `oo2core_*_win64.dll` is outside `DS\Binaries\Win64`.
- Searches standard Unreal Engine Oodle directories, then the complete game installation.
- Improved the missing-library error with a Steam file-verification instruction.

# Changelog

## 0.3.2c

- Established the standalone DragonSwordWorldRadar repository and release layout.
- Removed the installation dependency on DragonSwordTreasureMap 1.6.1.
- Added one-click local treasure-data extraction and generation.
- Added a reusable `IDataProvider` pipeline for future radar layers.
- Added game-version fingerprinting and launch-time reinstall prompts.
- Normalized runtime, host, installer, source, vendor, metadata, and tool directories.
- Preserved the completed treasure layer: WinForms map rendering, 16 ms map sampling, editable overrides, XYZ nearest selection, puzzle colors, and single-process hidden hosting.
