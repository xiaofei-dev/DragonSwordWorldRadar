Experimental legacy save-database reader source

This source is retained for future save-table research and historical reproducibility.
It is not registered in config/modules.json and is never compiled or loaded by the default profile.

Why it is not a default module
- It reads another process and depends on an available SQLCipher runtime.
- It contains game-build-specific patterns and fallback RVAs.
- The current Radar already has a working tb_actor_respawn availability tracker; duplicating that tracker would create conflicting refresh/state paths.

Use only after reviewing the current game build and replacing hard-coded Boss-specific behavior with a generic read-only table-export interface.
