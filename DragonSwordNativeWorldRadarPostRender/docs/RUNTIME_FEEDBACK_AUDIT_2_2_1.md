# Runtime Feedback Audit for 2.2.1

## Scope

Version 2.2.1 is a fixes-only world-map stability candidate. It does not add or
change marker catalogs, marker capacity, atlas resolution, marker styling, F6,
localization, compact height guidance, save reconciliation, or installer
behavior.

The repeated world-map drift, flashing, native-icon displacement, and click-
target displacement reports are treated as one ownership feedback defect, not
as a resolution preset problem.

## Confirmed architecture boundary

The game-native map-icon Canvas is read only. It remains the authoritative
geometry and player-icon witness, but the Mod must not:

- add or remove a child from that Canvas;
- create or resize a slot in that Canvas;
- alter its Z order, desired size, prepass, layout, clipping, or hit testing;
- use Mod marker capacity or atlas dimensions as native Canvas geometry.

The two existing Mod atlas hosts are independent, hit-test-invisible viewport
widgets created through `AddToViewport`. Accepted native-Canvas geometry is
converted with `LocalToAbsolute`, then into viewport-local coordinates before
the hosts are transformed. The established cached-Slate, DPI, zoom,
independent-X/Y, player-anchor, aspect-ratio, marker-center, and atlas
projection chain remains authoritative.

## Feedback-loop prevention

A same-parent pan, zoom, DPI, viewport, or layout observation may update only
the two viewport-host transforms. It must not rerasterize an atlas, rebuild the
marker snapshot, remove/add a host, or reparent a host.

Each of the five finite settle deadlines takes one fresh read-only sample of
game-owned Canvas and viewport geometry. That pass may update only the
Mod-owned hosts' position, desired size, or visibility; there is no
final-pass rebuild, reparent, or native-tree mutation.

Before the first verified transform, or after the layer identity changes,
missing or invalid geometry keeps the Mod-owned hosts collapsed and consumes
only the existing bounded retry budget. Once the same layer has a valid
transform, a transient native-Canvas replacement or geometry-read gap retains
the last verified host position, size, and gated visibility instead of
faulting, detaching, or rebuilding. Mod-owned payload, ABI, and guarded runtime
failures still fail closed. The settle schedule remains bounded; 2.2.1 adds no
steady timer, poll, enumeration, filesystem work, SQL work, or per-frame
projection route.

## Preserved 2.2.0 contracts

- fixed 4,096-entry world-map snapshot capacity;
- two event-built 3072-by-3072 atlases and style revision 50;
- all marker catalogs, sizes, centers, ordering, filtering, and visibility;
- F6 layout, localization, font routing, status and report actions;
- Treasure, Area Quest, and Fly/Mole/Wave compact height guidance;
- game-update save-key fallback and all save/encounter behavior;
- installer ownership, update, repair, uninstall, and package layouts.

## Evidence boundary

The exact 2.2.0 hashes, static gates, build, package matrices, archive checks,
and developer-deployment records remain historical 2.2.0 evidence. They are
not 2.2.1 evidence. Exact-artifact 2.2.1 source validation, static/core gates,
native build, package validation, Setup `20/20`, Manual `2/2`, payload
equivalence, layout, clean-target, and three-archive byte-identical
re-extraction passed. The accepted native DLL SHA-256 is
`C21823088E38D2BD1635651981187AB4C01C2FFD0DCD4804CB9FFDB1899FABB9`;
the compiled-source SHA-256 is
`DE0100B2D4DE894FA94C6911AD328F7699C55D50EC21193C688B44F7F2588BA2`.

Current 2.2.1 state:

| Phase | State |
| --- | --- |
| Source review | Passed |
| Static/core gates | Passed |
| Native build | Passed |
| Installer and manual matrices | Passed (`20/20` and `2/2`) |
| Final package | Passed, including byte-identical re-extraction of all three ZIPs |
| Developer deployment | Passed with diagnostics enabled; rollback backup `dist/work/deployment/deploy-backups/20260903-194004-398-native-only-deploy` |
| Exact-artifact gameplay | Not validated |
| Publication | Blocked pending acceptance and provenance review |

## Exact-artifact gameplay matrix

The exact final 2.2.1 DLL must still prove:

- native map icons do not move, flash, disappear, or change scale when Mod map
  markers are enabled or disabled;
- native icon click targets remain aligned before and after Mod activation;
- Mod markers remain aligned through repeated pan and zoom changes;
- 3840x2160 16:9, representative 21:9 and 16:10, Windows DPI scaling,
  fullscreen, borderless, and windowed transitions;
- dense Treasure display does not trigger repeated reconstruction or layout
  oscillation;
- controller-opened pause/map menus suppress compact markers correctly;
- travel, teleport, save/world reload, map close/reopen, F8/F7, and title return;
- normal game exit and shutdown without an access violation;
- diagnostics-off frame time and memory remain within the accepted boundary.

Until these checks are captured against the exact packaged bytes, 2.2.1 is a
technically validated release candidate, but no runtime issue is described as
proven fixed.
