# Public Source and Local Publishing Materials

Effective 2026-09-08, at the owner's request, GitHub carries implementation
source, tests, build/configuration scripts, applicable licenses, engineering
documentation, and required first-party runtime UI assets. Nexus publishing
materials are not source deliverables.

## Local only

- Every product's `assets/nexus/`: descriptions, FAQs, changelogs for posting,
  covers, banners, galleries, screenshots, and upload variants.
- `assets/screenshots/`, including the retired Radar's gameplay screenshots.
- `NEXUS_*.txt` / `NEXUS_*.md`, including the historical Nexus feedback audit.
- DataProbe's extracted `reference/` datasets and `metadata/world-boss-catalog.json`.
- Existing excluded release ZIPs, built binaries, runtime logs, build caches,
  SDK payloads, SQLCipher binary, generated game catalogs, credentials, saves,
  and local deployment state.

Keep these files locally. Use index-only removal (`git rm --cached`) for
previously tracked material; never delete the working copies as cleanup.
Local Nexus editing and package workflows may continue to use the local files.
The AutoPickup source manifest no longer requires marketing files, so source
verification does not depend on their presence in a public checkout.

## Keep tracked

Source and tests are not removed merely because they load data or mention
Nexus. Keep installation/build instructions, engineering attempt ledgers,
source metadata, changelogs, licenses, and required first-party F6 text-overlay
TGAs plus their manifest. Do not broadly exclude all images or all metadata.
The frozen Archive remains a source reference, not a supported active build.
Historical snapshot manifests remain dated records, not current-tree promises.

Research modules requiring excluded datasets need independently obtained local
inputs. A public checkout is not a complete reproduction of every binary
release. No third-party rights are granted by moving files out of tracking.

## Before committing

Run `powershell -NoProfile -File docs/Test-PublicSourceBoundary.ps1` after
staging. The check reads the Git index and rejects local-only paths while
preserving required UI assets. Do not bypass it with `git add -f`.
Also run the affected product's source-manifest verification and `git diff --check`.

This cleanup changes the current main tree and future tracking only. Older
commits may still contain the removed files. No history rewrite or force-push
is authorized or performed by this cleanup.
