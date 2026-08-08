# Contributing

- Treat Motion protocol v2 as a versioned 27-field wire format. Any field-order or semantic change requires a new protocol version and parser migration.
- Do not restore the active Static JSON Bridge or duplicate live state across two IPC channels.
- Preserve F7/F8 behavior and independent treasure/Boss layer controls.
- Keep the 24 ms motion cadence and 250 ms control cadence unchanged unless an isolated in-game A/B test proves a benefit.
- Run the Windows PowerShell 5.1 source compile gate before publishing.
- Add aggregate diagnostics for new hot paths; do not write per-frame diagnostic lines.
- Do not place experimental probes or broad UObject/Character scans in production radar modules.
- Update `CHANGELOG.md`, `metadata/release.json`, and the source manifest for every published version.
