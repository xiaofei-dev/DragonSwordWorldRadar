# Mod Release Acceptance Checklist

Copy this file into the target project and replace all placeholders. Record
exact paths, versions, hashes, UTC timestamps, and test counts.

## Candidate identity

- [ ] Product/version/runtime label agree everywhere.
- [ ] Supported game executable and other identity inputs have exact hashes.
- [ ] Product profile contains no placeholder or borrowed product value.
- [ ] Public defaults and load-control policy are explicit.
- [ ] Third-party redistribution rights/notices are complete.

## Source and build

- [ ] Source manifest exactly matches the publishable tree.
- [ ] Static lifecycle/thread/performance/fail-closed invariants pass.
- [ ] Every runtime and installer source set compiles through its real toolchain.
- [ ] Unit, refactor, protocol, mapping, and data-shape tests pass.
- [ ] Every required ABI/architecture artifact is independently built and hashed.
- [ ] Build output contains no local debug configuration, runtime state, or secret.

## Installer

- [ ] Exact executable selection is enforced.
- [ ] StableRoot and ExperimentalNested routing tests pass when supported.
- [ ] Missing UE4SS bootstrap uses only the pinned official archive.
- [ ] Ambiguous, unknown, missing-proxy, and wrong-proxy cases make zero mutation.
- [ ] Load-control update preserves unrelated bytes and leaves one authority.
- [ ] User configuration and overrides survive a tested upgrade.
- [ ] Optional components install/remove only exact product-owned artifacts.
- [ ] Unknown same-name and legacy conflicts fail closed.
- [ ] Reparse/junction target is rejected.
- [ ] Injected late failure rolls back every recorded mutation.
- [ ] Exact expected pass/fail/skip counts are enforced by the release builder.

## Public package

- [ ] Staging begins from an empty validated project-owned directory.
- [ ] Public file allowlist and count match the release contract.
- [ ] ZIP paths are relative, safe, and case-insensitively unique.
- [ ] ZIP entry size/hash bytes match staging and the release manifest.
- [ ] Installer checksum is included and verified.
- [ ] Signed/unsigned status is documented accurately.
- [ ] No logs, diagnostics, generated local data, backups, user config, old canary,
      obsolete payload, or unowned third-party Mod is present.
- [ ] Final archive size and SHA-256 are recorded.

## Local deployment

- [ ] User explicitly authorized deployment and the game was closed.
- [ ] Exact supported loader/proxy layout was revalidated immediately before write.
- [ ] Backup path and touched-file inventory were recorded.
- [ ] Installed runtime files match candidate hashes.
- [ ] Installed configuration differs from public defaults only by recorded local
      overrides.
- [ ] Load-control state is unique and verified.
- [ ] Optional component state/hash matches the selected choice.
- [ ] Rollback procedure is available.

## Gameplay acceptance

- [ ] Cold launch/default state behaves as documented.
- [ ] Every control and configurable key behaves once per input.
- [ ] Core behavior works for every supported object/activity/context.
- [ ] Optional component On and Off behavior is independently verified.
- [ ] Menus, cutscenes, dungeons, teleports, world travel, and return paths are safe.
- [ ] Repeated enable/disable and open/close operations do not accumulate work.
- [ ] Same-session performance comparison meets the product target.
- [ ] Long-session logs contain no exception, unbounded retry, stale object access,
      dropped work, or unexpected automatic disable.
- [ ] Clean game exit and restart succeed.
- [ ] Owner accepted the exact installed hashes.

## Publication

- [ ] Repository diff is scoped and reviewed.
- [ ] Commit/tag/release notes identify the exact artifact hashes.
- [ ] Explicit authorization exists for commit, push, and public upload.
- [ ] Published download was re-downloaded and hash-verified.
