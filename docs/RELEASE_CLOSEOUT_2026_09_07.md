# Two-Mod Local Release Closeout - 2026-09-07

This handoff completes AutoPickup 1.3.1 and Native World Radar 2.3.0 packages
and their current installation / Nexus documentation. Historical attempts and
acceptance receipts are retained, not relabelled as current evidence.

| Product | Complete local set | Current evidence |
| --- | --- | --- |
| AutoPickup 1.3.1 | Three main ZIPs, optional standalone range ZIP, release JSON | [Release status](../DragonSwordNativeAutoPickup/docs/RELEASE_STATUS.md) |
| Native World Radar 2.3.0 | Three ZIPs, release JSON, SHA256SUMS | [Release status](../DragonSwordNativeWorldRadarPostRender/docs/RELEASE_STATUS.md) |

Canonical directories:
- `DragonSwordNativeAutoPickup/dist/releases/1.3.1/`
- `DragonSwordNativeWorldRadarPostRender/dist/final-2.3.0/`

Both Setup executables offer Install / Update / Repair key editing with
existing-value loading, explicit confirmation, preservation of other settings,
and transactional rollback. Uninstall keeps its separate ownership safeguards.
Runtime compatibility policies remain product-specific; unified UI does not
mean interchangeable native ABI support.

Nexus copy is indexed under each product's `assets/nexus/README.md`.
Use the main description, compact Quick Support, changelog, and file-upload
mapping from the same product/version. Versioned older screenshots remain
historical; do not rename them to imply screenshots of a new version.

## Completed verification

- AutoPickup: fresh native build, 95 hotkey assertions, 11/11 installer fixtures,
  deterministic packaging and four final ZIP hash/entry/config checks passed.
- Radar: retained receipt-bound CM-04 DLL, core/source gates, 177 hotkey
  assertions, 20/20 installer fixtures, 2/2 manual fixtures, and three final
  ZIP hash/payload/default checks passed.
- Both complete sets include the unified key-editing Setup, not the older
  EXE-only checkpoint. Public debug remains off. Product status files record
  exact hashes and verified pre-refresh backup locations.
- Corrected stale Pickup source-gate expectations for the Update label and
  expanded repair fixture; registered new tests/docs in the source manifest.
  No gameplay source was modified during this closeout.

No new gameplay tests, game deployment, Git publication, or Nexus uploads
were performed. Exact-package gameplay acceptance remains separate. Radar's
existing `BLOCKED_PENDING_RIGHTS_AND_SOURCE_PROVENANCE_REVIEW` publication
status is preserved; these local release files do not clear that blocker.
