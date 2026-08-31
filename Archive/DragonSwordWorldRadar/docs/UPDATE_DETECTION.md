# Game update detection

`Install.cmd` records:

- executable file/product version;
- executable size and UTC modification time;
- source PAK size and UTC modification time;
- a SHA-256 fingerprint of those fields.

The watcher checks the same fields when UE4SS requests the overlay. A mismatch writes `runtime/reinstall-required.json`, displays a reinstall prompt, and skips startup. Re-running `Install.cmd` regenerates every registered dataset and replaces the stored fingerprint.
