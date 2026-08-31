# MnMRadar Binary and Data Reverse-Engineering Guide

| Field | Value |
|---|---|
| Status | Static research baseline for DragonSwordNativeWorldRadar |
| Audit date | 2026-08-11 |
| Runtime acceptance | Not performed |
| Deployment performed by this audit | None |

## 1. Purpose and scope

This document records the useful implementation evidence recovered from the
closed MnMRadar package supplied for compatibility research. Its purpose is to
reduce uncertainty while replacing the external transparent-window Radar with
an in-process UE4SS C++ renderer.

This is not a source reconstruction and it is not evidence that the sample is
safe to copy. The target implementation must remain independently written,
must use the current game's generated data, and must retain the lifecycle and
fail-closed behavior already proven by DragonSwordWorldRadar.

The audit covered:

- Package structure and all supplied text and CSV data.
- PE identity, exports, imports, strings, vtables, and relevant x64 code paths.
- PostRender hook installation, dispatch, exception handling, and unload
  behavior.
- UCanvas drawing calls and their reflected parameters.
- Player/camera sampling and world-to-screen math.
- Static marker selection, dynamic animal scanning, coordinate units, IDs,
  duplicates, and data-update behavior.
- The gap between the sample, the current DragonSwordWorldRadar, and the
  intended native minimap Radar.

No game process was launched, injected, or modified. Static analysis cannot
prove runtime ABI compatibility, travel safety, visual correctness, or a
performance gain.

## 2. Evidence labels

The following labels are used throughout this document:

| Label | Meaning |
|---|---|
| Confirmed | Directly supported by the exact binary, data, or current repository source. |
| Strong inference | The implementation is strongly implied by control flow or data, but symbols are unavailable. |
| Unknown | Requires an exact-build runtime canary or additional dynamic evidence. |

Binary RVAs in this document apply only to the exact audited main.dll. Its
preferred image base is 0x180000000. They are research anchors, not portable
addresses and not a supported hooking contract.

## 3. Audited artifact identity

### 3.1 Package

| Property | Value |
|---|---|
| Archive | MnMRadar 1.4 EXPERIMENTAL Ue4ss Layout 128 1.4 2026-08-11T17-21Z LV92O2wN5.zip |
| Archive size | 424,260 bytes |
| Archive SHA-256 | 27544BF446646A83174B457BB7758E2887B8FF827D355A863CE8708DC41F41D3 |
| Extracted audit root | DragonSwordWorldRadar/dist/_mnmradar_reverse_audit |
| Mod path in archive | ue4ss/Mods/MnMRadar |

The package naming, README, and CHANGELOG disagree:

- The archive name says version 1.4 and Layout 128.
- README.txt says version 1.1 and requests experimental UE4SS
  v3.0.1-1018-g662df915.
- CHANGELOG.txt ends at 1.1 and also mentions stable UE4SS 3.0.1.

Binary evidence must therefore take precedence over the package title and prose
when determining ABI details.

### 3.2 Native DLL

| Property | Value |
|---|---|
| Path | ue4ss/Mods/MnMRadar/dlls/main.dll |
| Size | 660,480 bytes |
| SHA-256 | 46E820A835CEB9726C2CAA395D6957B27AD23871E917AF5322CDA68B8046260F |
| Architecture | PE32+ x64 |
| Linker | Microsoft linker 14.51 |
| PE timestamp | 2026-08-05 |
| Export start_mod | RVA 0x68BF0 |
| Export uninstall_mod | RVA 0x68D50 |
| Leaked PDB path | C:\MyModsExp\build\DSMarmotRadarCpp\DSMarmotRadarCpp.pdb |

The DLL imports UE4SS.dll, KERNEL32.dll, USER32.dll, and the Visual C++ runtime.
It has no DXGI, Direct3D, OpenGL, or Vulkan imports. Rendering is performed
through Unreal UCanvas rather than a graphics-API Present hook.

### 3.3 Data files

| File | Size | SHA-256 |
|---|---:|---|
| marmot_locations.csv | 2,559 | 77C86C6359C5CC63F592BEE0943D7AC21482A9548BC18B7D3D3C6CCFE47135DF |
| mnm_ids.csv | 10,871 | 66AC85FF1F9A6F5649FBFA92FEE94ECE81D36D25A17EC3EC897808BF9B65724D |
| mnm_nodes.csv | 483,182 | 20808216CAC06DF7073C9C18E75EF1DE837CAAF39B8AC1E5290B463E4237C419 |

## 4. Executive findings

| Finding | Evidence | Consequence for DragonSwordNativeWorldRadar |
|---|---|---|
| The sample renders inside Unreal through a GameViewportClient PostRender vtable detour and UCanvas. | Confirmed | This validates the general in-process rendering route. |
| The exact DLL patches vtable slot 112, not 128. | Confirmed | Never use the archive's Layout 128 label as a slot number. Revalidate the exact target build. |
| The sample is a full-screen camera-space 3D marker overlay, not a minimap renderer. | Confirmed | It does not provide minimap UI geometry, range, scale, rotation, layer, or big-map projection. |
| The supplied coordinates are static packaged CSV data in Unreal centimeters. | Confirmed | Keep the current install-time PAK generator as the authoritative update route. |
| Each drawing primitive resolves its UFunction and reflected parameter fields again. | Confirmed | Do not copy this hot-path overhead; resolve validated static metadata outside drawing. |
| Dynamic animal support enumerates 4,000 UObject slots on every PostRender call and retains Actor pointers. | Confirmed | Do not port animal scanning. It violates the target performance and lifetime rules. |
| Manual/automatic completion changes and panel settings can synchronously rewrite progress.txt or config.txt from PostRender. | Confirmed | Persistence must be coalesced and moved completely outside the render callback. |
| Chest completion is inferred from repeated absence in live UObject-scan results, not read from an authoritative chest save API. | Confirmed | Retain the current Radar's consistent save-snapshot filtering; do not port the absence heuristic. |
| The hook has no vtable restoration path during uninstall. | Confirmed | The sample does not demonstrate safe hot unload. A separate hook lifecycle must be designed and tested. |
| Multiple patched vtables share one stored original function. | Confirmed | Track an original per patched vtable and validate hook chaining. |
| General draw faults are caught, but drawing is retried every frame indefinitely. | Confirmed | Use a bounded fault budget and fail closed instead. |
| The current production mini-game catalog has 83 valid entries; the sample has 84 because it includes Fly 11024. | Confirmed | Use the current generated 33 Fly, 40 Mole, and 10 Wave catalog. |

## 5. Native module lifecycle and vtable layout

### 5.1 UE4SS object

start_mod allocates a 0xC0-byte derived CppUserModBase object, calls the base
constructor, installs the derived vtable at RVA 0x862F0, and fills mod metadata.

Relevant derived virtual entries:

| Slot | RVA | Meaning |
|---:|---:|---|
| 0 | 0x3A3A0 | Deleting destructor |
| 1 | 0x66120 | on_update |
| 2 | 0x65EB0 | on_unreal_init |

The remaining visible entries are base thunks. There is no derived Lua lifecycle
override and no Lua-to-native snapshot bridge in this DLL.

uninstall_mod null-checks the mod pointer passed in RCX and calls that object's
virtual slot zero with the delete flag. The derived destructor only invokes the
base destructor and frees the 0xC0-byte object. It does not restore any patched
viewport vtable.

### 5.2 Hook installation

The installer begins at RVA 0x48FA0. on_update checks a global installed flag at
RVA 0x9E77D. Until at least one patch succeeds, on_update invokes the installer
again every update with no retry delay or failure budget.

The installer:

1. Calls FindAllOf with GameViewportClient.
2. Iterates the returned UObject pointers.
3. Checks only for a non-null object and vtable.
4. Reads the function pointer at vtable byte offset 0x380.
5. Skips a slot already equal to the detour.
6. Uses VirtualProtect on the eight-byte slot with PAGE_READWRITE.
7. Saves the first encountered original in one global at RVA 0x9E868.
8. Writes the detour at RVA 0x488F0.
9. Calls VirtualProtect again to restore the old page protection.
10. Sets the installed flag if at least one slot was patched.

The exact slot is:

~~~text
0x380 / sizeof(void*) = 0x380 / 8 = 112
~~~

The success logging path also passes the immediate value 0x70, which is decimal
112. This independently confirms the slot. The archive text Layout 128 is not
the PostRender slot used by this DLL.

Approximate recovered behavior:

~~~cpp
if (installed) {
    return;
}

auto viewports = FindAllOf(L"GameViewportClient");
for (auto* viewport : viewports) {
    if (viewport == nullptr || viewport->vtable == nullptr) {
        continue;
    }

    void** slot = &viewport->vtable[112];
    if (*slot == &post_render_detour) {
        continue;
    }

    if (make_writable(slot)) {
        if (global_original == nullptr) {
            global_original = *slot;
        }
        *slot = &post_render_detour;
        restore_page_protection(slot);
        ++patched_count;
    }
}

installed = patched_count > 0;
~~~

Important weaknesses:

- FindAllOf is a global object enumeration during installation and every failed
  retry.
- It does not use IsValid, object flags, or an exact class check.
- It may patch multiple distinct vtables but stores only one original.
- It ignores failure of the second VirtualProtect call.
- It records no per-vtable restoration information.
- It performs no further search after the first success, so a later viewport
  subclass or vtable cannot be added.
- The installed flag and original pointer are plain globals with no visible
  synchronization.

### 5.3 Detour behavior

The detour begins at RVA 0x488F0 and is consistent with:

~~~cpp
void PostRenderDetour(UGameViewportClient* viewport, UCanvas* canvas);
~~~

The function:

1. Preserves the Canvas argument from RDX.
2. Calls the stored original PostRender first with RCX and RDX unchanged.
3. Calls the custom drawing routine at RVA 0x58E40.
4. Returns without retaining the viewport or Canvas pointer.

The original call occurs at RVA 0x48909. The custom draw call occurs at
RVA 0x489CD. This ordering preserves the game's own PostRender work, but also
means the Radar is drawn after the original callback.

No worker-thread creation path was identified in the analyzed hook/render
control flow, and no CreateThread import or equivalent path was found. The
detour itself is synchronous and implicitly assumes PostRender is called on the
appropriate game/render thread. The actual thread affinity remains a runtime
property to verify.

### 5.4 Exception handling

A structured-exception scope surrounds only the custom draw call, approximately
RVA 0x489CA through 0x489D4. The original PostRender call is outside that scope.
The filter at RVA 0x77860 records the exception code and a draw-stage value and
returns EXCEPTION_EXECUTE_HANDLER.

The handler increments a fault counter and rate-limits messages to the first
three faults, counts 100 and 1,000, and multiples of 10,000. A branch exists to
disable animal scanning after at least five faults when the last stage is 40.
However, exhaustive references to the stage variable found assignments only
for stages 10, 20, 30, 50, 60, 65, and 70. No stage-40 producer was found, so
that recovery branch appears unreachable in this exact DLL. Other custom-render
faults do not trip a general circuit breaker; the same path is attempted again
on the next frame.

This protects the host from some access violations, but it is not a complete
fail-closed renderer lifecycle.

### 5.5 Development-only alternate callback

RegisterProcessEventPreCallback remains imported and is reachable from an
explicit developer-key path. Additional object enumeration and diagnostic code
also remains in the binary. The ordinary release rendering path uses the
vtable detour, but the CHANGELOG statement about a single hook should not be
interpreted as proof that no alternate development path exists.

## 6. UCanvas rendering path

The main draw routine begins at RVA 0x58E40. It reads Canvas SizeX and SizeY by
property-name lookup and invokes reflected UCanvas K2 drawing functions through
ProcessEvent.

Confirmed wrappers:

| Wrapper | RVA | Resolved function and named parameters |
|---|---:|---|
| Line | 0x45930 | K2_DrawLine: ScreenPositionA, ScreenPositionB, Thickness, RenderColor |
| Polygon | 0x45AF0 | K2_DrawPolygon: RenderTexture, ScreenPosition, Radius, NumberOfSides, RenderColor |
| Texture | 0x45DF0 | K2_DrawTexture: RenderTexture, ScreenPosition, ScreenSize, CoordinatePosition, CoordinateSize, RenderColor, BlendMode, Rotation, PivotPoint |
| Text | 0x462A0 | K2_DrawText: RenderFont, RenderText, ScreenPosition, Scale, RenderColor, Kerning, ShadowColor, ShadowOffset, bCentreX, bCentreY, bOutlined, OutlineColor |

The corresponding ProcessEvent sites include RVA 0x45A6E, 0x45D6C, 0x4621A,
and 0x467BF.

Strings for K2_DrawBox, K2_DrawTriangle, K2_DrawMaterial, and K2_DrawBorder are
also present, but the four wrappers above are the primary confirmed marker
path.

Every wrapper repeats all of the following:

- Resolve the UFunction by name.
- Allocate a reflected parameter buffer.
- Resolve individual parameters by property name.
- Populate parameters.
- Call ProcessEvent.
- Free the parameter buffer.

This maximizes loose-version compatibility at a measurable per-primitive cost.
The native target should instead resolve and validate static drawing metadata
outside PostRender, use fixed parameter structures on the callback stack or a
preallocated per-thread/per-in-flight slot with explicit ownership, and fail
closed if a signature does not match. Do not share one mutable global parameter
buffer: multiple viewports, nesting, or callback re-entry could overwrite it.
Any retained UFunction or property metadata must be limited to proven
class-lifetime objects and invalidated on an incompatible build or lifecycle
epoch; instance objects such as Pawn, Canvas, Viewport, or Widget must never be
retained.

## 7. Player and camera sampling

For each rendered frame, the sample:

- Obtains the current Pawn from its cached PlayerController.
- Calls AActor::K2_GetActorLocation on that current Pawn.
- Obtains the current PlayerCameraManager.
- Calls GetCameraLocation, GetCameraRotation, and GetFOVAngle through
  ProcessEvent.
- Uses Rotation.X as pitch and Rotation.Y as yaw.
- Ignores roll.
- Accepts FOV only when 1 < FOV < 175 and otherwise uses 90 degrees.

The player-location path is approximately RVA 0x5B6B0 through 0x5B7A1.

The sample does not put a Pawn pointer into a marker snapshot, but its cached
PlayerController is still an instance-lifetime dependency. The target design
must keep the stronger existing rule: obtain a fresh current Controller and
Pawn for each scalar sample and retain neither across samples or world epochs.

## 8. Recovered world-to-screen mathematics

The projection routine is at RVA 0x52660 through 0x5291A.

Given a target world position and camera state:

~~~text
d = targetWorld - cameraWorld
pitch = cameraPitch * pi / 180
yaw   = cameraYaw   * pi / 180
~~~

Camera-space axes:

~~~text
right =
    -sin(yaw) * d.x
    +cos(yaw) * d.y

up =
    -sin(pitch) * cos(yaw) * d.x
    -sin(pitch) * sin(yaw) * d.y
    +cos(pitch) * d.z

forward =
    cos(pitch) * cos(yaw) * d.x
    +cos(pitch) * sin(yaw) * d.y
    +sin(pitch) * d.z
~~~

Logical Canvas dimensions and focal length:

~~~text
logicalWidth  = Canvas.SizeX / screenScale
logicalHeight = Canvas.SizeY / screenScale
focal = logicalWidth * 0.5 / tan(FOV * pi / 360)
~~~

Projection:

~~~text
screenX = logicalWidth  * 0.5 + right / forward * focal
screenY = logicalHeight * 0.5 - up    / forward * focal
~~~

A target is accepted for direct projection only when:

~~~text
forward > 1 Unreal unit
~~~

The direction used for an off-screen indicator is:

~~~text
direction = normalize(right, -up)
~~~

This is a hand-written perspective projection using horizontal FOV. It is not a
minimap transform and does not call ProjectWorldLocationToScreen.

### 8.1 DPI and logical scale

The screen-scale routine is at RVA 0x47210 through 0x47306:

~~~text
if echelle_ecran > 0.1:
    screenScale = echelle_ecran
else:
    screenScale = GetDpiForWindow(hwnd) / 96
~~~

Invalid DPI falls back to approximately 1.0. Canvas dimensions default to
1920 x 1080 until refreshed from the callback Canvas.

Marker dimensions scale relative to a logical height of 1080. Recovered
defaults include:

- Icon size: 6.
- Marker vertical screen offset: 26.
- Text scale: 0.78.

The configuration value hauteur_marqueur is a screen-pixel offset. It is not a
world-Z offset.

### 8.2 Safe rectangle and edge indicator

The sample's hard-coded safe rectangle is:

~~~text
left   = logicalWidth  * 0.11
right  = logicalWidth  * 0.78
top    = logicalHeight * 0.08
bottom = logicalHeight * 0.87
~~~

These values are unrelated to the configuration panel's zone_haut and
zone_bas settings.

For an off-screen marker:

~~~text
center = midpoint(safeRectangle)
half   = halfSize(safeRectangle)

t = min(
    half.x / abs(direction.x),
    half.y / abs(direction.y)
)

edgePoint = center + direction * t
angleDegrees = atan2(direction.y, direction.x) * 180 / pi
~~~

A near-zero direction falls back to (1, 0).

This math is useful for a future optional 3D compass layer, but it should not be
used for the first native minimap milestone.

## 9. Coordinate and catalog analysis

### 9.1 Row counts

The two general catalogs use the exact header id,kind,x,y,z. The mini-game
catalog uses mini_game_id,kind,x,y,z. Every audited row has a non-empty identity
and kind, and every X/Y/Z value parses as a signed integer without fallback.

| File | Rows | Distinct kinds |
|---|---:|---:|
| marmot_locations.csv | 84 | 3 |
| mnm_ids.csv | 273 | 5 |
| mnm_nodes.csv | 9,452 | 62 |
| Total | 9,809 | 70 |

The kind sets are disjoint between these three files.

### 9.2 Coordinate ranges

| File | X range | Y range | Z range |
|---|---|---|---|
| marmot_locations.csv | -7,606 to 326,075 | -87,446 to 235,959 | -627 to 40,143 |
| mnm_ids.csv | -188,900 to 337,683 | -173,940 to 252,079 | -42,056 to 40,718 |
| mnm_nodes.csv | -204,385 to 345,652 | -175,249 to 269,394 | -47,362 to 58,465 |

Coordinates are stored unchanged in Unreal world units. The DLL divides
distance by 100 when displaying meters, confirming that the coordinates use
Unreal centimeters.

### 9.3 Mini-game records

| Kind | Count | ID range |
|---|---:|---|
| fly | 34 | 11001 through 11034 |
| mole | 40 | 12001 through 12040 |
| wave | 10 | 13001 through 13010 |

The sample includes:

~~~csv
11024,fly,128453,117518,10577
~~~

The current PAK-derived DragonSwordWorldRadar contract excludes 11024 and
contains exactly:

- 33 Fly records.
- 40 Mole records.
- 10 Wave records.
- 83 total records.

After removing 11024, the sample's remaining mini-game types and XYZ values
match the current generated 83-record catalog. The sample's complete
84-record file must not be copied as authoritative data.

At audit time, DragonSwordWorldRadar metadata still contains legacy prose/count
fields that mention 34 Fly records, while source and tests enforce 33 and
exclude 11024. The new package must derive its manifest counts from validated
generated artifacts rather than copying descriptive release fields.

### 9.4 ID-based records

| Kind | Count |
|---|---:|
| area_quests | 147 |
| poi_bounties | 40 |
| poi_fieldbosses | 9 |
| puzzle_platform | 69 |
| puzzle_slider | 8 |

The sample's 9 field-boss and 40 bounty coordinates have zero exact XYZ matches
with the current generated 9 Boss and 40 Assault records. There are systematic
XY offsets and substantial Z differences. These CSV records must not replace
the current PAK-generated Boss or Assault catalogs.

### 9.5 Node groups

| Group | Rows | Kinds |
|---|---:|---:|
| Animals | 1,385 | 5 |
| Chests | 1,589 | 8 |
| Ingredients | 4,460 | 40 |
| Materials | 1,724 | 8 |
| Lorebooks | 294 | 1 |

The sample contains 1,589 chest nodes, while the current game-derived treasure
catalog contains 1,693 records. This is another reason to preserve the current
PAK extraction and validation pipeline.

### 9.6 Generated node IDs

All 9,452 mnm_nodes IDs follow:

~~~text
baseId = floor(x / 100) + "_" + floor(y / 100)
~~~

Collisions append _2 and then _3:

- 9,421 base IDs.
- 30 IDs ending in _2.
- One ID ending in _3.
- Thirty one-meter XY cells collide: 29 pairs and one triple.

Negative coordinates use mathematical floor, not truncation toward zero.

These positional IDs are not durable game identities. They change if a point
moves into another one-meter cell or if collision ordering changes. The native
Radar should preserve authoritative game IDs whenever available and treat a
positional ID only as a deterministic catalog-local fallback.

### 9.7 Duplicate coordinates

- Every ID is unique within its source file.
- mnm_nodes.csv has no duplicate XYZ.
- mnm_ids.csv has one duplicate XYZ pair:
  - Area Quest 1110052 at (31963, 105378, 921).
  - Area Quest 1110061 at (31963, 105378, 921).
- One Bird Eggs pair has identical XY with Z values 5418 and 5423 at
  (147154, 80298).

Any deduplication must therefore be identity-aware. Deduplicating only by XY or
XYZ can incorrectly merge legitimate records.

## 10. Complete static kind inventory

### 10.1 Animals

| Kind | Count |
|---|---:|
| animal_boar | 147 |
| animal_fox | 130 |
| animal_goose | 194 |
| animal_rabbit | 587 |
| animal_squirrel | 327 |

### 10.2 Chests

| Kind | Count |
|---|---:|
| chests_grade1 | 339 |
| chests_grade2 | 669 |
| chests_grade3 | 367 |
| chests_grade4 | 68 |
| chests_grade5 | 5 |
| chests_key | 34 |
| chests_map | 29 |
| chests_puzzle | 78 |

### 10.3 Ingredients

| Kind | Count |
|---|---:|
| ingredients_aged_game_meat | 68 |
| ingredients_aqualily | 92 |
| ingredients_bird_eggs | 274 |
| ingredients_blue_lotus_mushroom | 110 |
| ingredients_cave_mushroom | 124 |
| ingredients_conch | 220 |
| ingredients_dawn_lanternleaf | 355 |
| ingredients_dried_maneflower | 98 |
| ingredients_drop_of_purity | 66 |
| ingredients_dwarf_potato | 121 |
| ingredients_fermented_gnoll_berry | 6 |
| ingredients_fragrant_herbs | 10 |
| ingredients_fruit_of_vitality | 66 |
| ingredients_goose_eggs | 45 |
| ingredients_grave_bellherb | 82 |
| ingredients_honey | 51 |
| ingredients_lantern_gourd | 67 |
| ingredients_leaf_of_vigor | 66 |
| ingredients_leaflet_tomato | 52 |
| ingredients_primordial_seed | 65 |
| ingredients_radish | 155 |
| ingredients_rice | 15 |
| ingredients_rock_mushroom | 356 |
| ingredients_round_eggplant | 82 |
| ingredients_salmon_fillet | 147 |
| ingredients_sand_crab | 75 |
| ingredients_shellfish | 105 |
| ingredients_shining_blue_lotus_mushroom | 10 |
| ingredients_shining_dawn_lanternleaf | 10 |
| ingredients_shrimp_flesh | 143 |
| ingredients_silverwing_fillet | 409 |
| ingredients_tree_sap_mushroom | 288 |
| ingredients_trout_fillet | 119 |
| ingredients_waveleaf_cabbage | 53 |
| ingredients_well_grown_mildwild_onion | 10 |
| ingredients_well_grown_waveleaf_cabbage | 10 |
| ingredients_wheat | 73 |
| ingredients_white_dewblossom | 55 |
| ingredients_whitebloom_carrot | 178 |
| ingredients_wild_onion | 129 |

### 10.4 Materials and lore

| Kind | Count |
|---|---:|
| materials_binding_crystal | 836 |
| materials_crystal_of_oblivion | 85 |
| materials_crystal_of_recollection | 85 |
| materials_crystal_of_remembrance | 90 |
| materials_crystal_of_reverie | 84 |
| materials_mana_shard | 6 |
| materials_piercing_crystal | 473 |
| materials_vein_crystal | 65 |
| poi_lorebooks | 294 |

## 11. Static marker selection

The selection routine at RVA 0x5DE50 through 0x5E4AA runs behind a 150 ms gate,
approximately 6.7 times per second.

For each refresh it:

1. Resets the selected point index for every configured category.
2. Scans all 9,809 static points.
3. Matches each point to a category by string.
4. Computes horizontal distance only.
5. Applies the configured maximum range.
6. Retains only the nearest point for each category.

Distance:

~~~text
horizontalDistanceMeters =
    sqrt((point.x - player.x)^2 + (point.y - player.y)^2) / 100
~~~

Z is ignored for nearest selection and displayed range. It is used only later
by the camera projection. At most one static marker per category is selected,
so the sample is intentionally not an all-nearby-point Radar.

Do not port this nested string lookup. A native catalog should compile each
record to a numeric type and a stable display layer during installation or
load. Spatial filtering should use a fixed-capacity result buffer or spatial
index, with no per-frame allocation.

## 12. Interior and map-state behavior

The only confirmed area filter is:

~~~text
playerInterior = player.x < -100000
pointInterior  = point.x  < -100000
show when playerInterior == pointInterior
~~~

Thirty-one supplied records satisfy x < -100000:

- 2 area quests.
- 13 chests.
- 16 lorebooks.

No references were found for:

- DLayerMiniMap.
- MiniMap or MapOverlay.
- RenderTransform or minimap scale.
- ProjectWorldLocationToScreen.
- World-map geometry.
- PersistentLevel or map name.
- Data-layer or floor identity.
- UWidget map state.

The sample therefore does not distinguish multiple interiors, floors,
underground layers, compact minimap state, or the expanded map. The x threshold
is a coarse binary partition, not a general map-area solution.

## 13. Dynamic animal path

The normal release rendering path calls the routine at RVA 0x3ACC0 through
0x3B2D1 after obtaining player position. It executes:

~~~text
ForEachUObjectInRange(currentIndex, currentIndex + 4000, callback)
~~~

This enumerates 4,000 UObject slots per PostRender frame. At the end of a full
sweep it resets the index and swaps the collected animal list. The resulting
list retains Actor pointers for later reads and drawing.

animals.txt contains nine enabled matching rules, one commented Beetle rule,
and a Salmon path explicitly described as inferred and unverified. Its comments
also indicate that streamed animals generally exist only within approximately
465 meters.

This mechanism must not be copied:

- It adds a large recurring UObject-array cost to the render path.
- It retains gameplay Actor pointers across frames.
- It couples object discovery, filtering, state mutation, and rendering.
- It relies on name/path rules with at least one acknowledged guess.

DragonSwordNativeWorldRadar retains the stronger invariant of no recurring
UObject enumeration and no retained gameplay instance.

## 14. User controls, panel, cadence, and persistence

### 14.1 on_update and hotkey requests

on_update at RVA 0x66120 polls GetAsyncKeyState for edges, publishes one-byte
requests, and attempts hook installation while it remains uninstalled. Most
user-facing work is deferred to PostRender rather than performed in on_update.

Confirmed ordinary controls:

| Key | Binary behavior |
|---|---|
| F9 | Toggles the global Radar-enabled byte at RVA 0x9B040 directly. |
| F1 | Publishes the panel-toggle request at RVA 0x9E28C. |
| Escape | Updates menu-related state at RVA 0x9E28D and time at RVA 0x9E828. |
| F10 | Publishes the cross-off request at RVA 0x9E28A. |
| F6 | Publishes the uncross request at RVA 0x9E28B. |

These are MnMRadar evidence only, not target key assignments. The native
DragonSword Radar keeps F7/F8 as its normal enable/disable controls. If F5/F6
A/B diagnostics are retained, they are registered only when
debug_logging=true. MnMRadar's F6/F10 behavior must not be imported.

PostRender consumes the F10, F6, and F1 request bytes around RVA 0x59601,
0x596B3, and 0x596CB. F9-disabled state returns early from PostRender, stopping
markers, panel work, later completion queries, and animal scanning together.

The developer-tools flag is at RVA 0x9E22C. When false, on_update skips the
diagnostic hotkeys. When true, the binary also polls F11, F12, F8, F7, F5,
F4 or Numpad 0, F3, F2, and Numpad 1 through 6 and publishes diagnostic
requests. Their exact business meanings were not all recovered. Package prose
claims eleven development shortcuts and warns that some perform full object
sweeps; the gate and key paths are confirmed, while that exact feature count
and the pause severity are package claims.

### 14.2 UCanvas settings panel

The panel toggle/controller begins at RVA 0x3B2E0 and uses the open-state byte
at RVA 0x9E778. The panel renderer at approximately RVA 0x40770 through
0x433C0 is drawn and interacted with through UCanvas; it is not ImGui.

Opening through F1 invokes reflected functions for SetShowMouseCursor,
SetIgnoreLookInput, and SetIgnoreMoveInput. Closing attempts to restore those
states. Opening through the bottom Radar bar is tagged as menu-originated and
avoids taking the same control ownership. Escape handling uses an 800 ms menu
heuristic.

This is useful evidence that an in-game configuration UI is possible, but it
adds lifecycle risk. A travel, exception, unload, or lost close event after
changing cursor/look/move state could leave controls altered. The first native
Radar should keep one file configuration and no interactive Canvas panel. If a
panel is added later, every control mutation requires a scoped token, captured
prior state, epoch invalidation, and guaranteed restore path.

Confirmed UI-backed settings:

| Setting | Binary storage |
|---|---:|
| icones | RVA 0x9B041 |
| contour | RVA 0x9E22D |
| taille_icone | RVA 0x9B0B8 |
| hauteur_marqueur | RVA 0x9B0BC |
| taille_texte | RVA 0x9B0E8 |
| portee_max | RVA 0x9E230 |
| Category display flag | Per-category record offset 0x6D |

### 14.3 Cross-off behavior and progress.txt

F10 and F6 both reach the area-quest trigger routine at RVA 0x514F0. Its
selector at RVA 0x52920 maps the nearest eligible live quest trigger to an
area_quests record, applies an 80 m threshold, and writes that record's
crossed-off Boolean at offset 0x58.

- F10 first tries this area-quest mapping with done=true. If no eligible quest
  trigger is found, PostRender implements a separate fallback around RVA
  0x59BFC through 0x59CA0 that crosses off the normal nearest selected marker.
- F6 calls only the area-quest mapping with done=false. It restores the mapped
  area_quests record or does nothing and logs failure. It has no generic
  selected-marker fallback.

F6 is therefore neither a historical undo stack nor a general uncross key.

After each change, the sample calls the writer at RVA 0x5B8F0 from PostRender.
It truncates and rewrites progress.txt with one crossed-off record ID per line.
Automatic completion changes use the same writer. This is synchronous file I/O
inside the rendering callback and must not be copied. The target must enqueue a
value-only persistence event, coalesce changes, and perform atomic replacement
outside PostRender on a bounded worker or non-render lifecycle callback.

### 14.4 config.txt behavior

The reader at RVA 0x4D1C0 is called once from on_unreal_init around RVA 0x660E4.
It constructs config.txt under the mod's dlls directory. If the file is absent,
the reader returns without creating a default file.

The writer at RVA 0x47310 truncates and rewrites config.txt with:

- icones.
- contour.
- taille_icone.
- hauteur_marqueur.
- taille_texte.
- zone_haut and zone_bas.
- echelle_ecran.
- portee_max.
- police.
- repousse_minutes.
- outils_dev = 1 when developer tools are currently enabled.

The last behavior conflicts with the CHANGELOG statement that outils_dev is a
line the mod never writes. The binary is the stronger source.

When a panel widget reports a change, its caller invokes the writer directly
from PostRender. Depending on the widget's change semantics, dragging a control
can cause repeated synchronous truncate-and-rewrite operations across frames.
The native target must only save a real value change, debounce it outside the
render callback, and use one configuration source.

The archive contains no config.txt. Its update-preserves-config property is
therefore primarily a packaging behavior: the package does not ship a file to
overwrite. It is not evidence of a DLL merge or migration mechanism.

### 14.5 Automatic completion behavior

on_unreal_init constructs three paths:

| File | Global | Confirmed role |
|---|---:|---|
| progress.txt | RVA 0x9B088 | Persistent user-facing crossed-off IDs; read at startup and rewritten after state changes. |
| puzzles_releves.txt | RVA 0x9B110 | Developer CSV-style observation export; no read path found. |
| collecte_releves.txt | RVA 0x9B0F0 | Developer observation export; no read path found. |

The progress loader at RVA 0x50EA0 matches saved IDs back to 0x70-byte static
records and sets their done byte at offset 0x58. No game-save mutation API is
used: the file is local overlay state that is later reconciled with read-only
game observations.

Mini-games and supported puzzles have a real read-only game query. The routine
at RVA 0x491F0 resolves DETUtil::CIsClearMiniGameInStandAlone, populates the
reflected parameters WorldContextObject and ID, calls ProcessEvent, and reads
ReturnValue. It covers exactly:

- fly.
- mole.
- wave.
- puzzle_* kinds.

Normal PostRender orchestration invokes this query every 5,000 ms, not every
frame. A developer path invokes the same routine in expanded-report mode. When
a returned value changes local state, progress.txt is synchronously rewritten.
Static analysis cannot prove that this game function is correct for every ID or
safe across every travel state.

Chest completion is not obtained from a chest/save completion API. RVA 0x44160
compares chests_* static markers against live chest snapshots built by the
recurring UObject sweep. It is gated on a new completed sweep generation and:

- Clears absence evidence during its player-movement cooldown.
- Restores a completed marker if a matching live chest Actor reappears.
- Marks a nearby marker completed only after repeated Actor absence.
- Uses a normal player radius of 25 m.
- Uses a conservative 15 m radius when no chest Actors are known.
- Matches live Actors to markers within 3 m.
- Requires two missing sweep generations normally or six when the live list is
  empty.

This is a guarded streaming heuristic, not authoritative save truth. It can
still misclassify an Actor absent because of streaming, floor, interior, class
matching, or lifecycle behavior. It must not replace the current Radar's
consistent save-snapshot semantics.

Puzzle completion has three confirmed paths:

1. The 5,000 ms CIsClearMiniGameInStandAlone query.
2. A generation-gated missing-trigger fallback at RVA 0x44920. It evaluates an
   undone puzzle_* record only within 30 m, resets absence evidence when a live
   trigger exists within 60 m, and marks completion after two missing sweep
   generations.
3. Pressure-plate logic in PostRender. It is generation-gated, evaluates
   puzzles within 80 m, requires at least three plates within 40 m, and marks
   completion only when all observed plates are lit.

Every transition writes progress.txt through the same synchronous render-path
writer. The developer export files are emitted only when their gated tool flags
are set and do not restore completion state.

### 14.6 Recovered cadence map

| Cadence | Confirmed work |
|---|---|
| on_update | Hotkey edge polling, request publication, and hook-install retry while uninstalled. |
| Every PostRender | Enable/context checks, request consumption, current player/camera work, optional panel input/draw, marker projection/draw, and the active dynamic-animal scan chunk. |
| 150 ms | Static status/automatic-completion orchestration and full static point selection; chest/puzzle heuristics additionally require a new completed object-sweep generation. |
| 5,000 ms | CIsClearMiniGameInStandAlone update for fly, mole, wave, and puzzle_* records. |
| 120 callbacks while missing | Controller/font discovery retry. |
| 300 callbacks | Top-level diagnostic summary. |
| 800 ms window | Escape/menu-origin heuristic. |

The sample caches PlayerController and font-related objects across callbacks.
That is a confirmed implementation detail, not a pattern for the target. The
native Radar retains only proven process-lifetime metadata; gameplay instances
remain callback- or sample-local and are invalidated by epoch.

The wall-clock duration of a complete UObject sweep depends on total UObject
count and frame rate even though each PostRender advances at most approximately
4,000 slots. The runtime hitch cost and the correctness of absence-based
completion remain unknown without gameplay measurement.

## 15. Data loading and update model

The DLL enumerates CSV files, recognizes supported headers, and ignores legacy
thgl-prefixed files. It parses the packaged static CSV files at startup and
updates completion or display state during runtime.

It does not:

- Extract current coordinates from game PAKs.
- Regenerate catalogs after a game update.
- Discover new static points during normal play.

The sample author's development tooling may have used object or spawn scans,
but the distributed runtime consumes static authored files. A game update
therefore requires a new package from that author.

DragonSwordWorldRadar's current installation flow is preferable:

~~~text
current game PAK
    -> bounded installation-time extraction
    -> validated generated catalogs
    -> locked game fingerprint
    -> runtime scalar-only use
~~~

The native replacement should retain that flow. It is automatic when the user
runs the installer after an update; it is not a runtime dynamic scan. Current
source directly enforces the Boss, Assault, and mini-game counts 9/40/83. The
1,693 Treasure count is the accepted current generated output, not a general
hard-coded invariant of every future game build. Coordinate or content changes
that still satisfy the validated schema can be regenerated automatically. A
game update that changes a schema, a directly enforced count, or an extraction
rule must fail closed until the generator and its tests are reviewed;
installation must never silently accept a newly shaped catalog.

## 16. Language and package quality observations

- en.txt and fr.txt each contain 104 unique keys.
- Both files lack cat.chests_puzzle even though that kind is present in CSV.
- Each file retains 12 category keys not used by the current CSV files.
- Language-file comments say ASCII while the CHANGELOG says UTF-8.
- Version and UE4SS compatibility statements conflict across the archive.

These inconsistencies reinforce the rule that manifests, catalog counts, ABI
fingerprints, and validation results must be generated from the exact staged
package rather than copied from human-maintained prose.

## 17. What the sample proves and does not prove

### 17.1 Useful independent reference

- A GameViewportClient PostRender detour can reach UCanvas in this game/UE4SS
  family.
- UCanvas Line, Polygon, Texture, and Text primitives can construct a marker
  layer without an external window.
- The camera-space projection formula is available for an optional 3D HUD
  layer.
- DPI-aware logical coordinates and rectangular edge intersection are useful
  implementation references.
- Its CSV schemas provide an independent coordinate cross-check.

### 17.2 Do not copy

- Slot 112 as a universal constant.
- The Layout 128 archive label.
- Per-frame 4,000-object scanning.
- Retained Actor pointers.
- One global original function for multiple patched vtables.
- No-restoration unload behavior.
- Infinite per-frame retry after general drawing faults.
- Per-primitive reflection and heap allocation.
- Synchronous progress/config file rewriting from PostRender.
- Absence-based chest or puzzle completion as authoritative save truth.
- All-point string matching every 150 ms.
- Nearest-one-per-category selection.
- Horizontal-only distance where height matters.
- x < -100000 as a complete map/area model.
- The 84-record mini-game file.
- The sample's Boss, Assault, or chest catalogs as current truth.

### 17.3 Still unknown

- The current game executable's actual PostRender slot and viewport subclass.
- Compatibility of slot 112 with the exact installed UE4SS/game pair.
- Whether another loaded mod already detours the same slot.
- The exact UCanvas parameter layouts and blend behavior under the target SDK.
- Callback thread affinity and synchronization requirements.
- UE4SS hot-unload and FreeLibrary behavior for a vtable-patched user mod.
- Safe coexistence and hook chaining with other native mods.
- Whether cached static UFunction/property metadata remains valid across every
  travel path in this game.
- Exact screen rectangle of the compact minimap at every resolution, UI scale,
  aspect ratio, window mode, and HUD state.
- Actual frame-time improvement, crash safety, and travel behavior.

## 18. Current NativeWorldRadar gap

The existing DragonSwordNativeWorldRadar is a medium-native proof of concept:

~~~text
UE4SS C++ scalar sampler
    -> alternating protocol-v5 files
    -> external PowerShell/WinForms host
    -> transparent GDI+ window
~~~

It already demonstrates fresh 250 ms Controller/Pawn sampling and
epoch-sensitive travel suspension, but it does not remove the external
compositor path. The final native architecture must replace:

- host/DragonSwordNativeWorldRadar.Host.ps1.
- The external WinForms renderer under src/overlay.
- Transparent layered-window and DWM composition.
- File bridge polling and protocol-v5 records.
- CreateProcessW host launch.
- The isolated F10 proof-of-concept control model.
- config/default.ini as a second configuration source.
- Old 34-Fly catalog assumptions.
- Existing stale SDK pins and build-validation metadata.

Those files may remain temporarily as migration evidence, but they must not be
staged in the final in-process package.

Migration governance:

- Treat DragonSwordWorldRadar as the frozen behavior and A/B reference during
  native development. Do not modify it as part of native implementation work.
- Develop and package the replacement only under DragonSwordNativeWorldRadar.
- Never enable or deploy both Radars concurrently; independent process names do
  not prevent duplicate visual work or hotkey/state conflicts.
- Do not delete the existing installed Radar merely because the native DLL
  builds. Archive its source, accepted package, configuration, and runtime
  evidence only after native visual, travel, exit, and performance acceptance.
- Archiving is a deliberate release transition, not part of build or staging.

The current NativeWorldRadar build script pins an older SDK set. Before native
renderer implementation, align it with the exact currently validated SDK
baseline:

| Dependency | Required commit |
|---|---|
| RE-UE4SS | 1c1a1497f942c707f47ba668db75b25e86f6c08a |
| UEPseudo | b2e876da82b17254c04304746341c8fde0ddb37c |
| patternsleuth | da8bfe4c5a464be0ef225c2c9a6ccaa2d9284018 |
| ImGuiColorTextEdit | 6d943aba9f7cef05da80b86dbb0253b63818f95c |

Do not reuse an existing `dist/work/build/native/main.dll` or its stale
validation JSON as
the renderer baseline. Rebuild only after source and fingerprint gates have
been updated.

## 19. Reusable current Radar behavior

The native replacement should port behavior from the current
DragonSwordWorldRadar, not from MnMRadar:

- Current PAK-derived catalogs:
  - 1,693 treasures.
  - 9 Bosses.
  - 40 Assault targets.
  - 83 mini-games: 33 Fly, 40 Mole, and 10 Wave.
- Current save-state filtering and encounter availability semantics.
- Current north-up compact minimap projection.
- Current compact minimap range derived from the game minimap scale.
- Current colors and vector marker shapes.
- The closest two treasure height indicators.
- Enlargement and label only for the closest treasure.
- Current fixed-capacity nearby selection and one-second treasure reselection.
- Current motion prediction based only on numeric scalar history.
- F7/F8 semantics and bounded automatic recovery.
- Epoch/token invalidation during world travel.
- Fresh Engine to current Controller to current Pawn scalar sampling.
- One user configuration source, preferably scripts/config.lua during the
  migration.
- No minimap bitmap or layer caching.

The compact projection currently used by DragonSwordWorldRadar is a north-up
world-relative transform:

~~~text
screenX = minimapCenterX
        + (markerWorldX - playerWorldX) / worldRadius * minimapRadiusPixels

screenY = minimapCenterY
        + (markerWorldY - playerWorldY) / worldRadius * minimapRadiusPixels
~~~

It is independent of MnMRadar's camera projection. The first native milestone
should reproduce this behavior inside the Canvas at a validated minimap screen
rectangle. Rotation should not be added unless direct runtime evidence shows
that the game minimap rotates.

### 19.1 Current compact baseline constants

These values are current-source migration baselines, not proof that the native
Canvas rectangle is correct at runtime. Revalidate them at every supported UI
scale, aspect ratio, resolution, and window mode.

| Behavior | Current value |
|---|---|
| Reference client size | 2560 x 1440 |
| Display scale | min(clientWidth / 2560, clientHeight / 1440) |
| Compact minimap square | 360 x 360 reference pixels |
| External compact window height | 400 reference pixels including the status strip |
| Right margin | 40 reference pixels |
| Top margin | 37 reference pixels |
| Absolute minimap center | (clientRight - 220 * scale, clientTop + 217 * scale) |
| Radar circle radius | 170 * scale pixels |
| Nearby Treasure capacity | 80 |
| Treasure reselection | 1,000 ms |
| Player Z comparison offset | -150 Unreal units |
| Active compact presentation | 33 ms |
| Motion prediction horizon | Clamp to 250 ms |
| Prediction stale freeze | 500 ms |
| Predictor source-gap reset | 1,000 ms |
| Whole motion-state stale hide | 2,500 ms |

The 400-pixel external-window height and its status strip are not automatically
part of the native UCanvas minimap. The first native renderer should draw only
the validated 360-pixel minimap region. World time, weather, and the status
strip are explicitly deferred until compact Radar parity and performance are
accepted.

## 20. Target architecture

~~~text
Installer
  current PAK -> validated compact catalogs -> game fingerprint
                                      |
                                      v
Lua control plane                 Native DLL
  F7/F8 and config                 immutable catalogs
  fresh scalar sampling   ---->    numeric control receiver
  minimap range                    visibility snapshot
  lifecycle epoch                  PostRender hook
  bounded recovery        <----    atomic fault status
                                      |
                                      v
                               callback UCanvas only
~~~

### 20.1 Ownership

Lua control plane:

- Own F7/F8, configured layers, and bounded recovery.
- Preserve the existing epoch/token world-transition behavior.
- Walk fresh Engine, Controller, and Pawn links only at the existing bounded
  sampling cadence.
- Publish only finite numeric and Boolean values.
- Never pass a UObject pointer to the DLL.
- Advance activation_generation on every F7/F8 transition. F8 publishes and
  latches a new disabled generation before shutdown work; F7 may publish an
  enabled state only under a later generation after its stability gate.
- While enabled or a bounded recovery is pending, read one atomic native status
  record on the existing 250 ms control cadence. Manual F8 stops this query; do
  not add another polling loop.

Native DLL:

- Load only validated generated catalogs matching the current game
  fingerprint.
- Receive scalar snapshots through a direct registered native Lua function.
- Accept exactly one active publisher session at a time and reject superseded
  Lua-state generations.
- Use the mod-name on_lua_start/on_lua_stop overload, accept only the exact
  DragonSwordNativeWorldRadar Lua mod, and register only in its canonical main
  state. Do not register equivalent publishers in main, async, and hook states.
- Publish snapshots through a preallocated, C++ data-race-free structure whose
  payload accesses are atomic; do not assume an ordinary seqlock or two-slot
  buffer makes non-atomic concurrent payload access legal.
- Render only from immutable numeric snapshots.
- Use only the Canvas received by the current PostRender callback and discard
  it before returning.
- Keep save/database work outside PostRender and publish immutable
  visibility/availability snapshots.
- Own no transparent window, external host, bridge file, or polling loop.
- Expose one non-blocking scalar status query containing hook state,
  fault_generation, and a closed numeric fault code. It performs atomic loads
  only and never returns pointers.
- While the F8 disabled latch is active, stop catalog selection, maintenance,
  directory/fingerprint polling, save copying, and SQLCipher work. A resident
  worker may wait dormant on a signal; it may not wake periodically to poll.
- The unavoidable F8 cost of a resident manual detour is limited to chaining the
  original and one atomic disabled-state branch. No custom Canvas primitive,
  snapshot selection, status polling, or background maintenance runs.

### 20.2 Snapshot boundary

A caller-owned input and a DLL-owned render snapshot must be distinct types. A
proposed input schema is:

~~~cpp
struct RadarControlInputV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;

    uint64_t source_sequence;
    uint64_t activation_generation;
    uint64_t world_epoch;

    uint8_t enabled;
    uint8_t context_valid;
    uint8_t transition_suspended;
    uint8_t context_mode;
    uint8_t has_player_z;
    uint8_t show_treasures;
    uint8_t show_bosses;
    uint8_t show_assaults;
    uint8_t show_minigames;
    uint8_t show_height;
    uint8_t show_treasure_types;

    double player_x_cm;
    double player_y_cm;
    double player_z_cm;
    double minimap_world_radius_cm;
    double text_scale;
};

struct RenderSnapshotV1 {
    uint64_t publisher_session_generation;
    uint64_t accepted_sequence;
    uint64_t activation_generation;
    uint64_t world_epoch;
    uint64_t received_qpc_ticks;

    uint32_t catalog_generation;
    uint32_t visibility_generation;

    RadarControlPayloadV1 control;
};
~~~

RadarControlPayloadV1 is the validated control portion copied from the input;
it contains no ownership or generation counters. The registration path assigns
a publisher-session generation and captures it in that Lua state's native
callable, so Lua cannot choose or spoof it. The DLL assigns accepted_sequence
and received_qpc_ticks and obtains catalog_generation and
visibility_generation only from their native owners. The Lua publisher never
submits those DLL-owned values.

The exact ABI can change before implementation. Its invariant cannot:

- No Pawn, Controller, Canvas, Viewport, World, Widget, FString owner, Lua
  object, or other UObject crosses the boundary or survives a callback.
- The DLL assigns the active publisher-session generation. A Lua reload or new
  Lua state invalidates the old session before accepting its first snapshot.
- Reject unknown version, size, non-finite values, a superseded publisher
  session, non-increasing source sequence, stale activation generation, stale
  epoch, and invalid radius.
- After accepting disabled generation G, reject every enabled input with
  activation_generation <= G. Only a later post-stability F7 generation can
  clear the disabled latch. This prevents a pre-F8 call that enters late from
  re-enabling work.
- context_mode is a closed numeric enum. Mode changes reset motion prediction
  even when the world epoch is unchanged. has_player_z controls all height
  comparisons; a numeric zero cannot substitute for an unavailable Z sample.
- text_scale and show_treasure_types preserve the current user configuration
  semantics and are range-validated before publication.
- The DLL assigns accepted_sequence and timestamps acceptance with its own QPC.
  Do not compare a Lua monotonic timestamp with a C++ clock unless a shared
  clock domain is explicitly proven.
- A transition or invalid context immediately publishes a disabled snapshot.
- A failed restart is bounded and cannot loop indefinitely.

An ordinary C++ seqlock over a non-atomic struct has a language-level data race,
even if sequence checks usually detect a torn copy. A two-slot buffer can also
be overwritten after wraparound while a reader still uses the old inactive
slot. A valid implementation must use one of these proven alternatives:

- Atomic payload words plus an atomic odd/even sequence, with a bounded reader
  retry and exactly one serialized writer. Require compile-time/runtime proof
  that the chosen atomic word is lock-free on the target x64 toolchain.
- A fixed slot bank with atomic ownership/reference state that proves a writer
  cannot touch a slot while PostRender reads it.
- Another design whose C++ memory-model proof and no-blocking render behavior
  are covered by deterministic stress tests.

If more than one registered Lua state can call the publisher, serialize writers
outside PostRender and accept calls only from the active session generation.
The render callback must never acquire that writer lock. Publisher shutdown and
Lua-state replacement must be explicit lifecycle events, not inferred from a
sequence reset.

The intended normal case is stricter: one exact-name Lua mod, one canonical
main-state callable, and one control-callback writer. Reject calls from an
unexpected thread or state unless a separately proven serialized handoff is
implemented.

The exact pinned LuaMadeSimple API exposes register_function but no matching
unregister_function. The callable is stored in a process-global function table.
on_lua_stop must invalidate the publisher session before that Lua state is
destroyed, and every publisher invocation must participate in the module's
in-flight accounting. Destroying a Lua state does not by itself prove that the
process-global callable no longer references DLL code. If all registered
callables cannot be invalidated or proven unreachable, the DLL must remain
resident for the process lifetime and must not advertise hot unload.

Canvas dimensions and the callback's current drawing object belong to the
render callback and should not be copied into long-lived control state.

### 20.3 Hot-path budget rules

PostRender must not perform:

- File I/O.
- SQL or SQLCipher work.
- PAK access.
- UObject enumeration or search.
- Game-instance discovery.
- Locks that can block.
- Heap allocation in steady state.
- String formatting or routine logging.
- Per-marker UFunction or property-name lookup.

Allowed steady-state work:

- Read one coherent numeric control snapshot.
- Read immutable catalog and visibility generations.
- Reject stale or disabled state.
- Query callback-local Canvas dimensions.
- Transform bounded nearby records into a fixed-capacity draw buffer.
- Issue validated UCanvas primitives.
- Increment allocation-free diagnostic counters when debug mode is explicitly
  enabled.

### 20.4 Native fault feedback and bounded recovery

Automatic recovery requires a return path. A one-way Lua-to-DLL publisher is
insufficient. The DLL should expose a direct scalar query equivalent to:

~~~cpp
struct RadarNativeStatusV1 {
    uint32_t version;
    uint32_t hook_state;
    uint32_t fault_code;
    uint32_t reserved;
    uint64_t fault_generation;
    uint64_t accepted_activation_generation;
};
~~~

The query performs only atomic loads. While enabled or a bounded recovery is
pending, Lua reads it on the already existing 250 ms control callback, not from
PostRender and not through a new timer. A manual F8 stops the query. A new
recoverable fault generation may request the existing bounded F8-to-F7
lifecycle restart. The DLL must already be fail-closed before publishing that
status; recovery is never required to make the hook safe.

Rules:

- Use a closed fault-code enum with an explicit recoverable/fatal table.
- ABI mismatch, lost hook ownership, corrupted catalog, unload uncertainty,
  and repeated render faults are fatal until a manual restart or process exit.
- A transient missing map/widget or intentionally unavailable expanded-map
  state is not an error and cannot arm recovery.
- Handle each fault generation at most once.
- Never exceed the existing bounded recovery ceiling, currently three attempts
  per session. A failed final attempt remains disabled without further polling
  work or retry.
- Every recovery advances activation_generation, so an older enabled publisher
  call cannot race after the recovery's F8 latch.
- If the status function is unavailable or invalid, Lua fails closed and waits
  for manual F7. The render callback never calls into Lua.

## 21. Safe hook requirements

The sample demonstrates where to investigate, not a safe production hook. The
new implementation requires all of the following.

### 21.1 Installation

- Gate the build on exact executable, UE4SS, SDK, and generated-data
  fingerprints.
- Resolve the current GameViewportClient through a bounded known property chain
  or supported hook API; do not enumerate the full UObject array.
- Derive and verify the target slot for the exact build. Slot 112 is only an
  audited candidate anchor.
- Validate that the candidate pointer is executable and belongs to the expected
  module or known hook chain.
- Track each patched vtable as a separate record:

~~~cpp
struct HookRecord {
    void** slot_address;
    PostRenderFn original;
    void* vtable_identity;
};
~~~

- Never overwrite an unknown detour silently.
- After making the aligned pointer slot writable, install with an atomic
  compare-exchange from the validated expected original to this detour.
  VirtualProtect changes page permissions; it is not synchronization.
- Treat compare-exchange failure as lost ownership. Do not overwrite the new
  value, disable installation, and roll back only records still owned by this
  mod.
- Install once with bounded retry and exponential or lifecycle-based backoff.
- Roll back partial installation when a required validation or page-protection
  restoration fails.
- Publish enabled state only after the complete installation transaction
  succeeds.

### 21.2 Dispatch

- Call the correct original exactly once.
- Define and test whether custom drawing occurs before or after original.
- Keep exceptions from this mod's bookkeeping and custom drawing inside the
  detour ABI. Call the chained/original game PostRender outside this mod's C++
  catch and SEH scopes so an original fault is not swallowed or misattributed.
- Maintain an atomic in-flight callback counter.
- Reject drawing while shutting down, suspended, stale, or incompatible.
- Trip a bounded fault budget to a disabled fail-closed state and publish a new
  scalar fault generation for the control plane.
- Do not automatically re-enable more than the configured bounded recovery
  count.
- Never retain the callback Canvas or viewport instance.

### 21.3 Removal and module lifetime

Safe hot unload is not proven by MnMRadar. A simple in-flight counter is not
enough by itself because another thread can fetch the detour address before the
slot is restored and enter it later.

Required sequence:

1. Atomically reject new snapshot publication and custom drawing, and mark the
   hook as shutting down.
2. In on_lua_stop, invalidate every publisher session, and disable or unregister
   every UE4SS callback using supported APIs so no external entry point can
   begin new work. Because LuaMadeSimple has no confirmed unregister_function,
   do not assume the registered callable can be removed.
3. Restore every aligned slot with an atomic compare-exchange from this detour
   to that record's original. A mismatch means ownership was lost: never
   overwrite the later hook.
4. Synchronize with the proven callback thread and wait for a confirmed
   post-restoration quiescent point. This closes the race where a thread fetched
   the detour address immediately before restoration.
5. Wait a bounded time for all callbacks and publisher calls that already
   entered this module to leave.
6. Only after every external entry point is quiescent, release non-UObject
   resources and invalidate process-lifetime metadata.
7. Unload the DLL only if the exact UE4SS/module-lifetime contract proves the
   code cannot be entered again.

If slot ownership, callback quiescence, or loader lifetime cannot be proven,
the implementation must not rely on returning from uninstall_mod while a
manual detour can still execute. Before installing the detour, Phase 1 must
prove that UE4SS keeps the module resident or deliberately establish a
process-lifetime module pin and design its disabled state for that lifetime. If
neither is acceptable, do not install the manual detour. Do not claim hot-unload
support until the exact runtime has tested this contract.

## 22. Development sequence

### Phase 0: compatibility lock

- Update NativeWorldRadar to the validated SDK pins.
- Add exact executable and UE4SS fingerprints.
- Generate current catalogs into the new mod without modifying the existing
  Radar.
- Add package-derived schema, count, hash, and game-fingerprint validation.
- Confirm that same-shape coordinate changes regenerate during installation and
  that a changed schema/count fails closed for review.
- Document the exact PostRender candidate-discovery method.

Exit criterion: all static gates pass and no runtime or installed files change.

### Phase 1: PostRender canary

- Remove no existing architecture yet.
- Implement a default-off, fixed-point UCanvas canary.
- Call original exactly once.
- Draw no catalog and perform no game-object scan.
- Add fault budget, per-vtable original tracking, and restoration logic.
- Instrument only aggregate callback count, maximum duration, fault count, and
  lifecycle state.

Runtime checks:

- Startup with canary disabled.
- One enable/disable cycle.
- Windowed, borderless, and target resolutions.
- UI-scale changes.
- Repeated teleport and dungeon transitions.
- Main-menu return if supported.
- Normal game exit.
- UE4SS mod disable/unload only if explicitly supported.
- Coexistence with every installed native mod that could touch the viewport.

Exit criterion: no crash, no duplicated original call, no stale draw after
disable, and no unbounded retry.

### Phase 2: direct numeric bridge

- Register a Lua-to-DLL scalar publisher using the exact SDK's on_lua_start and
  LuaMadeSimple registration surface.
- Filter the exact Lua mod name and register only one canonical main-state
  publisher; reject duplicate or unexpected-state registration.
- Implement on_lua_stop session invalidation and publisher in-flight accounting;
  prove callable destruction or require process-lifetime DLL residence.
- Assign a native publisher-session generation for each accepted Lua state and
  explicitly supersede the prior state on reload.
- Publish enabled, context, epoch, activation generation, source sequence,
  player XYZ, minimap range, and layer flags. Stamp acceptance with the DLL's
  own QPC.
- Replace bridge files with a preallocated, race-free coherent snapshot whose
  concurrency model is stress-tested under rapid reload and publication.
- Add the atomic DLL-to-Lua status query on the existing 250 ms control cadence
  and prove one bounded recoverable F8-to-F7 cycle plus a fatal no-retry case.
- Draw one player-relative test marker.
- Keep the external renderer available only as an A/B reference, never enabled
  concurrently.

Exit criterion: scalar updates, F7/F8, travel invalidation, and stale snapshot
rejection are proven in game.

### Phase 3: compact minimap visual parity

- Load the current 1,693/9/40/83 generated catalogs.
- Port the current north-up compact projection.
- Port the existing colors and allocation-free vector icon shapes.
- Accept a deterministic in-memory completion/availability fixture so visual
  filtering can be tested without file or database work in PostRender. This
  phase is a development canary, not a user replacement.
- Port the two closest treasure height indicators.
- Enlarge and label only the closest treasure.
- Use fixed-capacity selection and drawing buffers.
- Keep the expanded map disabled.

Exit criterion: marker position, range, height, fixture filtering, and lifecycle
match the current Radar in controlled screenshots and gameplay. Persistent
save-state parity remains explicitly pending Phase 4.

### Phase 4: native save-state visibility worker

- Move save snapshot parsing and SQLCipher reads to a bounded low-priority
  worker.
- Never open live game save files directly; preserve consistent snapshot
  copying and fingerprint checks.
- Publish an immutable visibility/availability snapshot plus generation. It
  must distinguish unavailable data from a valid empty result and represent:

    - Treasure and mini-game completion.
    - Boss respawn availability and recovery time.
    - Assault task and time-condition availability.
    - Applied ignore and alias overrides.
- Stop and join the worker safely during shutdown. If safe unloading is not
  proven, permit only the explicitly validated process-lifetime resident mode
  defined by Phase 1.
- On F8, cancel or invalidate queued work and leave the resident worker dormant
  with no directory, fingerprint, copy, SQLCipher, or timer polling until a
  later accepted activation generation.

Exit criterion: persistent treasure and encounter visibility matches the
current Radar with no database or file work on PostRender. This is the first
phase eligible for full user-visible compact Radar replacement testing.

### Phase 5: performance and stability acceptance

Compare the same scene and camera path in:

1. Current DragonSwordWorldRadar F7.
2. DragonSwordNativeWorldRadar F7.
3. DragonSwordNativeWorldRadar F8.

Use repeated runs and an external PresentMon or CapFrameX capture. Record:

- Median, 95th, 99th, and 99.9th percentile frame time.
- One-percent and 0.1-percent lows.
- Counts and timing of visible stutters.
- Game-process CPU, GPU, and DWM GPU usage.
- Native PostRender average, maximum, and percentile duration.
- Marker count and enabled layers.
- Travel, toggle, and resolution-change events.

Do not accept the native version from theory, a successful build, clean logs,
or average FPS alone. It must reduce the actual F7-to-F8 smoothness gap without
introducing new periodic frame-time spikes.

Only after this phase should the current DragonSwordWorldRadar be archived.

### Phase 6: expanded map

The expanded map is intentionally deferred. MnMRadar provides no useful
expanded-map transform or visibility mechanism. Implement it only from direct
game-state evidence after the compact native renderer is accepted.

## 23. Acceptance matrix

| Area | Required proof |
|---|---|
| ABI | Exact fingerprint accepted; wrong fingerprint refuses to hook. |
| Original dispatch | Original PostRender called exactly once per callback. |
| Disabled state | F8 latches a new activation generation and immediately stops custom drawing, selection, maintenance, directory/fingerprint polling, save copying, and SQLCipher work. Resident workers wait dormant without periodic polling. |
| Stale enable race | An enabled publication already in flight before F8 cannot clear its disabled latch or restart work. |
| Recovery | One bounded recovery sequence; failure cannot retry forever. |
| Fault feedback | DLL status is scalar and atomic; recoverable/fatal codes are distinct, and Lua reads it only on the existing 250 ms control cadence. |
| UObject lifetime | No Pawn, Controller, Canvas, Viewport, World, or Widget retained. |
| Discovery | No recurring full UObject enumeration. |
| Snapshot concurrency | Payload access is data-race-free under the C++ memory model; the render reader is bounded and non-blocking. |
| Lua reload | Exactly one publisher session is active; stale states and reset sequences are rejected. |
| Render hot path | No file, database, PAK, blocking lock, routine allocation, or reflected name search. |
| Catalog | Exactly 1,693 Treasure, 9 Boss, 40 Assault, and 83 mini-game records for the locked current game build. |
| Mini-games | Exactly 33 Fly, 40 Mole, and 10 Wave; Fly 11024 excluded. |
| Visual parity | Positions, range, colors, icons, nearest-two height indicators, and closest-only enlargement match. |
| Resolution | Correct at every supported resolution, aspect ratio, UI scale, and window mode. |
| Travel | Repeated town, field, dungeon, teleport, story, and return transitions do not draw stale data or crash. |
| Hook coexistence | Existing hook chains are detected and never overwritten blindly. |
| Hook ownership | Install and restore use aligned atomic compare-exchange; a mismatch never overwrites a later hook. |
| Removal | External entry points are disabled before resources; quiescence and module residence are proven before unload. |
| Performance | Repeated gameplay A/B shows a real reduction in frame-time loss and stutter relative to the external Radar. |
| Packaging | No external host, transparent overlay, bridge files, stale catalogs, or mismatched metadata in the final package. |

## 24. Source-of-truth order

When evidence conflicts, use this order:

1. Exact current game executable and installed UE4SS fingerprints.
2. Exact validated SDK source and ABI.
3. Current PAK-derived generated catalogs and their tests.
4. Current DragonSwordWorldRadar behavior confirmed in gameplay.
5. Exact MnMRadar binary and CSV evidence.
6. MnMRadar README, CHANGELOG, archive title, and comments.

This order prevents the known Layout 128, version, encoding, mini-game count,
and data-coordinate inconsistencies from becoming native implementation
assumptions.

## 25. Key RVA index for future research

| RVA | Meaning |
|---:|---|
| 0x3ACC0-0x3B2D1 | Incremental dynamic-animal UObject scan |
| 0x3A3A0 | Derived deleting destructor |
| 0x3B2E0 | Canvas panel toggle/control ownership |
| 0x40770-0x433C0 | Canvas settings panel draw/input path |
| 0x44160 | Chest live-Actor absence completion heuristic |
| 0x44920 | Puzzle missing-trigger completion fallback |
| 0x45930 | K2_DrawLine wrapper |
| 0x45AF0 | K2_DrawPolygon wrapper |
| 0x45DF0 | K2_DrawTexture wrapper |
| 0x462A0 | K2_DrawText wrapper |
| 0x47210-0x47306 | DPI/logical screen scale |
| 0x47310 | Synchronous config.txt writer |
| 0x488F0 | PostRender detour |
| 0x48FA0 | Vtable hook installer |
| 0x491F0 | CIsClearMiniGameInStandAlone completion update |
| 0x4D1C0 | Startup config.txt reader |
| 0x50EA0 | Startup progress.txt reader |
| 0x514F0 | Nearest live area-quest-trigger map and cross/restore |
| 0x523EE | Reachable ProcessEvent pre-callback registration path |
| 0x52660-0x5291A | Camera world-to-screen projection |
| 0x58E40 | Main custom drawing routine |
| 0x5B6B0-0x5B7A1 | Current Pawn location path |
| 0x5B8F0 | Synchronous progress.txt rewrite |
| 0x5DE50-0x5E4AA | 150 ms static point selection |
| 0x65EB0 | on_unreal_init |
| 0x66120 | on_update |
| 0x68BF0 | start_mod export |
| 0x68D50 | uninstall_mod export |
| 0x77860 | Structured-exception filter |
| 0x862F0 | Derived mod vtable |
| 0x9E77D | Installed flag |
| 0x9E868 | Single stored original PostRender pointer |

## 26. Immediate implementation decision

The next code change should be Phase 0 plus a default-off Phase 1 hook canary,
not a complete renderer rewrite in one step. That isolates the only genuinely
new crash-critical mechanism: exact-build PostRender installation, dispatch,
fault containment, and removal.

Once that canary survives runtime travel and exit testing, the existing Radar's
numeric minimap behavior can be moved behind it without copying MnMRadar's
enumeration, pointer retention, hot-path reflection, static data, or unsafe
unload design.
