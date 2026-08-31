# Project Context

## Role

`DragonSwordNativeAllMountsFreeFlight` is an independent pure-resource Mod that
enables the game's native free-dash/free-flight behavior for supported mounts.
It owns only that authored resource change.

## Boundaries

- Prefer the removable PAK implementation and leave the game's original
  `pakchunk*.pak` files untouched.
- Extracted/cooked game assets, analysis workspaces, backups, staging output,
  and local PAK builds remain outside Git.
- Source tooling and metadata must not embed game keys, credentials, or
  proprietary extracted payloads.
- Static artifact verification and owner gameplay acceptance are separate
  evidence states.
- Do not deploy, launch, or terminate the game as part of repository-only work.

## Repository status

This is a maintained root-level product. It is not an archive or a shared
runtime dependency of the other Mods.
