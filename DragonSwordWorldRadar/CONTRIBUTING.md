# Contributing

- Treat Motion protocol v4 as a versioned 35-field wire format. Any field-order or semantic change requires a new protocol version and parser migration.
- Do not restore the active Static JSON Bridge or duplicate live state across two IPC channels.
- Preserve F7/F8 behavior and independent treasure, Boss, Mole/Fly, and world-status controls.
- Keep the dev25 50 ms active producer/consumer cadence, 250 ms window sampling, and 250 ms control cadence coherent unless an isolated in-game A/B test proves a benefit.
- Run the Windows PowerShell 5.1 source compile gate before publishing.
- Add aggregate diagnostics for new hot paths; do not write per-frame diagnostic lines.
- Do not place experimental probes or broad UObject/Character scans in production radar modules.
- Keep the game-time read scalar, read-only, cached, and bounded. Do not reintroduce weather sampling until a changing source and semantic mapping are independently verified.
- Update `CHANGELOG.md`, `metadata/release.json`, and the source manifest for every published version.
