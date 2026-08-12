# Validation Record

Date: 2026-08-10

## Passed static and build gates

- Pinned RE-UE4SS v3.0.1 native build produced `main.dll`.
- `main.dll` SHA-256: `b3e4b54a13d48116aef5001c41e2b76ce5f922a1539d34c4b34d550e66dad991`.
- The isolated Overlay compiled from 41 C# source files.
- A 37-field compact protocol-v5 frame was accepted by the actual copied `MotionRecordParser`.
- The staged package contains 55 files and excludes copied `bin`, `obj`, PDB, project, Lua provider, and shared bridge-slot artifacts.
- Package ZIP SHA-256: `3a77ff01b72aaf55b02eb9126342082b5b2146e904813fd2d178289174b3b3f5` (1,316,086 bytes).
- Source policy found one bounded Engine `FindFirstOf`, zero continuous UObject scans, zero actor Tick hooks, and zero LoadMap hooks.

## Existing Radar preservation gate

The current `DragonSwordWorldRadar` non-build tree was fingerprinted before and after this project was created. Both checks returned 110 files and SHA-256 `83abcca2c80499e31a68c2f294d40ad4395d02c28b05a240bdd3fcad42386f45`.

## Unvalidated

No files were deployed to the game directory. No claim is made for in-game stability, marker accuracy, FPS, transition recovery, or visible behavior.
