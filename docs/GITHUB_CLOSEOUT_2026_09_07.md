# AutoPickup 1.3.1 and Radar 2.3.0 - Source Closeout

The owner authorized a complete source/documentation sync to the existing
GitHub main branch on 2026-09-07 after completing the Nexus page corrections.
This follows the earlier local package closeout, not a new binary release or
a rewrite of its dated receipts.

## Included scope

- AutoPickup startup-readiness correction, unified installer key editing,
  tests, packaging scripts, and 1.3.1 documentation.
- Radar mounted-flight map attachment, controller-menu hiding, persistent
  AUTO language selection, configurable keys, unified installer workflow,
  tests, and 2.3.0 documentation.
- Evergreen Nexus Full Description and Quick Support for both products.
  Pickup file copy matches the corrected public text; file descriptions
  must remain within 255 characters. Radar's local installer description
  explicitly mentions Install / Update / Repair key editing; that optional
  wording was not present on the public page at the last visitor check.

## Visitor-page verification

The latest visitor audit confirmed the corrected Pickup Range PAK and
No-UE4SS descriptions and the complete Quick Support ending. The preceding
audit checked versions, main descriptions, files, and changelogs for both
products. These were page-content checks, not downloads or gameplay tests.

## Fresh offline checks for this source sync

- AutoPickup Verify-Source: selector/source gates, 115-file manifest,
  package-layout and mods.txt tests, and core build/test passed (1/1).
- Radar isolated core build/tests passed (2/2). The default sandbox could
  not write the build directory; the approved isolated rerun passed.
- Radar compact-renderer, world-map, PostRender safety, and release-hygiene
  source gates passed, including localized overlay asset verification.
- Existing release Setup key tests passed: Pickup 95/95; Radar 177 assertions.
- Changed-text credential-pattern scan and Git whitespace check passed.

The previous full packaging receipts remain in the product RELEASE_STATUS
files: Pickup installer 11/11 and Radar installer 20/20/manual 2/2. Those full
matrices were not rerun as part of this source-only sync.

## Boundaries

No new gameplay changes, game deployment, native Mod binary rebuild, or binary
upload are introduced by this closeout. Existing package hashes remain
authoritative. Exact-package and all-device gameplay acceptance remain
separate from source checks and the owner's request to finish development
pending further reports.

Only eligible source, tests, first-party assets, metadata, and documentation
are included. Runtime logs, build outputs, release archives, generated game
catalogs, databases, and unreviewed third-party payloads remain excluded.
The existing rights/provenance publication gate is not cleared by this sync.

The initial fetch found main equal to origin/main and no other branch heads.
GitHub CLI API requests returned HTTP 401, while Git fetch and push dry-run
succeeded. API-only PR/Actions status must not be reported as verified.
The actual commit and remote SHA comparison are reported after the push;
this pre-commit document does not itself assert successful remote delivery.
