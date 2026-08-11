# Owner Runtime Canary Procedure

1. Review and install only the exact authorized package while the game is closed.
2. Start with Radar and AutoPickup enabled; confirm `READY` and trusted fingerprints.
3. Press F9, then activate Radar with F7. Confirm no queue stall or crash.
4. Confirm `raw_hook_callbacks` and `accepted_pulses` increase, with throttle rejects expected between accepted 150 ms pulses.
5. Approach ordinary ground loot and confirm exactly one `AUTO_PICKUP` per nearby item.
6. Verify outside-radius loot and excluded interactables remain untouched.
7. Travel through open world, dungeon, menu, and cutscene states. Confirm `WORLD_RESET`, no stale action, and recovery only after a fresh accepted input-frame pulse.
8. Press F9 to disarm and confirm automatic pickup stops immediately.
9. Capture AutoPickup/Radar user and debug logs, crash evidence, and enabled/disabled frametime.

This procedure supplies owner evidence only for the exact tested package and fingerprints.
