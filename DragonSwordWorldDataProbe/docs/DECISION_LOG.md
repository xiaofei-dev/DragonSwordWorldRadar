# Decision log

- Boss functionality is complete and is not modified by this framework.
- Task completion was investigated but is not required for assault markers.
- Full-map Actor memory scanning was rejected because world partition does not load the entire map.
- `ReceiveDestroyed` and `ReceiveEndPlay` were rejected after repeatable UE4SS native crashes.
- A mission-controller hook and UI fields did not yield a complete global list.
- The existing Radar has already demonstrated stable reads of the required save-state tables. This framework preserves the legacy reader source but does not claim a universal SQLCipher solution or replace the Radar tracker.
- Static PAK extraction is the correct catalog path.
- Adjacent encoded-entry recovery produced false positives and is forbidden.
- The remaining static blocker is an exact encoded-entry decoder for Kind/Place/RespawnCycle entries.
