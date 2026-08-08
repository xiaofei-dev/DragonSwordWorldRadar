# 1.0.32 crash hardening

The 1.0.30 crash diagnostic ended after `assault_condition_transition` was queued
and before its completion record. The prior implementation performed 80 native
ProcessEvent calls in one step (40 PlaceIDs x two ISAll modes).

1.0.32 reduces this to 10 calls per step:

- 10 PlaceIDs per batch;
- ISAll=false only;
- four batches for a complete 40-ID sweep;
- fresh DETUtil/UFunction/WorldContext handles for every batch;
- IsValid checks immediately before every call;
- first call failure disables the module for the rest of the session.

The previously added Quest Trigger and weather runtime probes remain physically
preserved but are not scheduled in game.

## RespawnCycle 105

The XML from the crash diagnostic proves:

- ID = 105
- RespawnType = DAILY
- RespawnRealTime = 0
- no explicit ConditionType/ConditionValue fields

The old parser missed it because all XML attributes are namespace-prefixed.
1.0.32 parses attributes by LocalName.

This proves the 40 assault target actors share a DAILY respawn cycle, but does not
yet prove which local reset boundary the game's DAILY rule uses.
