# DragonSword Mod Integration Status

Last updated: 2026-08-31.

## Maintained products

| Product | Version/boundary | Owner status |
|---|---|---|
| `DragonSwordNativeWorldRadarPostRender` | Native Radar `2.1.0` | Gameplay-tested and accepted |
| `DragonSwordNativeAutoPickup` | Native AutoPickup `1.3.0` | Gameplay-tested and accepted |
| `DragonSwordPickupRangeExpansion` | Optional authored range PAK variants | Maintained independently |
| `DragonSwordNativeAllMountsFreeFlight` | Pure-resource free-flight PAK | Maintained independently |
| `DragonSwordWorldDataProbe` | Independent read-only research/test Mod | Retained at root; currently not required to be active |

## Radar and AutoPickup mounted-flight observation

During one 2026-08-31 diagnostic session with both Mods enabled, the native
`F` interaction prompt temporarily disappeared and manual interaction was not
available. AutoPickup also stopped confirming actions for several seconds.
Continued flight was followed by recovery without a reported restart.

The captured evidence established:

- AutoPickup invoked its Enhanced Input route and recorded no injection API
  failures, but multiple actions timed out because the exact Component remained
  interactable instead of producing the expected confirmation transition.
- The semantic `INTERACT` keyboard binding resolved to `F` without a reported
  mapping conflict.
- The player Pawn changed to a mounted context while the recorded interaction
  owner and receiver identities remained stable.
- UE4SS recorded separate engine-tick hook registrations for Radar and
  AutoPickup and later `ClientRestart` lifecycle activity.
- Radar diagnostics recorded no native fault or ABI error. Source inspection
  found no Radar mutation of the game's `F` binding or native interaction UI.
- Another active mount-speed modification was present in the same session, so
  the run was not a controlled two-Mod isolation test.

The evidence therefore does not prove a direct Radar/AutoPickup hook conflict.
The strongest current hypothesis is a transient controller/Pawn/Enhanced Input
lifecycle mismatch around mounted flight and `ClientRestart`, with Radar at
most an unproven timing condition. This remains a diagnostic hypothesis, not a
claimed root cause.

## Acceptance decision

The owner subsequently reported completed gameplay testing and accepted the
current Radar and AutoPickup versions. The transient observation above is kept
for regression comparison but is not a release blocker.

This owner attestation does not assert that every prescribed checklist row has
an independently archived log, nor does it establish the exact SHA-256 identity
of an installed package unless an installed-hash receipt is separately stored.

## Public-source boundary

The workspace may publish first-party source and documentation to its existing
public GitHub repository. PostRender's local SQLCipher binary and generated or
derived game catalogs remain excluded from new public commits until their
provenance and redistribution review is complete. Functional acceptance does
not clear those independent rights questions.
