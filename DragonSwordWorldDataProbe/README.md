# DragonSword World Data Probe

Independent read-only data-capture and research Mod for DragonSword: Awakening.

This project is intentionally retained at the repository root even when it is
not in active use. It is not a retired product and must not be moved to
`Archive/` solely because no probe profile is currently enabled.

## Boundary

- Collection must be bounded, read-only, explicitly whitelisted, and tied to an
  active profile and method matrix.
- Probe observations are evidence, not production truth or implicit permission
  to change another Mod.
- Do not write game state, invoke gameplay actions, retain unsafe object
  pointers, or collect secrets and keys.
- Keep runtime output and local captures outside Git.

Read `PROJECT_CONTEXT.md` and the applicable documents in `docs/` before
enabling or modifying a probe.

## License and tool boundary

First-party work is licensed under `GPL-3.0-only`; see [`LICENSE`](LICENSE),
[`THIRD_PARTY_NOTICES.txt`](THIRD_PARTY_NOTICES.txt), and the repository
[`LICENSE_SCOPE.md`](../LICENSE_SCOPE.md). Historical `PakReaderCore.exe` and
`ooz.exe` dependencies are local-only and excluded from that grant pending
separate provenance and redistribution clearance.
