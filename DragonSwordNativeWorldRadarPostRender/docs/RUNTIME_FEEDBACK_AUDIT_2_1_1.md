# Runtime Feedback Audit for 2.1.1

Version 2.1.1 is the corrective patch for controller-menu suppression and the
Area Quest height-arrow feedback. It is a patch release rather than 2.2.0
because it repairs existing behavior and presentation without adding a new
user-facing feature family or configuration contract.

The accepted 2.1.0 history remains owned by
`docs/RUNTIME_FEEDBACK_AUDIT_2_1_0.md`. This document does not rewrite or
relabel that evidence.

The 2.1.1 single-height Area Quest contract documented here is historical and
has been superseded by the 2.2.0 one- or two-band profile. Current task-height
acceptance belongs exclusively to `RUNTIME_FEEDBACK_AUDIT_2_2_0.md`; no
historical automated result below validates the 2.2.0 model.

## Feedback disposition

| Feedback | 2.1.1 source disposition | Static evidence | Runtime evidence still required |
| --- | --- | --- | --- |
| Controller-opened map or pause menu can leave the compact radar visible | The exact `SetWorldMapImage` post event immediately latches world-map suppression. `IsVisible` is queried only while latched or in a bounded catch-up. Optional `IsGamePaused` sampling shares the existing 250 ms activity probe. The 16 ms path reads booleans only; no controller mapping is read and no timer, scan, query, or allocation was added. | `PASS`: the complete source gate checks the exact function names, ABI validation, lifecycle resets, catch-up boundaries, pure suppression predicate, and edge-only diagnostics. | `NOT_VALIDATED`: use a real controller to open/close the map and pause UI, including F7 and travel recovery, and capture external frame-time/hitch evidence. |
| Area Quest arrows use the wrong task height | Version 2.1.1 introduced a separately sourced single-height precursor and prohibited marker-Z fallback. The current 2.2.0 release replaces that data shape with 144 height profiles, one genuine two-band profile, and three no-source rows. | `PASS` applies only to the archived 2.1.1 source gate; it is not evidence for the 2.2.0 height-band generator or renderer. | `NOT_VALIDATED`: historical 2.1.1 task-height gameplay was never accepted, and current height-band behavior must be tested through the 2.2.0 matrix. |
| Area Quest and treasure height arrows look too similar | Area Quest uses a shaftless chevron with black outline and white fill by collapsing the two shaft pieces in the existing fixed six-piece pool. Treasure keeps its colored fill and shaft. | `PASS`: the source gate checks the Area Quest visual branch, fixed-piece reuse, shaft collapse, and preserved treasure branch. | `NOT_VALIDATED`: capture in-game up/down examples of both marker categories and confirm placement, contrast, and visual distinction. |
| Deployment could overwrite a player's treasure override | Deployment validates both source and installed override files, preserves an existing installed `treasure_overrides.txt`, detects a staging-time change, restores the preserved file, and verifies its SHA-256 byte identity. The shipped default remains the single `ignore 11230106` rule. | `PASS`: the source gate checks the single default override, exact actor/render catalog delta, preservation path, validation, race guard, restore behavior, and final hash comparison. | `PASS`: the exact 2.1.1 debug deployment retained SHA-256 `CD52EE5C006B99BBF32ACDFE3ADD301507FDFD6C3DA7249BA6C3B6E6DB00BA43` before and after deployment and installed the receipt-bound DLL. |

## Evidence ledger

The statuses below include the final automated 2.1.1 release receipt. They do
not advance gameplay or performance status without separate runtime captures.

| Evidence layer | Status | Evidence available | What it does not prove |
| --- | --- | --- | --- |
| Source | `PASS` | The controller latch/provider, task-height catalog contract, distinct arrow branch, and override-preserving deployment path are present in the reviewed 2.1.1 source. | Compilation, archive contents, installed bytes, or game behavior. |
| Static | `PASS` | Core tests and all four checked-in source gates pass, including the complete controller, height-catalog, arrow-style, and deployment-preservation assertions. | Deployment, controller behavior, visual correctness, or frame time. |
| Build | `PASS` | Clean native `/W4 /WX` build receipt; packaged `main.dll` SHA-256 `B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`. | Installed loadability or game behavior. |
| Package | `PASS` | Setup `20/20`, manual-copy `2/2`, payload equivalence, clean-target validation, and all three archive re-extractions pass in `dist/final-2.1.1`. | Installed artifact identity or gameplay. |
| Deploy | `PASS` | Local debug deployment installed exact `B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`; one Radar load line is enabled, no predecessor remains, diagnostics are enabled only in the installed copy, and `treasure_overrides.txt` retained SHA-256 `CD52EE5C006B99BBF32ACDFE3ADD301507FDFD6C3DA7249BA6C3B6E6DB00BA43`. | Game load, controller suppression, arrow behavior, or performance. |
| Gameplay | `NOT_VALIDATED` | No real-controller session, Area Quest visual capture, or external frame-time capture is attached to this audit. | Nothing at runtime is accepted by inference from lower evidence layers. |

## Debug gameplay acceptance matrix

| Check | Required observation | Current status |
| --- | --- | --- |
| Provider readiness | Debug startup reports bounded readiness for the world-map visibility and pause providers, with no recurring failure spam. | `NOT_VALIDATED` |
| Controller world map | With the hardware cursor hidden, opening the map produces one world-map suppression edge and hides the compact radar; closing it clears the edge and restores eligible rendering. | `NOT_VALIDATED` |
| Controller pause | Opening the controller pause/menu UI produces a known paused edge and hides the compact radar; closing it produces the matching unpaused edge without sticky suppression. | `NOT_VALIDATED` |
| Lifecycle recovery | Repeat map and pause checks across F7 disable/enable and travel; stale weak layers and old latches must not suppress the new world. | `NOT_VALIDATED` |
| Performance | Compare external frame-time/hitch captures before and after the change while idle, moving, opening/closing the map, and pausing. Confirm no new timer, per-frame reflection, scan, allocation, or recurring log. | `NOT_VALIDATED` |
| Historical task height | The superseded 2.1.1 single-height cases are retained only as artifact history; do not use them as current release acceptance. | `NOT_VALIDATED` (historical; superseded by 2.2.0) |
| Current task height | Use the 2.2.0 matrix to exercise inside-band, below-all, above-all, between-band, and no-source states. | `NOT_VALIDATED` in 2.2.0 |
| Arrow distinction | Capture Area Quest and treasure up/down arrows together where practical. Area Quest must be black/white and shaftless; treasure must retain its colored shafted style. | `NOT_VALIDATED` |
| Override preservation | Before and after debug deployment, hash an existing installed `treasure_overrides.txt`; hashes and bytes must match exactly. | `PASS` (`CD52EE5C...BA43` before and after) |

Until those rows are exercised, 2.1.1 has automated source, build, installer,
package, and local deployment acceptance only. In particular, real-controller
compatibility, Area Quest visual correctness, gameplay, and frame-time impact
remain explicit `NOT_VALIDATED` results rather than assumed passes.
