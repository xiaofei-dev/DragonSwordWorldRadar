# Contributing

- Keep the 21-field motion protocol backward compatible unless a versioned migration is introduced.
- Preserve F7/F8 behavior and independent treasure/Boss layer controls.
- Run the Windows PowerShell 5.1 source compile gate before publishing.
- Add aggregate diagnostics for new hot paths; do not write per-frame diagnostic lines.
- Do not place experimental probes or broad UObject/Character scans in the production radar modules.
- Update `CHANGELOG.md` and `metadata/release.json` for every published version.
