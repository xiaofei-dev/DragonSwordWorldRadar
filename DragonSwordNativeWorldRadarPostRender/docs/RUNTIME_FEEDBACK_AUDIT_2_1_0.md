# Runtime Feedback Audit for 2.1.0

Version 2.1.0 is a corrective release for feedback received after the public
2.0.0 release. This document separates implemented corrections from runtime
acceptance for the exact 2.1.0 artifacts.

| Feedback | 2.1.0 disposition | Remaining acceptance |
| --- | --- | --- |
| Expanded-map markers are offset in windowed, 21:9, or 16:10 layouts | Current-source correction revised, not runtime-accepted. Projection uses the exact witnessed map Canvas parent-local cached Slate geometry, scales X/Y by its live local width/height, treats `WorldMapUISize` as metadata, and rejects invalid geometry without a centered or desktop-resolution fallback. | Package and deploy the current `D4EE...` build, then validate windowed 16:9, 16:10, native 21:9, and a 4K viewport with internal 21:9 black bars while panning and zooming. A numeric 3840x1600 unit input is not this proof. |
| Wheel zoom flickers, stalls, offsets, or temporarily removes expanded-map markers | Current-source correction revised, not runtime-accepted. The atlas-local placement correction adds no new parent-size post-check, growth rejection, or extent-change allowance and continues the existing witnessed stable-geometry and bounded map/zoom reconstruction strategy. | Confirm continuous wheel zoom and repeated window/aspect transitions produce no visible flicker/offset or steady work. Verify the current-build geometry and reconstruction evidence. |
| Bird Egg active sampling competes with other runtime object work | Implemented structurally. The nearest-16 exact-state service now shares the existing 250 ms discovery edge instead of owning a second 100 ms timer, reducing its maximum state-query rate from 160 to 64 reads per second. Exact EndPlay remains immediate and the 400 ms unavailable debounce remains bounded. | Confirm eggs still appear and clear, and compare external frame-time evidence; this structural reduction alone is not gameplay performance acceptance. |
| Assault ALL still follows defeat or cooldown state | Implemented. ALL is now a static presentation view of all 40 Assault records and ignores save readiness, time, defeat, and cooldown. AVAILABLE and every completion/cooldown write path are unchanged. | Toggle ALL to AVAILABLE after defeating an Assault and confirm the completed entry is visible only in ALL. |
| Cooking or delivery area quests sometimes remain visible after completion until F8/F7 | Implemented. Exact runtime `COMPLETE_CNT` growth remains authoritative. Tasks without a useful disappearance event receive one immediate and at most two delayed positive-only exact-ID save confirmations on the existing below-normal worker. This is event-driven, not periodic SQL. | Complete representative cooking and delivery tasks and confirm the icon clears without F8/F7. |
| Intermittent gameplay stalls | Static inspection keeps layout transitions on the existing bounded reconstruction path. This is not runtime hitch attribution or frame-time acceptance. | Keep Pickup disabled for the next comparison, capture current-build Radar diagnostics plus external frame times, and separate event-local attach cost from steady gameplay. |

## Automated evidence boundary

The current source passed native core/static gates, a clean `/WX` build, Setup
`20/20`, manual-copy `2/2`, payload equivalence, and archive re-extraction with
`main.dll` SHA-256
`D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
It is sealed in the authoritative `dist/final-2.1.0` package. A local 2.1.0
diagnostic installation later produced runtime evidence, and the owner reported
completed gameplay testing and acceptance on 2026-08-31. Older `4AFE...` and
`BDE21...` package sets are historical and non-authoritative. The tested
installed DLL was not independently hash-matched to `D4EE...`; source, static,
build, installer, and package evidence do not fill that separate receipt.

## 2026-08-31 integration observation and owner decision

One session with AutoPickup also enabled showed a temporary native `F` prompt
loss and missing AutoPickup confirmations during mounted flight, followed by
recovery during continued flight. Radar recorded no native fault or ABI error,
used a separate engine-tick hook, and source inspection found no Radar mutation
of the game's `F` binding or interaction UI. The same session contained
Pawn/`ClientRestart` lifecycle changes and another mount-speed modification.
The evidence does not prove a direct Radar conflict; see the repository-level
`docs/INTEGRATION_STATUS.md`.

The owner subsequently reported completed gameplay testing and accepted the
current Radar and AutoPickup versions. The observation is retained for future
regression comparison and is not an acceptance blocker.

Status: `GAMEPLAY_ACCEPTED = OWNER_ACCEPTED_2026_08_31`.

Status: `INSTALLED_ARTIFACT_HASH = NOT_RECORDED`.

## Prescribed owner matrix

The owner acceptance above is recorded as an attestation. Individual items
below remain the preferred reproducible evidence matrix and are not silently
claimed as separately archived logs.

1. Open the expanded map and continuously zoom from minimum to maximum and back
   for at least 20 seconds.
2. Repeat in windowed 16:9, 16:10, and 21:9 layouts; pan before and after zoom.
3. Confirm no marker flicker, temporary disappearance, or position offset.
4. Confirm the five deadlines take one observation per due pass, the first four
   are read-only, and layout transitions remain within the existing witnessed
   stable-geometry and bounded reconstruction strategy.
5. Complete one cooking and one delivery area quest and confirm both icons clear
   without F8/F7.
6. Compare gameplay with Pickup disabled before attributing any remaining stall
   to Radar.
