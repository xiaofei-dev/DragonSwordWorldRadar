# Runtime Feedback Audit for 2.2.1

## Scope

Version 2.2.1 is a fixes-only world-map stability candidate. It does not add or
change marker catalogs, marker capacity, marker coordinates, F6, localization,
compact height guidance, save reconciliation, or installer behavior. The
current candidate changes expanded-map ownership and reduces the two atlas textures
from 3072 to 2048.

The repeated world-map drift, flashing, native-icon displacement, and click-
target displacement reports are treated as one ownership feedback defect, not
as a resolution preset problem.

## Current full-stretch-host architecture boundary

The current `DLayerMap.FogAbovePanel` is the sole expanded-map parent. It is
resolved directly from the current layer rather than inferred from whichever
native icon happens to be first in `ArrayIconInfo`. The array has one bounded,
creation-only role: when either Mod host is missing, it may supply one currently
instantiable native icon class for cloning. Retained-host validation and refresh
never scan it and never use native icon order to decide ownership.

The Mod adds only two hit-test-invisible outer hosts to `FogAbovePanel`. Each
outer `UCanvasPanelSlot` fills the native parent rather than contributing the
atlas bounds to the parent layout:

- anchors are full stretch, `(0,0)` to `(1,1)`;
- offsets are zero;
- `AutoSize=false`, alignment is `(0,0)`, and Canvas Z is maximum;
- the host receives no render transform and no forced layout prepass is used.

After every fresh attachment, including a scheduler-accepted bounded rebuild,
the cloned host's inner `Panel_Point` Canvas slot is forced to full stretch,
zero offsets, `AutoSize=false`, and alignment `(0,0)`. It requires no Z override
because maximum Z belongs to the outer host slot.

Each atlas Image owns the atlas rectangle inside that Mod-owned inner Canvas:
its Canvas slot is `{atlas_left,atlas_top,atlas_width,atlas_height}` and its
render translation is `(0,0)`. The resulting hierarchy is `FogAbovePanel ->
full-stretch outer host -> full-stretch Panel_Point -> atlas-rectangle Image`.
The native parent therefore sees only a zero-offset full-stretch child, while
the Image still uses the native parent's local origin and zoom pivot. The hosts
inherit continuous `FogAbovePanel` pan, zoom, clipping, visibility, and
RetainerBox composition without replaying delayed viewport transforms.

## Topology diagnosis

The temporary zoom-topology diagnostics captured the rejected heuristic's
failure directly. The game rebuilt native icon widgets as the zoom category
changed. The first valid `ArrayIconInfo` entry then moved between
`FogAbovePanel` and `FogUnderPanel`; the Mod treated that transient icon parent
as its own parent and reattached both hosts four times in one zoom sequence.
Those reattachments crossed the fog composition boundary, explaining the
observed partial disappearance, hitching, and flashing. This evidence supports
the ownership correction. It does not prove corrected gameplay.

The later direct-`FogAbovePanel` full-stretch-outer/Image-translation candidate
kept one owner but still used a different local origin/zoom pivot from the
atlas. Quantitative 2026-09-05 screenshots reject it:

- screenshot 6 to 7 (zoom-in): base-map scale about 1.214 with translation
  `(-138,-18)`, Radar scale about 1.218 with translation `(-25,-208)`, and a
  32-point mean fitted residual of 0.28 px; relative drift is about
  `(+113,-190)` px;
- screenshot 8 to 9 (reverse zoom): base-map scale about 0.760 with translation
  `(+64,+64)`, Radar scale about 0.758 with translation `(+131,+264)`, and a
  35-point mean fitted residual of 0.82 px; relative drift is about
  `(+67,+200)` px.

The scale ratios agree while the vertical offset reverses with zoom direction;
the implied vertical pivot difference is roughly 823-872 px. This rules out a
scale-formula mismatch and cumulative frame drift, but it does not validate a
replacement candidate.

The subsequent outer-atlas-rectangle DLL
`CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2`,
from compiled source
`433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`,
is also runtime rejected. It successfully attached 1,632 markers, but its
negative atlas-left outer-slot offset expanded the same native Canvas from
`3000x3000` to `3191.521x3000`. That self-authored extent change repeatedly
requested rebuilds and produced six attaches and five detaches, restoring the
flashing/blank-map failure. Diagnostics recorded no marker-data, texture, or ABI
fault. Its rollback backup remains
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`.

The next inner-atlas DLL
`5C632820CC44065AB9FEDDA260A392B5C72655D3815F445C73CF4A50A72DD606`,
from compiled source
`798297BBF7F9791A9E2FFBFF4EC5894A62CC4B16B825CB6BAEBC19A398CB2585`,
is independently runtime rejected. It fixed the outer-slot extent feedback and
held one `FogAbovePanel` identity at `3000x3000`, with one attach, zero detaches,
and zero rebuilds. However, six same-parent refreshes treated changing
`PlayerIconWidget` anchors as coordinate-space migration and rewrote the inner
Image slot. Atlas origin moved from `(-320.726335,118.720902)` to
`(-297.909929,-107.178756)`. At the logged map scale 2.7 and DPI 1.5, the net
local delta corresponds to approximately `(+92,-915)` screen pixels and exactly
explains the reported zoom displacement. Its rollback backup remains
`dist/work/deployment/deploy-backups/20260905-191717-084-native-only-deploy`.
The complete immutable candidate matrix and prohibition list is maintained in
`docs/WORLD_MAP_ATTEMPT_LEDGER.md`.

## Feedback-loop prevention

Same-parent pan and zoom are inherited and require no Mod transform write,
rerasterization, marker-snapshot rebuild, or reprojection. Only a fresh
attachment reads `PlayerIconWidget` to author the atlas projection. Every pass
in the finite five-deadline retained-host tail directly revalidates
`FogAbovePanel` and reads only that exact owner's live local extent. While
parent identity and extent remain unchanged, the attach-time Image Canvas-slot
placement is immutable and refresh performs no layout, transform, widget-tree,
or RetainerBox write. A parent replacement reports `RebuildRequired`; refresh
never reparents or delta-rebases retained content. The main scheduler alone may
perform one fresh bounded attachment.

A same-parent extent change is not accepted from one observation. The existing
geometry-stability sampler must see two matching successful changed-extent
samples before refresh reports `RebuildRequired`; until then the last valid
payload remains retained. Reporting rebuild is observation-only: refresh does
not pre-collapse the payload or clear transform readiness, and the bounded main
scheduler owns any accepted detach/rebuild. Attach completion arms its event
tail from a fresh post-attach clock, so a slow attach cannot consume the settle
delay before attachment is live. Attach also issues no empty-state end-of-attach
`RequestRender`; the guarded visibility path owns the first repaint.

One open-map session may service `RebuildRequired` at most once while preserving
the marker snapshot. The rebuild receives its own hard-capped three-attempt
attach/geometry budget, so the complete open session is bounded to the initial
three attempts plus the rebuild's three attempts. A final-tail `RetryLater`
closes through the same bounded rebuild transaction.

Before valid ownership exists, missing or invalid geometry keeps the Mod-owned
hosts collapsed inside the existing bounded retry budget. Mod-owned payload,
ABI, and guarded runtime failures still fail closed. The settle schedule adds no
steady timer, poll, enumeration, SQL work, or per-frame projection route.
Persistent-cache file validation/write and texture import are bounded explicit
attachment work, not steady-state work.

## Preserved 2.2.0 contracts

- fixed 4,096-entry world-map snapshot capacity;
- two event-built 2048-by-2048 atlases, approximately 32 MiB decoded BGRA;
- persistent envelope `DSNWRA52`, with fingerprint quantized at 1/4096 UMG
  logical unit, exact dimensions/header/magic/fingerprint/visible-count checks,
  complete expected-pixel RLE decode, encoded-payload checksum, and exact EOF;
- automatic cache miss for revision-51, corrupt, truncated, or trailing-byte
  files, and same-directory temporary writes atomically published by
  `MoveFileExW` with replace-existing and write-through flags;
- all marker catalogs, sizes, centers, ordering, filtering, and visibility;
- F6 layout, localization, font routing, status and report actions;
- Treasure, Area Quest, and Fly/Mole/Wave compact height guidance;
- game-update save-key fallback and all save/encounter behavior;
- installer ownership, update, repair, uninstall, and package layouts.

## Evidence boundary

The exact 2.2.0 hashes, static gates, build, package matrices, archive checks,
and developer-deployment records remain historical 2.2.0 evidence. They are
not 2.2.1 evidence. Earlier 2.2.1 technical candidates also retain evidence only
for their exact bytes. The independent-viewport/extreme-Z deployed DLL
`2ABBE35DAC7D3AD588D35461F0CCDC29C95E60997BAF834D8F83434111E34F88`
is runtime rejected: markers displayed, but the map remained severely laggy,
wrongly positioned, and delayed. The full-stretch-outer/Image-translation DLL
`CCC6B1170BAD1BF94AE5149DE52B48E2E11553747016E10299423BFD0C06AE00`
from compiled source
`B650B5FBD731EC0BC24D43F2256A1826E403D164353DFC5B684AC943FAD174EA`
is separately runtime rejected by the screenshot evidence above. The later
outer-atlas-rectangle DLL
`CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2`
from compiled source
`433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`
is runtime rejected by the 1,632-marker attach and `3000x3000` to
`3191.521x3000` self-oscillation evidence above. The later inner-atlas DLL
`5C632820CC44065AB9FEDDA260A392B5C72655D3815F445C73CF4A50A72DD606`
is runtime rejected by the six same-parent Image-slot writes and quantified
`(+92,-915)` maximum screen displacement above. None validates the current
immutable-placement candidate.

Current 2.2.1 state:

| Phase | State |
| --- | --- |
| Source review | Passed for current immutable-placement source; release hygiene passed |
| Static gates | Passed for current source |
| Core | Passed `2/2` |
| Native build | Passed: DLL `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`, compiled source `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`, size 1,107,968 bytes |
| Installer and manual matrices | Passed for current bytes: Setup `20/20`, Manual `2/2`, payload equivalence, layout, clean-target, source immutability, and fixture cleanup |
| Final package | Passed for current bytes: three ZIPs re-extracted byte-identically; hashes recorded in `dist/final-2.2.1/release-manifest.json` and `SHA256SUMS.txt` |
| Developer deployment | Passed: installed DLL hash and size match; one enabled `mods.txt` entry; diagnostics enabled; rollback backup `20260905-202742-614-native-only-deploy` |
| Exact-artifact gameplay | `NOT_VALIDATED` |
| Visual and resolution acceptance | `NOT_VALIDATED` |
| Performance acceptance | `NOT_VALIDATED` |
| Publication | Blocked pending acceptance and provenance review |

Developer deployment passed for current DLL
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`:
the installed file is 1,107,968 bytes with the same hash, `mods.txt` contains
exactly one enabled product entry, diagnostics are enabled for acceptance, and
rollback backup `dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`
exists.
The intermediate A5CEBAEC/72BD98D3 build remains build evidence only and was
superseded before deployment when successful retained refresh was made fully
observation-only.
The preceding `20260905-191717-084-native-only-deploy` backup belongs to rejected
DLL `5C632820...D606` and remains rollback evidence only.

## Exact-artifact gameplay matrix

The exact final 2.2.1 DLL must still prove:

- direct `DLayerMap.FogAbovePanel` ownership remains stable while native zoom
  rebuilds icon widgets and changes `ArrayIconInfo` order/content;
- retained-host validation and refresh perform no `ArrayIconInfo` scan, and
  missing-host creation uses it only to obtain an instantiable icon class;
- outer slots remain full stretch with zero offsets, `AutoSize=false`, alignment
  `(0,0)`, and maximum Canvas Z without changing native parent extent, native
  icon position, scale, or click targets;
- after every fresh attachment, including a bounded rebuild, inner
  `Panel_Point` slots remain full stretch with zero offsets and each local 2048
  Image remains at
  `{atlas_left,atlas_top,atlas_width,atlas_height}` with zero render translation;
- native icon click targets remain aligned before and after Mod activation;
- Mod markers inherit repeated pan, zoom, clipping, visibility, and RetainerBox
  composition without delayed same-parent transform synchronization;
- every retained event-tail pass reads only the exact `FogAbovePanel` local
  extent; same-parent/same-extent passes, including the final pass, keep the
  attach-time Image slot immutable and perform no layout/transform write,
  restack, Remove/Add, or `RequestRender`;
- native zoom-tier `PlayerIconWidget` rebuilds cannot defer or mutate retained
  rendering; real `FogAbovePanel` replacement reports `RebuildRequired` for a
  fresh attachment rather than reparents or delta-rebases retained pixels;
- same-parent extent drift requires two matching successful changed-extent
  samples before `RebuildRequired`, and rebuild reporting retains the last valid
  payload until the scheduler accepts the rebuild;
- attach tails start from a fresh post-attach clock and attach completion issues
  no empty-state `RequestRender`;
- parent-extent, retained-RetainerBox, and owned-payload drift enter at most one
  snapshot-preserving rebuild per open session, with an independent three-
  attempt budget and a total ceiling of initial 3 plus rebuild 3;
- 3840x2160 16:9, representative 21:9 and 16:10, Windows DPI scaling,
  fullscreen, borderless, and windowed transitions;
- dense Treasure display does not trigger repeated reconstruction or layout
  oscillation;
- controller-opened pause/map menus suppress compact markers correctly;
- travel, teleport, save/world reload, map close/reopen, F8/F7, and title return;
- normal game exit and shutdown without an access violation;
- only a fully validated `DSNWRA52` cache hit across a process restart skips
  rerasterization; validate the 1/4096 fingerprint, exact metadata, full RLE
  pixel count, encoded checksum, and exact EOF, while cold/hit attach time and
  bounded file/import cost are measured separately;
- diagnostics-off frame time and approximately 32 MiB raw atlas memory remain
  within the accepted boundary.

Source, static, build, and deployment gates validate only their exact bytes and
never convert the temporary topology trace into corrected gameplay evidence.
Until the remaining exact-package and runtime checks are captured, the corrected
game behavior remains `NOT_VALIDATED` and no runtime issue is described as
proven fixed.
