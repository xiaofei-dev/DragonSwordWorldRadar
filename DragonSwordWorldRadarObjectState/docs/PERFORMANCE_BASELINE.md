# Performance Acceptance

No historical DragonSwordWorldRadar version is a mandatory baseline for this
project. Acceptance is based on equal-route runtime A/B evidence against F8 and
the currently installed production radar.

## Removed recurring work

- No periodic save database fingerprint.
- No periodic save copy/decrypt/SQL query.
- No compact player-motion bridge-file write.
- No recurring treasure actor scan.
- No hot-reload polling for configuration or treasure overrides.

## Added bounded work

- One native fresh-player coordinate sample every 33 ms while F7 is enabled.
- Constant-size shared-memory publication after a valid sample.
- BeginPlay/EndPlay callbacks with an early class/hash rejection path.
- At most one exact-class catch-up enumeration per class and F7 activation,
  released only near a matching catalog point and at no more than one class per
  250 ms.
- Exactly one background save reconciliation attempt per F7 activation.

## Required measurements

Use equal locations, camera behavior, route duration, graphics settings, and
foreground state. Record at least five minutes after warm-up for each mode.

- F7 frametime distribution and FPS.
- F8 frametime distribution and FPS.
- Counts and maximum duration of native class catch-up operations.
- Native position sample cadence and missed/invalid intervals.
- Motion Bridge write count; steady compact movement must not increase it.
- SQL sync count; it must be exactly one per F7 activation.
- Travel/dungeon transitions and event epoch mismatches.
- Overlay/DWM GPU and CPU cost, which this iteration does not remove.

## Acceptance rules

- No crash, stale event, marker resurrection, or infinite retry is acceptable.
- No periodic SQL or repeated class enumeration is acceptable.
- One-off native catch-up must not create a user-visible hitch.
- Static gates and clean logs do not prove smoothness.
- If F8 remains materially smoother after state collection is proven bounded,
  the remaining target is external transparent rendering, not additional state
  polling changes.

