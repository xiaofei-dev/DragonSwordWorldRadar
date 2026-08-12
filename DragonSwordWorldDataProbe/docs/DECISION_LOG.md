# Decision log

- Boss functionality is complete and is not modified by this framework.
- Task completion was investigated but is not required for assault markers.
- Full-map Actor memory scanning was rejected because world partition does not load the entire map.
- `ReceiveDestroyed` and `ReceiveEndPlay` were rejected after repeatable UE4SS native crashes.
- A mission-controller hook and UI fields did not yield a complete global list.
- The existing Radar has already demonstrated stable reads of the required save-state tables. This framework preserves the legacy reader source but does not claim a universal SQLCipher solution or replace the Radar tracker.
- Static PAK extraction is the correct catalog path.
- Adjacent encoded-entry recovery produced false positives and is forbidden.
- The exact 12-byte uncompressed compact entry for `RevealCycleData.xml` is now decoded and validated without guessing.
- Assault research collection is closed for production implementation: static 40-target identity plus `tb_actor_respawn` supplies the ordinary target loop.
- RevealCycle row 10001 is confirmed as 23:00-06:00. Applying it to CID 143 is accepted only as a clearly labeled strong inference until a direct generator-asset key binding is recovered.
- Weather is excluded from the production Assault model because no target-specific weather dependency was established.
- Further `MonsterSpawnBase` enumeration is prohibited because the first successful enumeration was followed by a UE4SS-path native crash.
