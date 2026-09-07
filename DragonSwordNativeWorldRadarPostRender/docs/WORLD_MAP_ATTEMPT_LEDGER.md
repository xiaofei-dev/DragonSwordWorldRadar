# World-Map Attempt Ledger

## Purpose and immutability policy

This file is the append-only decision record for expanded-world-map rendering
attempts. It exists to prevent a visually rejected architecture from being
reintroduced under a new candidate number.

- Existing candidate entries are immutable. Do not delete or rewrite them.
- A correction must be added as a new entry that explicitly supersedes the old
  statement.
- Every built candidate must record its exact DLL SHA-256 and compiled-source
  digest before deployment.
- `STATIC_ACCEPTED`, `BUILD_ACCEPTED`, `PACKAGE_ACCEPTED`, and installer success
  do not mean that gameplay is accepted.
- A candidate becomes `GAMEPLAY_ACCEPTED` only after runtime and visual
  acceptance covering map open/close, pan, continuous zoom, zoom-tier changes,
  marker toggles, dense marker populations, and representative resolutions.
- A user screenshot or runtime log that demonstrates incorrect behavior is
  sufficient to mark those exact bytes `RUNTIME_REJECTED`, even if all static,
  build, package, and installer checks passed.

Status vocabulary:

- `RUNTIME_REJECTED`: exact deployed bytes produced an unacceptable runtime
  result and must never be promoted or silently rebuilt under the same design.
- `NOT_GAMEPLAY_VALIDATED`: evidence is insufficient to claim runtime
  correctness.
- `PENDING`: design boundary only; no candidate bytes exist yet.

## Immutable candidate entries

### WM-00A - Historical full-parent outer host with inner atlas offset

- **Hash:** No independently preserved candidate hash is attributable to this
  historical sub-attempt. It predates the receipt discipline used below.
- **Architecture:** The outer host filled the native parent, while the atlas
  offset was moved into the inner Image.
- **Runtime evidence:** The 2.2.0 runtime audit records that this moved the
  complete marker layer and displaced it relative to the native map. The
  architecture was replaced by an outer-atlas-rectangle layout.
- **Decision:** `RUNTIME_REJECTED` as an architecture family.
- **Do not retry:** Do not claim that a full-parent/full-stretch outer host plus
  a dynamically positioned inner Image is new merely because ownership,
  texture size, or candidate numbering changed. Any future inner-atlas design
  must make the attach-time Image slot immutable while the same parent and
  extent remain valid.

### WM-00B - Historical 2.2.0 final first-valid-parent outer atlas rectangle

- **DLL SHA-256:**
  `6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`
- **Compiled-source digest:**
  `A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`
- **Deployment receipt:** `20260903-003509-033-native-only-deploy`
- **Architecture:** Select the parent of the first valid native icon; put the
  atlas rectangle on the outer Canvas slot; keep the inner Image at local zero;
  use layout prepass and dynamic parent re-witness/rebase behavior.
- **Runtime evidence:** Dense uncollected Treasure reports associated this
  family with world-map flash/shift. A different prior runtime log showed one
  attach without repeated detach, but the exact bytes above did not receive a
  complete visual/gameplay acceptance run.
- **Decision:** `NOT_GAMEPLAY_VALIDATED`. It is not an accepted fallback.
- **Do not retry:** Do not infer Mod ownership from the first valid
  `ArrayIconInfo` entry, and do not use a successful attach or installer result
  as proof of visual correctness.

### WM-01 - Independent viewport transform replay

- **DLL SHA-256:**
  `2ABBE35DAC7D3AD588D35461F0CCDC29C95E60997BAF834D8F83434111E34F88`
- **Compiled-source digest:**
  `3898DC4D208A7F981112FEAB397DEA5FAA2525657F8F144F23530A870A2DD8CC`
- **Deployment receipt:** `20260904-181831-008-native-only-deploy`
- **Architecture:** Add cloned hosts independently to the viewport at maximum
  Z. Treat the native Canvas as a read-only witness, convert its cached geometry
  to viewport placement, and replay `SetDesiredSizeInViewport` and
  `SetPositionInViewport` from the event tail.
- **Runtime evidence:** Markers became visible, but updates were delayed,
  placement was incorrect, and map interaction was severely laggy. The copied
  viewport transform could not remain synchronized with the native map's live
  transform/composition pipeline.
- **Decision:** `RUNTIME_REJECTED`.
- **Do not retry:** No independent viewport, viewport-overlay, delayed geometry
  replay, or per-frame/event-tail imitation of the native pan/zoom transform.

### WM-02A - First-valid-parent full-stretch host with Image translation

- **DLL SHA-256:**
  `FE811E81B741E2F9FA849DB3FF208CDC567187BD3E77F5A6C440E80BB7B82A21`
- **Compiled-source digest:**
  `45EF8C60E46427DC66FC13C16FFCA1BBF11FC0A78E5EC68753ECDD46A6339CA3`
- **Deployment receipt:** `20260905-053728-726-native-only-deploy`
- **Architecture:** Select the parent of the first valid native icon; attach a
  layout-neutral full-stretch outer host; size the 2048 atlas Image locally and
  apply the atlas offset as Image render translation; refresh may reselect the
  parent.
- **Runtime evidence:** Visual testing showed fog-boundary occlusion, partial
  disappearance, hitching, and flashing as zoom changed.
- **Decision:** `RUNTIME_REJECTED`.
- **Do not retry:** Do not use native icon array order as an ownership signal,
  and do not apply atlas placement through Image render translation.

### WM-02B - Topology-instrumented first-valid-parent variant

- **DLL SHA-256:**
  `2D45DEFC948F69EC93BD6001BBF1C20476E8F96944B6B35275FEEB510E7E783B`
- **Compiled-source digest:**
  `18A2B8F333712C7B97C9A10BFDFE651D224F21034667F2BF79F9FC280F96C377`
- **Deployment receipt:** `20260905-081326-940-native-only-deploy`
- **Architecture:** Same ownership and full-stretch/Image-translation family as
  WM-02A, with topology diagnostics added.
- **Runtime evidence:** The first valid icon's parent alternated between
  `FogAbovePanel` and `FogUnderPanel` as icon population/zoom tier changed.
  Topology logs recorded four Mod host reparent operations during one zoom
  sequence.
- **Decision:** `RUNTIME_REJECTED`; it supplies the root evidence for WM-02A.
- **Do not retry:** `ArrayIconInfo` may be used only for bounded creation-class
  discovery when a Mod host is absent. It must not choose, validate, or refresh
  world-map ownership.

### WM-03 - Direct FogAbove full-stretch host with Image translation

- **DLL SHA-256:**
  `CCC6B1170BAD1BF94AE5149DE52B48E2E11553747016E10299423BFD0C06AE00`
- **Compiled-source digest:**
  `B650B5FBD731EC0BC24D43F2256A1826E403D164353DFC5B684AC943FAD174EA`
- **Deployment receipt:** `20260905-092836-495-native-only-deploy`
- **Architecture:** Resolve `DLayerMap.FogAbovePanel` directly; use a
  full-stretch outer host; apply atlas placement through Image render
  translation.
- **Runtime evidence:** Ownership remained stable, but paired screenshots
  measured approximately `(+113,-190)` pixels of relative drift during zoom-in
  and `(+67,+200)` during reverse zoom. Base-map and Radar scale ratios agreed,
  while the vertical translation changed sign. The implied vertical pivot
  mismatch was approximately 823-872 pixels.
- **Decision:** `RUNTIME_REJECTED`.
- **Do not retry:** Do not correct a direct-FogAbove child with render
  translation, PlayerIcon-anchor delta, inferred pivot compensation, or any
  other retained-atlas delta transform.

### WM-04 - Direct FogAbove with outer atlas rectangle

- **DLL SHA-256:**
  `CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2`
- **Compiled-source digest:**
  `433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`
- **Deployment receipt:** `20260905-182946-652-native-only-deploy`
- **Architecture:** Resolve `FogAbovePanel` directly; put the sparse atlas
  rectangle on the outer native-parent Canvas slot; keep the inner Image at
  local zero.
- **Runtime evidence:** All 1,632 markers attached, so marker data, texture
  upload, and the basic ABI path were operational. However, atlas left was
  `-320.726335`; the negative sparse bounds plus width expanded a nominal
  3000-wide parent contribution to 3191.521. Logs recorded six attaches and
  five detaches, accompanied by flash/blank behavior.
- **Decision:** `RUNTIME_REJECTED`.
- **Do not retry:** Never place negative sparse-atlas bounds on a child slot
  that participates in the native parent's desired layout. A Mod child of the
  native parent must remain layout-neutral and full stretch.

### WM-05 - Current direct FogAbove with dynamically rebased inner atlas slot

- **DLL SHA-256:**
  `5C632820CC44065AB9FEDDA260A392B5C72655D3815F445C73CF4A50A72DD606`
- **Compiled-source digest:**
  `798297BBF7F9791A9E2FFBFF4EC5894A62CC4B16B825CB6BAEBC19A398CB2585`
- **Deployment receipt:** `20260905-191717-084-native-only-deploy`
- **Architecture:** Resolve `FogAbovePanel` directly; keep the outer host and
  cloned `Panel_Point` full stretch with zero offsets; put the atlas rectangle
  on the inner Image Canvas slot; treat PlayerIcon anchor movement as a reason
  to delta-rebase that retained Image slot and request a Retainer render.
- **Runtime evidence:** The exact deployed log recorded one attach, zero
  detaches, zero rebuild-required events, and a stable parent identity
  `258722:60764` with stable extent `3000x3000`. Six layering updates were caused
  only by PlayerIcon anchor changes. They moved atlas origin from
  `(-320.726335,118.720902)` to `(-297.909929,-107.178756)`, a net local movement
  of approximately `(+22.816406,-225.899658)`. With map render scale 2.7 and
  viewport DPI 1.5, this is approximately `(+92,-915)` screen pixels and
  directly explains the user-observed zoom displacement.
- **Decision:** `RUNTIME_REJECTED`. Static/build/package success for these bytes
  is superseded by runtime evidence.
- **Do not retry:** For a retained direct-FogAbove host, never use PlayerIcon
  local anchor movement to rewrite atlas placement. Do not call
  `rebase_world_map_atlas_placement`, `SetPosition`, `RequestRender`, or perform
  Remove/Add solely because that anchor changed. This candidate also repeats
  the dynamically positioned inner-Image failure family recorded in WM-00A.

## Next candidate boundary

### WM-06 - Immutable attach-time inner Image slot

- **Definition-time DLL SHA-256:** `PENDING - NOT BUILT`
- **Definition-time compiled-source digest:** `PENDING - NOT BUILT`
- **Initial status:** `PENDING`. This entry defines the candidate boundary; it does not claim
  static, build, package, deployment, runtime, or gameplay acceptance.
- **Required architecture:**
  - resolve the current `DLayerMap.FogAbovePanel` directly;
  - keep the outer host and cloned root/`Panel_Point` full stretch with zero
    offsets and no render transform;
  - assign the inner Image Canvas slot its atlas rectangle once at attach time;
  - while parent weak identity and stable extent are unchanged, keep that Image
    slot immutable.
- **Same-parent/same-extent invariant:** Refresh must perform zero layout or
  composition writes. PlayerIcon anchor changes are diagnostic only. This path
  must not call `SetPosition`, `SetOffsets`, `SetAnchors`, render translation,
  forced layout prepass, `RemoveFromParent`/Add, or `RequestRender`.
- **True topology-change rule:** If the direct FogAbove weak identity changes,
  or a changed extent remains stable through the bounded stability gate,
  discard the retained placement and perform one fresh rebuild from a new
  stable geometry sample. Never delta-rebase the old atlas from PlayerIcon or
  old-parent coordinates.
- **Required regression fixture:** With parent `258722:60764`, extent
  `3000x3000`, and initial atlas rectangle
  `(-320.726335,118.720902,2870.794737,2341.547368)`, anchor deltas including
  `(-12.133,-63.531)` and `(+16.800,-166.337)` must leave all four atlas values
  unchanged and produce no layout/reparent/repaint operation.
- **Promotion boundary:** Passing native state tests, ABI checks, build,
  packaging, installer matrices, hash verification, or deployment receipts can
  promote this candidate only to the corresponding static/build/package state.
  Runtime logs must then prove stable ownership and zero same-parent writes, and
  user visual testing must separately accept pan, continuous zoom in both
  directions, zoom-tier transitions, dense markers, toggles, and supported
  resolutions before `GAMEPLAY_ACCEPTED` may be recorded.

## Append-only status events

### 2026-09-05 - WM-06 native build

- **State:** `BUILD_ACCEPTED_NOT_DEPLOYED`; gameplay remains
  `NOT_GAMEPLAY_VALIDATED`.
- **DLL SHA-256:**
  `A5CEBAECAD75E8AE5C8BC2435625CD52BEE878AA656C1D8EA1C58C5C24542EDB`
- **Compiled-source digest:**
  `72BD98D3AA612D4F902D901B4174C4340BE325A54A2015E8B88DEACE738862A1`
- **Size:** 1,107,968 bytes.
- **Evidence:** Core tests passed `2/2`; F6 overlay, compact renderer, native
  world-map, PostRender/runtime safety, and release-hygiene gates passed; the
  native `/W4 /WX` build and source-bound receipt passed.
- **Unverified:** Developer deployment, live map visibility, pan/zoom/tier
  alignment, dense markers, supported resolutions, interaction, exit, and
  performance remain pending for these exact bytes.

### 2026-09-05 - WM-06 post-A5CE native build

- **Supersedes before deployment:** The preceding
  `A5CEBAECAD75E8AE5C8BC2435625CD52BEE878AA656C1D8EA1C58C5C24542EDB`
  build was never deployed. A follow-up source change removed the remaining
  success-path visibility reconciliation so same-parent/same-extent refresh is
  strictly observation-only.
- **State:** `BUILD_ACCEPTED_NOT_DEPLOYED`; gameplay remains
  `NOT_GAMEPLAY_VALIDATED`.
- **DLL SHA-256:**
  `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`
- **Compiled-source digest:**
  `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`
- **Release-tools digest:**
  `AB85C2002BCD685BD0CCEB4EC3C70254D11A8AE3844E7373AFAD144BC3C01BA8`
- **Size:** 1,107,968 bytes.
- **Evidence:** Native `/W4 /WX` build and source-bound receipt passed. The
  world-map canary now proves that the refresh function's sole visibility
  reconcile belongs to its transient fail-closed handler; the successful
  same-parent path performs none. Core tests passed `2/2`; F6 overlay, compact,
  world-map, and PostRender/runtime-safety gates passed.
- **Unverified:** Developer deployment, live map visibility, pan/zoom/tier
  alignment, dense markers, supported resolutions, interaction, exit, and
  performance remain pending for these exact bytes.

### 2026-09-05 - WM-06 developer deployment

- **State:** `DEPLOYMENT_ACCEPTED_GAMEPLAY_PENDING`.
- **DLL SHA-256:**
  `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`
- **Compiled-source digest:**
  `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`
- **Installed identity:** 1,107,968 bytes; installed and build SHA-256 match
  exactly; one enabled `mods.txt` entry; debug logging enabled.
- **Rollback backup:**
  `dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
- **Deployment gates:** Core `2/2`, F6 overlay, compact renderer, world-map
  canary, PostRender/runtime safety, release hygiene, source-bound receipt, and
  installed-payload verification passed during deployment.
- **Unverified:** No live session has run these bytes yet. Map visibility,
  continuous zoom and reverse zoom, zoom-tier transitions, pan, dense markers,
  supported resolutions, click targets, exit, and performance remain
  `NOT_GAMEPLAY_VALIDATED`.

### 2026-09-06 - WM-06 final technical package set

- **State:** `PACKAGE_AND_INSTALLER_ACCEPTED_GAMEPLAY_PENDING_PUBLICATION_BLOCKED`.
- **Native identity:** DLL
  `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`,
  compiled source
  `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`,
  size 1,107,968 bytes.
- **Setup identity:** unsigned Setup SHA-256
  `39B10BD46A5E7679D4686AC581310195471CB2E65468D11C9B510E7E3BF66E2B`.
- **Final ZIP identities:** Installer
  `A7F4065F52C4032A26B0D93FA7074FA478C1F985B98315C498DFEA23F44FC025`;
  Manual No-UE4SS
  `5ED7B0736C27B521CD11381EB912AE32068FD360B043DB9D35FBCA3CC97FEF14`;
  Manual With-UE4SS
  `D54AF55078C972DE4044403BBA29E2785821C73B8747C9CF306587A9B21595F2`.
- **Release gates:** Core `2/2`, compact/world-map/PostRender/static and release
  hygiene gates, source-bound receipt, Setup `20/20`, Manual `2/2`,
  Setup/manual payload equivalence, manual layout, clean-target policy,
  source immutability, fixture cleanup, and fresh byte-identical re-extraction
  of all three public ZIPs passed.
- **Content correction:** Before the final rebuild, package metadata left from a
  superseded viewport-owned candidate was detected and rejected. The final ZIPs
  contain current WM-06 native-Canvas ownership text and current DLL/source
  hashes; no `C2182308`, `DE0100B2`, old viewport-owner claim, or current-package
  pending marker remains.
- **Boundary:** Gameplay, visual, click-target, resolution, controller, exit,
  memory, attach-time, and performance acceptance remain `NOT_VALIDATED`.
  Binary and derived-data publication remains `BLOCKED` pending the recorded
  rights and provenance reviews.
