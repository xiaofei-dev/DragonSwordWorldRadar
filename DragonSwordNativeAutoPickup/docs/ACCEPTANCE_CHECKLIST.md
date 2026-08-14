# Acceptance Checklist

## Static and build acceptance for 0.9.0

- [x] CMake links only `src/ue4ss/main_0_9.cpp`.
- [x] Exact current executable hash, RVA, and 16-byte native prefix are required.
- [x] PolyHook2 detour installation must return a valid original trampoline.
- [x] The original native function runs before edge recording.
- [x] F9 uses a 250 ms qualified-release edge and queues only a scalar toggle.
- [x] `on_update` performs no UObject, detour, foreground, or input work.
- [x] F input is one scan-code keydown/keyup pair, foreground-only, with a 250 ms cooldown.
- [x] A new active component edge queues one request; hidden state clears it.
- [x] InitGameState forces Off and clears component/input state.
- [x] No reflected hook, `ProcessEvent`, object scan, Pawn query, target write, or direct RPC exists in the active adapter.
- [x] Strict pinned native build, core tests, source manifest, and package layout pass.

## Owner runtime acceptance

- [ ] Startup records `NATIVE_DETOUR_READY` and `READY` without a crash.
- [ ] One F9 press produces one stable `state=On` transition.
- [ ] Approaching ordinary loot increments `native_visibility_events`.
- [ ] A new active edge records `F_INPUT_SENT` while the game is foreground.
- [ ] The item is visibly collected without manual F.
- [ ] One component does not create a repeated input loop.
- [ ] Mounted pickup is tested separately.
- [ ] F9 Off, World travel, Radar F7 coexistence, and normal exit are safe.

Static acceptance does not satisfy any unchecked runtime item.
