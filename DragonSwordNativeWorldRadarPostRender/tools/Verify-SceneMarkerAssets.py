#!/usr/bin/env python3
"""Verify SG-09's interior backing and preserved Scene geometry/resources."""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

from PIL import Image

EXPECTED = {
    "TreasureOther": "treasure-other.tga",
    "TreasureMiniGame": "treasure-mini-game.tga",
    "TreasureMap": "treasure-map.tga",
    "TreasurePuzzle": "treasure-puzzle.tga",
    "AreaQuest": "area-quest.tga",
    "MiniGame": "mini-game.tga",
}
# Independently reconstructed from SCENE_SG04_SOURCE_ONLY_PREVIEW.json before
# the native styles were removed. This pins all 72 rectangles/colors/angles,
# including the four Treasure colors and the shared UI-only chevron.
SG04_GEOMETRY_SHA256 = "C9E36C49CFC1E29E54C452FBFF6C62A168D6C2BFDF5CAB04E53EAA2EBC7C08B0"
ACCEPTED_GEOMETRY_SHA256 = "103849493F0C9230D113B87480B21682B5CF16C636529E728A2182A67268DE3B"
SG04_SOURCE_SHA256 = "80B6D324C8D090541DF40382D2FA0632C0F5C377E717FC3DF92B91FF60891AD0"
SG06_AREA_PIXELS = "314A3CF68D0065E8CA16FCEDB3B932580EE1510CB1EFF50BDE5411A1DAB7133E"
SG09_AREA_PIXELS = "8308D1016DC3D19850C51147D66B163A422A70CB217C51CF953B5D1BFF846943"
# SG-06 keeps these five complete textures byte-for-byte. Area Quest has a
# separately approved geometry hash above; its glyph may grow, never its v.
SG05_UNCHANGED_PIXELS = {
    "TreasureOther": "E02B94016C0EB60657AA6264484514BD550D35F0DB2252B643B64D70A749F621",
    "TreasureMiniGame": "D26E808AFB118746C37CED3D94ED1C2FD8330CC35E7F3D6CFB1216D1B8FD70FA",
    "TreasureMap": "9089276D76EB71DF71BF9130967B8942F81BA41FD5C0DF844FDCF0C515B18AEC",
    "TreasurePuzzle": "FFBF413336594316491D38FCFB2DE5F9A8B2FA4243D60F00F9D830EA9539F20D",
    "MiniGame": "A2F6A1E72E1E714E510C145F5E565C45F20AF953E1B0954C81CAD6DED2AA6442",
}


def original_area_glyph() -> list[dict]:
    # Independent SG-04 record: restore just these eight entries to prove
    # that all other geometry, colors and all six chevrons are unchanged.
    records = ((11.4, 11.4, 12, 1.4, -45, [171, 176, 184, 255]),
               (20.6, 11.4, 12, 1.4, 45, [171, 176, 184, 255]),
               (11.4, 20.6, 12, 1.4, 45, [171, 176, 184, 255]),
               (20.6, 20.6, 12, 1.4, -45, [171, 176, 184, 255]),
               (16, 16, 8.1, 3.5, 0, [9, 14, 19, 240]),
               (13.4, 16, 1.3, 1.3, 0, [255, 255, 255, 255]),
               (16, 16, 1.3, 1.3, 0, [255, 255, 255, 255]),
               (18.6, 16, 1.3, 1.3, 0, [255, 255, 255, 255]))
    return [{"x": float(x), "y": float(y), "w": float(w), "h": float(h),
             "angle": float(angle), "rgba": rgba} for x, y, w, h, angle, rgba in records]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def unique_object(pairs: list[tuple]) -> dict:
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def verify_area_backing_pixels(image: Image.Image, baseline: Image.Image, check) -> None:
    """Check actual pixels without consulting the backing generator or manifest."""
    check(image.size == baseline.size == (128, 144), "Area backing changed the canvas")
    check(image.getchannel("A").getbbox() == baseline.getchannel("A").getbbox(),
          "Area backing expanded the existing footprint")
    check(image.crop((0, 108, 128, 144)).tobytes() == baseline.crop((0, 108, 128, 144)).tobytes(),
          "Area backing changed the lower v or its padding")
    for point in ((64, 40), (64, 88), (42, 60), (86, 68)):
        red, green, blue, alpha = image.getpixel(point)
        check(145 <= alpha <= 170 and 45 <= red <= 53 and 60 <= green <= 71 and 79 <= blue <= 91,
              "Area diamond interior needs a restrained translucent gray-blue backing")
    differences = 0
    solid_rails = solid_dots = 0
    for y in range(144):
        for x in range(128):
            old, new = baseline.getpixel((x, y)), image.getpixel((x, y))
            if old != new:
                differences += 1
                check(abs(x / 4 - 16) + abs(y / 4 - 16) <= 10.5,
                      "Area backing spills outside the existing diamond interior")
            if old in ((171, 176, 184, 255), (255, 255, 255, 255)):
                check(new == old, "Area backing obscures a solid outline or dot pixel")
                solid_rails += old == (171, 176, 184, 255)
                solid_dots += old == (255, 255, 255, 255)
    check(3000 <= differences <= 3600 and solid_rails >= 500 and solid_dots >= 60,
          "Area backing lost its bounded fill or preserved solid symbols")


def verify(root: Path, module: Path, rebuild: bool) -> int:
    count = 0

    def check(condition: bool, reason: str) -> None:
        nonlocal count
        count += 1
        if not condition:
            raise ValueError(reason)

    entries = list(root.iterdir())
    check({entry.name for entry in entries} == set(EXPECTED.values()) | {"manifest.json"},
          "Scene assets must contain exactly six named TGAs and manifest.json")
    check(all(entry.is_file() and not entry.is_symlink() for entry in entries),
          "No directory, symlink or unexpected payload entry is allowed")
    manifest = json.loads((root / "manifest.json").read_text("utf-8"), object_pairs_hook=unique_object)
    check(manifest["schema_version"] == 1 and manifest["reference_size"] == [32, 36]
          and manifest["anchor"] == [16, 16], "Reference bounds/anchor changed")
    check(manifest["raster_scale"] == 4 and manifest["supersample"] == 8,
          "Scene raster quality contract changed")
    check(manifest["source_sg04_sha256"] == SG04_SOURCE_SHA256, "Accepted source identity changed")
    geometry = json.dumps(manifest["geometry"], ensure_ascii=True, sort_keys=True,
                          separators=(",", ":")).encode("ascii")
    check(sha256(geometry) == manifest["geometry_sha256"] == ACCEPTED_GEOMETRY_SHA256,
          "Approved SG-06 Area Quest geometry or preserved SG-04 geometry changed")
    check(manifest["design_revision"] == "SG-09"
          and manifest["area_quest_adjustment"] == {"glyph_scale": 1.18, "grey_stroke_width": 1.8},
          "SG-09 must retain SG-06's Area Quest body and 1.8-unit grey rails")
    check(manifest.get("area_quest_backing") == {
        "points": [[16, 6.4], [25.6, 16], [16, 25.6], [6.4, 16]], "rgba": [49, 66, 85, 156]},
        "Only the approved translucent diamond-interior backing may be added")
    original = copy.deepcopy(manifest["geometry"])
    original["AreaQuest"][:8] = original_area_glyph()
    original_bytes = json.dumps(original, ensure_ascii=True, sort_keys=True,
                                separators=(",", ":")).encode("ascii")
    check(sha256(original_bytes) == manifest["source_sg04_geometry_sha256"] == SG04_GEOMETRY_SHA256,
          "Chest/flag geometry, shared v or original SG-04 provenance changed")
    check(set(manifest["geometry"]) == set(EXPECTED)
          and all(len(pieces) == 12 for pieces in manifest["geometry"].values()),
          "Exactly six accepted 12-piece designs are required")
    builder = module / "tools/Build-SceneMarkerAssets.py"
    check(manifest["generator_sha256"] == sha256(builder.read_bytes()), "Stale generated assets")
    spec = importlib.util.spec_from_file_location("scene_asset_baseline", builder)
    baseline_builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(baseline_builder)
    area_baseline = baseline_builder.raster_pieces(manifest["geometry"]["AreaQuest"])
    check(sha256(baseline_builder.tga_bytes(area_baseline)) == SG06_AREA_PIXELS,
          "The preserved pre-backing Area Quest raster no longer matches SG-06")
    check(len(manifest["files"]) == 6
          and {entry["kind"]: entry["path"] for entry in manifest["files"]} == EXPECTED,
          "Manifest filename/kind order mapping changed or duplicated")
    expected_header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 128, 144, 32, 0x28)
    fingerprints = set()
    for entry in manifest["files"]:
        path = root / entry["path"]
        payload = path.read_bytes()
        digest = sha256(payload)
        fingerprints.add(digest)
        check(len(payload) == entry["bytes"] == 73746 and payload[:18] == expected_header,
              f"{path.name}: not exact uncompressed 128x144 top-left BGRA TGA")
        check(digest == entry["sha256"], f"{path.name}: checksum mismatch")
        if entry["kind"] in SG05_UNCHANGED_PIXELS:
            check(digest == SG05_UNCHANGED_PIXELS[entry["kind"]],
                  f"{path.name}: SG-06 must preserve all chest/flag pixels")
        else:
            check(digest == SG09_AREA_PIXELS,
                  f"{path.name}: SG-09 Area Quest pixels differ from the approved interior backing")
        check(entry["width"] == 128 and entry["height"] == 144, f"{path.name}: stale size")
        image = Image.open(path).convert("RGBA")
        if entry["kind"] == "AreaQuest":
            verify_area_backing_pixels(image, area_baseline, check)
        alpha = image.getchannel("A")
        check(list(alpha.getbbox() or ()) == entry["alpha_bounds"], f"{path.name}: alpha bounds mismatch")
        check(all(alpha.crop(box).getbbox() is None for box in
                  ((0, 0, 128, 1), (0, 143, 128, 144), (0, 0, 1, 144), (127, 0, 128, 144))),
              f"{path.name}: opaque texture border or clipped geometry")
        check(alpha.crop((20, 20, 108, 108)).getbbox() is not None
              and alpha.crop((40, 108, 88, 136)).getbbox() is not None,
              f"{path.name}: missing glyph or lower chevron")
        check(alpha.getpixel((64, 112)) == 0 and alpha.getpixel((64, 135)) == 0,
              f"{path.name}: chevron gap or bottom padding changed")
        raw = image.tobytes()
        check(all(raw[i:i + 3] == b"\0\0\0" for i in range(0, len(raw), 4) if raw[i + 3] == 0),
              f"{path.name}: hidden RGB garbage in transparent pixels")
        check(alpha.getextrema() == (0, 255), f"{path.name}: no opaque glyph core")
    check(len(fingerprints) == 6, "All six designs must have distinct final pixels")

    # Runtime must use the same kind-to-file mapping and canvas. Do not import
    # this build script into the game, and never reload assets in update_unsafe.
    source = (module / "src/native/scene_umg_renderer.cpp").read_text("utf-8-sig")
    header = (module / "src/native/scene_umg_renderer.hpp").read_text("utf-8-sig")
    runtime_files = re.search(r"kMarkerTextureFiles\{\{(.*?)\}\};", source, re.S)
    check(runtime_files is not None
          and re.findall(r'L"([^"]+\.tga)"', runtime_files.group(1)) == list(EXPECTED.values()),
          "Runtime texture enum mapping must match all six assets")
    check("kMarkerTextureCount = 6" in header and "kMarkerPieces" not in header
          and "PieceStyle" not in source and "/Script/UMG.Border" not in source,
          "Scene must use fixed shared Image resources, without the old Border pool")
    update = source.split("bool SceneUmgRenderer::update_unsafe(", 1)[1].split(
        "bool SceneUmgRenderer::run_guarded(", 1)[0]
    check(not any(token in update for token in
                  ("NewObject", "import_file_as_texture_", "FString", "filesystem", "FindObject", "GetPropertyByName")),
          "Scene update must not allocate widgets, import files or resolve properties")
    check("marker.x != submitted_positions_[i].x" in update
          and "marker.y != submitted_positions_[i].y" in update
          and "dx * dx + dy * dy > 0.25 * 0.25" not in update
          and "submitted_positions_[i] = {marker.x, marker.y}" in update
          and "marker.x - 16.0, marker.y - 16.0" in update,
          "Submitted-position reuse must preserve the SG-04 anchor and submit changed subpixel motion")
    check("if (!activation_ || suppressed_ || state_ != SceneUmgRendererState::Attached)" in source,
          "Suppressed Scene updates must return before projection and widget traversal")
    check("texture_keepers_ = {};" in source and "marker_textures_ = {};" in source
          and "position_valid_ = {};" in source,
          "Travel/detach must clear the texture and position weak-state caches")
    if rebuild:
        with tempfile.TemporaryDirectory(prefix="dsnwr-scene-assets-") as temporary:
            generated = Path(temporary) / "scene"
            subprocess.run([sys.executable, "-X", "utf8", str(builder), "--output", str(generated)],
                           check=True, stdout=subprocess.DEVNULL)
            check({p.name for p in generated.iterdir()} == {p.name for p in entries},
                  "Rebuild output entries changed")
            for entry in entries:
                check(entry.read_bytes() == (generated / entry.name).read_bytes(),
                      f"{entry.name}: deterministic rebuild differs")
    return count


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    module = Path(__file__).resolve().parents[1]
    parser.add_argument("--assets-root", type=Path, default=module / "assets/ui/scene")
    parser.add_argument("--skip-rebuild", action="store_true")
    args = parser.parse_args()
    try:
        count = verify(args.assets_root, module, not args.skip_rebuild)
    except (AssertionError, KeyError, ValueError, OSError, subprocess.SubprocessError) as error:
        raise SystemExit(f"Scene asset verification FAILED: {error}") from error
    print(f"Scene marker assets: {count} checks passed; SG-09 task backing confined to the existing diamond, solid outline/dots, chest/flag pixels and all v geometry preserved; no runtime/FPS claim.")


if __name__ == "__main__":
    main()
