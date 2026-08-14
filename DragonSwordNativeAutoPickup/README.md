# DragonSword Native Auto Pickup

Version `0.9.0-native-visibility-f-input` detects the game's native ground-loot visibility transition and sends one F scan-code press/release so the game can execute its normal interaction path.

## Runtime behavior

- Press F9 once to enable or disable.
- The native detector has no object scan and no polling search.
- Each newly active component may queue one F input.
- F is sent only when DragonSword is the foreground process.
- World transition turns the Mod Off and clears pending input.

## Why this replaces 0.8

The 0.8 reflected hook registered but received zero events because the current executable directly calls the native implementation. Version 0.9 installs an exact-version PolyHook2 detour at RVA `0x61B3AC0`, guarded by the executable hash and native byte prefix.

The active source performs no UObject/Actor scan, Pawn-chain traversal, distance query, target-field write, direct `Server_RunInteractV2`, or reflected pickup replay.

## First runtime test

1. Start in stable open-world play and press F9 once.
2. Approach one ordinary ground drop without pressing F manually.
3. Confirm visible pickup.
4. Preserve both AutoPickup logs even if pickup fails.
5. Repeat while mounted only after the on-foot result is known.
6. Verify F9 Off, World travel, Radar F7 coexistence, and normal process exit.

Log interpretation is explicit: zero `native_visibility_events` means native detection failed; events without `F_INPUT_SENT` mean an input gate rejected the request; `F_INPUT_SENT` without collection means Windows input did not activate the game's interaction binding.
