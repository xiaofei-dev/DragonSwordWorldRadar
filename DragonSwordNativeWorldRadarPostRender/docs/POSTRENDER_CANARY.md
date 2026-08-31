# PostRender Canary

> Historical source-only record. The production CMake target does not compile
> this canary, `main.dll` does not reference it, and neither its source nor its
> configuration is included in the binary runtime payload. The verification
> script keeps only retirement and historical hot-path safety assertions.

## Purpose

This project is an isolated copy of the accepted
`DragonSwordWorldRadarObjectState` development baseline. It tests only the new
crash-critical mechanism required to remove the external overlay: an Unreal
`UGameViewportClient::PostRender` vtable hook and callback-local `UCanvas`
drawing.

The stable source directory and installed mod are not modified by this
project.

## Default state

Both settings are disabled by default in `config/postrender_canary.ini`:

```ini
postrender_hook_enabled=false
postrender_canary_enabled=false
postrender_relative_marker_enabled=false
postrender_vtable_slot=112
```

These values describe the archived experiment only. They are not production
runtime settings, and enabling the source-tree file does not connect a hook to
the shipped `main.dll`.

## Compatibility lock

The first prototype accepts only the current audited PE identities:

| Module | PE timestamp | Image size | SHA-256 evidence |
|---|---:|---:|---|
| `DSClient-Win64-Shipping.exe` | `0x691B0D98` | `0x09EB2000` | `3DDDCEE474825310000A4CD24239AE5C8B76EF81BAC223C9F3D52565816A0CEA` |
| `UE4SS.dll` | `0x6A76FCA3` | `0x00FCA000` | `F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1` |

Runtime gating uses the in-memory PE timestamp and image size without file I/O.
The SHA-256 values are build evidence and are not computed on the render path.
Any mismatch refuses installation.

## Hook rules

- Slot 112 is an audited candidate from the supplied MnMRadar binary, not a
  universal Unreal constant.
- The candidate original must be executable and belong to the main game image.
  An existing third-party detour fails closed instead of being overwritten.
- Installation publishes a complete per-vtable hook record before an atomic
  compare-exchange installs the detour.
- The original function is called exactly once and before custom drawing.
- Each callback uses only its `Canvas` argument and never retains the viewport
  or Canvas instance.
- `K2_DrawLine` and all parameter offsets are resolved and size-validated once
  before installation. The callback uses a zeroed fixed stack buffer and does
  no reflection lookup, file I/O, catalog work, scan, logging, or allocation.
- One drawing fault permanently disables the canary for the process.
- F8, transitions, suppressed contexts, and shutdown disable custom drawing.
- The DLL and owner object remain resident until process exit. Slot restoration
  uses compare-exchange and never overwrites a later hook owner.

## First runtime gate

Do not enable this prototype alongside MnMRadar or the stable Radar.

1. Start with both settings false and confirm ordinary startup and exit.
2. Enable only `postrender_hook_enabled`; confirm callback chaining without a
   custom line.
3. Enable the canary, press F7, and confirm one short green line at logical
   coordinates `(48, 48)` to `(80, 48)`.
4. Press F8 and confirm the line disappears immediately.
5. Repeat teleport, dungeon entry/exit, return to the open world, window-mode
   changes, and normal process exit.
6. Reject the phase on any crash, duplicated frame presentation, stale line,
   unbounded retry, or new frame-time spike.

Static compilation is not runtime acceptance. Full marker migration starts
only after this gate passes.

## Phase 2 relative marker

The dev3 prototype selects the closest planar catalog treasure between 30 and
225 meters once after the first valid post-activation player sample. Target
selection runs on the existing game-thread
state path, not in `PostRender`. Subsequent 33 ms player samples publish only
atomic numeric fields: source sequence, player XY, target XY, validity, and a
22,500 cm test radius.

When `postrender_relative_marker_enabled=true`, `PostRender` reads that snapshot
with two bounded sequence checks and draws a yellow cross over the compact
minimap. Canvas `SizeX` and `SizeY` property offsets are resolved and validated
before hook installation; the callback performs direct fixed-offset scalar
reads without reflected name lookup.

This marker deliberately ignores save visibility and treasure type. It proves
only the native coordinate-to-Canvas path before bounded catalog rendering is
ported.

Dev3 also draws a temporary yellow witness cross at logical Canvas coordinate
`112,64` whenever a valid marker snapshot is consumed. If the witness is
visible but the projected compact-radar cross is not, the remaining blocker is
render/UI layer ordering rather than target selection, numeric publication, or
the reflected `K2_DrawLine` path. Remove this witness after that question is
answered.
