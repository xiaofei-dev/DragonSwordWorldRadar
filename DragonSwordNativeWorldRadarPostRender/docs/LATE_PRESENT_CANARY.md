# Late Present Canary

> Historical source-only record. The production CMake target does not compile
> or connect this D3D canary, and no late-Present source or configuration is
> included in the binary runtime payload. Native UMG is the only shipped render
> route.

## Purpose

Dev3 proved that the native 33 ms coordinate snapshot changes correctly and
that reflected UCanvas primitives execute, but DragonSword composites the
compact minimap UMG after `UGameViewportClient::PostRender`. A marker placed in
the compact-map rectangle is therefore covered.

Dev4 proved that the active swap chain reaches the late `Present` detour, but
its D3D11-only resource setup failed because this game session used D3D12.
Dev5 supports both D3D11 and D3D12 while leaving state collection, target
selection, F7/F8, travel epochs, and the atomic numeric snapshot unchanged.

## Defaults

```ini
late_present_hook_enabled=false
late_present_canary_enabled=false
late_present_relative_marker_enabled=false
```

These archived source defaults remain off. They are not part of current runtime
acceptance and no installed configuration is shipped for this canary.

## Hook and lifetime rules

- Resolve `IDXGISwapChain::Present` slot 8 and `ResizeBuffers` slot 13 once by
  creating a hidden 1x1 discovery swap chain during initialization.
- Resolve `ID3D12CommandQueue::ExecuteCommandLists` slot 10 from a temporary
  direct queue and capture the game's direct queue without retaining UObject
  state.
- Install PolyHook x64 detours outside the render callback.
- Chain each original trampoline exactly once.
- Accept only a current-process swap chain whose client area is at least
  800x600, then keep that identity for the process session.
- Pin the DLL and owner until process exit. F8 and shutdown disable drawing;
  they do not free code beneath a potentially late detour entry.
- Release the cached render-target view before the original `ResizeBuffers`
  call and recreate it on the next accepted `Present`.

## Render path

The D3D11 path caches the device, immediate `ID3D11DeviceContext1`, and one
back-buffer render-target view. It draws each cross with two small `ClearView`
rectangles without changing the game's pipeline state.

The D3D12 path caches the matching direct queue, device, per-back-buffer RTVs
and command allocators, one command list, and one fence. Each submitted canary
frame transitions only the current back buffer from `PRESENT` to
`RENDER_TARGET`, clears the small cross rectangles, and transitions back. If a
buffer allocator is still in flight, that canary frame is skipped instead of
waiting inside `Present`.

The steady-state callback performs no file access, logging, database work,
catalog scan, UObject lookup, lock, allocation, or GPU wait. It reads only the
atomic numeric marker snapshot.

## Runtime gate

1. Confirm the legacy green/yellow PostRender witnesses are absent.
2. Press F7 and confirm one cyan cross at the upper left.
3. Confirm one yellow cross appears over the compact minimap and moves when
   the player moves.
4. Press F8 and confirm both crosses disappear; press F7 and confirm recovery.
5. Resize or change display mode once and confirm recovery without a crash.
6. Travel once and exit normally.
7. Inspect `LATE_PRESENT_STATE`, `F8_DISABLED`, and shutdown metrics for zero
   faults before any full marker migration.

This canary is not a performance acceptance result and is not yet the complete
Radar renderer.
