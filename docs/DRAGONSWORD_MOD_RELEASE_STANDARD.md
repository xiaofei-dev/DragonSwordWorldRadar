# DragonSword Mod Release and Acceptance Standard

## Release objective

A release is a reproducible mapping from reviewed source to a small declared
artifact set. It is not a copy of a developer's installed Mods directory and it
must not inherit runtime logs, generated local data, backups, secrets, debug
overrides, or unrelated third-party Mods.

## Gate order

Run gates in this order and stop at the first failure:

1. **Source inventory** - current source manifest exactly matches publishable
   repository files.
2. **Product contract** - version, compatibility fingerprints, public defaults,
   load-control policy, optional components, and third-party notices agree.
3. **Static invariants** - forbidden runtime routes and required lifecycle,
   thread, performance, and fail-closed rules are checked.
4. **Compilation/tests** - compile every source path used by installation or
   runtime and run unit/refactor/protocol/data tests.
5. **Variant artifacts** - independently build and fingerprint every required
   UE4SS ABI or architecture variant.
6. **Installer build** - embed only verified payloads and emit the installer
   checksum.
7. **Installer matrix** - require exact pass/fail/skip counts before staging.
8. **Release staging** - copy an explicit allowlist into a fresh, validated
   staging directory.
9. **Archive audit** - verify safe relative paths, case-insensitive uniqueness,
   file count, size, hash, and exact ZIP-to-staging byte identity.
10. **Local deployment** - only with explicit authorization and while the game
    is closed; back up and verify the installed state.
11. **Gameplay acceptance** - owner tests the exact installed artifact.
12. **Publication** - commit, tag, push, or upload only after separate explicit
    authorization.

## Public artifact shape

For an installer-first release, prefer:

```text
<ModName>-v<Version>.zip
  <ModName>-Setup-<Version>.exe
  <ModName>-Setup-<Version>.exe.sha256
  INSTALL.md
  THIRD_PARTY_NOTICES.txt
```

Runtime payloads and optional PAKs may be embedded in Setup. Do not also expose
loose copies unless a documented manual-install channel requires them and the
archive audit covers both routes.

When both channels are required, publish two independently audited archives:

```text
<ModName>-v<Version>-Installer.zip
  <ModName>-Setup-<Version>.exe
  <ModName>-Setup-<Version>.exe.sha256
  INSTALL.md
  THIRD_PARTY_NOTICES.txt

<ModName>-v<Version>-Manual-<Layout>.zip
  <declared runtime tree>
  README.md
  SHA256SUMS.txt
  THIRD_PARTY_NOTICES.txt
```

The manual archive name must identify any layout/ABI restriction. Its README
must state the exact supported loader layout, load-control file, preserved
configuration behavior, upgrade/uninstall procedure, and the fact that manual
copying cannot bootstrap or repair an unsupported loader. The release manifest
must bind both archives and prove that their runtime payloads match the Setup
resources for the same variant.

The release manifest should record:

- version and generation UTC;
- installer-first and signed/unsigned status;
- supported layouts;
- public defaults;
- every native/runtime payload size and SHA-256;
- UE4SS bootstrap version/hash when included;
- each optional component and ownership boundary;
- installer-exposed runtime choices, including semantic input mode and explicit
  fallback defaults when applicable;
- installer test counts;
- every public file size/hash;
- final archive size/hash;
- static and gameplay acceptance states.

## Public versus local diagnostics

Package the public default first. Deploy from that exact package. If the owner
needs diagnostics, change only the installed configuration and record its hash.
Never rebuild the public archive from a locally modified configuration.

Before final publication, decide whether diagnostic controls are part of the
product or only an acceptance aid. Turning Debug Off is a product-specific
change and requires at least a configuration/package rerun; it does not erase
the prior diagnostic evidence.

## Versioning

- Use a stable public version such as `1.0.0` only for the artifact intended for
  users.
- Keep internal experiments and rejected attempts distinct from the public
  identity.
- One version maps to one runtime marker, installer file version, package name,
  documentation state, and release manifest.
- Rebuilding changed bytes under the same version invalidates the old artifact
  hash and requires a new candidate record. Do not silently reuse old evidence.

## Evidence language

Use exact terms:

| State | What it proves |
|---|---|
| `SOURCE_VALIDATED` | Reviewed source and declared inventory passed static gates |
| `BUILT` | Compiler/linker completed for a specific artifact |
| `PACKAGED` | Declared public archive was produced and audited |
| `INSTALLER_TESTED` | Isolated filesystem scenarios passed for the exact installer |
| `DEPLOYED` | Exact installed files and load control were verified |
| `GAMEPLAY_ACCEPTED` | Owner observed required behavior for the exact installed artifact |

Do not convert `BUILT`, `PACKAGED`, clean logs, or successful `ProcessEvent`/
input return values into a gameplay claim.

## Required gameplay checklist classes

Each project must specialize the shared template, but normally cover:

- cold launch and default enablement state;
- all user controls and custom keys;
- normal, mounted, menu, cutscene, dungeon, travel, and world-map contexts that
  the product supports;
- optional component On/Off behavior;
- upgrade preservation and complete disable/unload control;
- frame-time and memory behavior over a long session;
- clean exit and restart;
- logs containing no new exceptions, repeated retries, dropped work, or stale
  cross-world references.

## Publication boundary

Keep the final archive available locally for owner testing, but do not infer
permission to commit, tag, push, create a GitHub release, or upload the artifact.
Those are separate external mutations and require explicit authorization.
