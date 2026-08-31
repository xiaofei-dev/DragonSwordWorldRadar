# Manual DropItemActor Capture

> Historical diagnostic procedure only. Version 1.3.0 does not use this
> capture procedure; current release QA is defined in
> `ACCEPTANCE_CHECKLIST.md` and `RELEASE.md`.

## Evidence correction

The accepted 0.5.1 trace proved the manual chain only for the observed `Vitality_Leave_01_C` interaction. It did not prove a general ordinary `DropItemActor` contract. Versions 0.5.2-0.5.4 incorrectly generalized that evidence and are rejected as implementation bases.

## 0.6.0 procedure

1. Install only after explicit authorization and record the staged DLL hash.
2. Start on foot in a stable open-world scene.
3. Place exactly one ordinary ground drop within 4.5 m; move other drops away.
4. Press and release F9 once.
5. Require `DIAGNOSTIC_ARMED`, `DISCOVERY_COMPLETE`, and `DIAGNOSTIC_TARGET_LOCKED`.
6. Manually collect that same locked item through normal game interaction once.
7. Do not interact with NPCs, chests, resources, or another drop during the window.
8. Wait at least five seconds, then press F9 Off or allow the 60-second timeout.
9. Exit normally and preserve the user and debug logs.

## Decisive evidence

- locked owner and component weak identities;
- candidate class, World, location, state fields, and distance;
- correlated overlap and interaction pre/post ordering;
- receiver and parameter identities;
- receiver target fields before and after each call;
- `AniPickUp` and `SetDestroy` involvement, if any;
- exact owner/component invalidation or state change;
- `actions_invoked=0` throughout the session.

This trace defines only the exact observed drop category. Mounted play and other interactable categories require separate traces.
