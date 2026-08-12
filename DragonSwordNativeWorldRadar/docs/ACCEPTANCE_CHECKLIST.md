# Gameplay Acceptance Checklist

Run this only after explicit deployment approval. Disable `DragonSwordWorldRadar` before enabling this proof-of-concept.

1. Start the game and confirm no Overlay is visible before F10.
2. Press F10 in a stable open-world scene and confirm markers appear after the stability delay.
3. Compare stationary and moving FPS against the current accepted Radar baseline.
4. Open and close menus and confirm the Overlay suppression policy remains correct.
5. Enter and leave a dungeon, hunt, and story transition at least three times each.
6. Confirm the Overlay hides during invalid coordinate windows and returns only after `WORLD_READY`.
7. Confirm no crash, increasing stutter, duplicate host, or stale marker layer occurs.
8. Inspect `runtime/logs/DragonSwordNativeWorldRadar.Native.log` and both Overlay logs.

This version does not draw world-map markers and does not display live game time/weather. Those are follow-up gates, not accepted omissions for a production replacement.
