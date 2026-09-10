#!/usr/bin/env python3
"""Validate guide inventory, complete text, safe crops, source icons and native atlas bounds."""
from __future__ import annotations

import argparse
import json
import re
import struct
from pathlib import Path

from PIL import Image

import f6_guide_assets as guide
import f6_guide_icons as icons


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def normalized(value: str) -> str:
    return re.sub(r"\s+", "", value)


def verify_native(root: Path) -> int:
    hub = re.sub(r"\s+", " ", (root / "src/native/radar_visibility_hub.cpp").read_text(encoding="utf-8"))
    for snippet in (
        "constexpr double kGuideContentHeight = 1120.0;",
        "constexpr double kGuideAtlasHeight = 1184.0;",
        "width == 1520U && height == 2368U",
        "add_to_canvas(body_stack, guide, 0, 0, kReferencePanelWidth, kGuideContentHeight, 1)",
        "add_to_canvas(header, parent, 550, 17, 92, 24, 23)",
        "const double atlas_y = index < 2U ? -24.0 * static_cast<double>(index) : -64.0;",
        "add_to_canvas(parent, image, 0, atlas_y, kReferencePanelWidth, kGuideAtlasHeight, 0)",
    ):
        require(snippet in hub, f"guide/native atlas contract changed: {snippet}")
    header = (root / guide.SOURCE_HEADER).read_text(encoding="utf-8")
    require("kRadarGuideStringCount = 44;" in header, "guide native field count differs")
    enum = re.search(r"enum class RadarGuideRadarKind[^\{]*\{([^}]+)\}", header)
    require(enum is not None and tuple(value.strip() for value in enum[1].split(",") if value.strip())
            == ("Fly", "Mole", "Wave", "Boss", "Assault", "AreaQuest", "BirdEgg", "Count"),
            "guide icon labels differ from their native ordering")
    # The title keeps its original broad TextBlock slot. Its actual rendered
    # ink must still clear the new Guide button that begins at x=550.
    checked = 0
    for language in guide.LANGUAGES:
        for status in ("off", "on", "fault"):
            path = root / "assets/ui/f6" / f"{language}-{status}.tga"
            require(path.is_file(), f"missing main text atlas for header clearance: {path.name}")
            alpha = Image.open(path).convert("RGBA").getchannel("A")
            title = alpha.crop((24 * 2, 12 * 2, 634 * 2, 48 * 2)).getbbox()
            require(title is not None and title[2] + 24 * 2 <= 548 * 2,
                    f"{path.name} title ink overlaps the Guide button")
            checked += 1
    return checked


def verify_raster(path: Path) -> Image.Image:
    data = path.read_bytes()
    require(len(data) >= 44, f"guide TGA is truncated: {path.name}")
    require(data[2] == 10 and data[16] == 32 and data[17] & 15 == 8,
            f"guide requires RLE RGBA TGA: {path.name}")
    require(struct.unpack_from("<HH", data, 12) == (1520, 2368),
            f"guide raster dimensions differ: {path.name}")
    image = Image.open(path).convert("RGBA")
    require(image.size == (1520, 2368), f"decoded guide dimensions differ: {path.name}")
    require(image.crop((0, 96, 1520, 128)).getchannel("A").getbbox() is None,
            "guide button tiles bleed into the atlas separator")
    require(image.crop((0, 128, 36, 2368)).getchannel("A").getbbox() is None
            and image.crop((1484, 128, 1520, 2368)).getchannel("A").getbbox() is None,
            "guide artwork escaped its body margins")
    return image


def verify_text_metrics(values: dict[str, str], metrics: dict) -> int:
    layouts = {name: (name, *rectangle, 13, True, False)
               for name, rectangle in guide.BUTTON_CROPS.items()}
    layouts.update({identifier: (field, x, y + 64, width, height, size, center, secondary)
                    for identifier, field, x, y, width, height, size, center, secondary in guide.text_layout()})
    require(set(metrics) == set(layouts), "guide text cell inventory changed")
    for name, (field, x, y, width, height, size, center, secondary) in layouts.items():
        record = metrics[name]
        require(record["rectangle"] == [x, y, width, height] and record["font_pixels"] == size * 2,
                f"guide/{name} changed crop or shrank its type")
        require(record["centered"] == center and record["secondary"] == secondary,
                f"guide/{name} text role differs")
        expected_top_alignment = False
        require(record.get("top_aligned") == expected_top_alignment, f"guide/{name} paragraph baseline differs")
        require(normalized("".join(record["lines"])) == normalized(values[field]),
                f"guide/{name} omitted or changed text while wrapping")
        require(len(record["lines"]) == len(record["bounds"]) and bool(record["lines"]),
                f"guide/{name} line metrics incomplete")
        for left, top, right, bottom in record["bounds"]:
            require(2 <= left < right <= width * 2 - 2 and 2 <= top < bottom <= height * 2 - 2,
                    f"guide/{name} touches or crosses its crop edge")
        require(x >= 0 and y >= 0 and x + width <= 760 and y + height <= 1184,
                f"guide/{name} lies outside the atlas")
    return len(layouts)


def verify_contrast() -> float:
    def linear(value: int) -> float:
        value /= 255
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4
    weights = (0.2126, 0.7152, 0.0722)
    opacity = guide.BODY_BACKGROUND[3] / 255
    background = sum(weight * (linear(channel) * opacity + 1 - opacity)
                     for weight, channel in zip(weights, guide.BODY_BACKGROUND[:3]))
    contrasts = [(sum(weight * linear(channel) for weight, channel in zip(weights, color[:3])) + .05)
                 / (background + .05) for color in (guide.PRIMARY, guide.SECONDARY)]
    require(min(contrasts) >= 4.5, "guide small text falls below 4.5:1 on a linear-light white scene")
    return min(contrasts)


def verify_guide_content(values: dict[str, str], metrics: dict) -> int:
    layout = {row[0]: row for row in guide.text_layout()}
    # Four meanings without redundant color-name lines; each distance mode
    # owns a complete row with a separately readable label and description.
    require(all("\n" not in values[f"treasure_{index}"] for index in range(4)),
            "treasure descriptions reintroduced stacked color-name headings")
    require(guide.HEIGHT_STATES == ("above", "level", "below")
            and "height_state_3" not in metrics["texts"],
            "guide must only show above, near-level and below height columns")
    for index in range(4):
        label, body = layout[f"distance_label_{index}"], layout[f"distance_description_{index}"]
        require(label[3] == body[3] == 942 + index * 40 and label[6] == body[6] == 14,
                "distance modes must remain four full-size independent rows")
    expected_order = (0, 4, 2, 3, 1, 5, 6)
    require(guide.RADAR_DISPLAY_ORDER == expected_order,
            "guide must group flying, wave and marmot on the left, missions on the right")
    icon_cells = {record["kind"]: record["rectangle"] for record in metrics["icons"]}
    for slot, index in enumerate(expected_order):
        row = layout[f"radar_{index}"]
        x, y = 36 + slot % 2 * 360, 242 + slot // 2 * 42
        require(row[1] == f"radar_{index}" and row[2] == x + 90 and row[3] == y,
                "activity guide lost its two-column entry layout")
        kind = guide.RADAR_KINDS[index]
        require(icon_cells[kind] == [x, y + 64, 40, 40],
                "activity guide icon moved without its label")
        if kind != "egg":
            require(icon_cells["map_" + kind] == [x + 46, y + 64, 40, 40],
                    "activity guide map icon moved without its radar icon and label")
    expected_states = {f"height_{kind}_{state}" for kind in guide.HEIGHT_KINDS for state in guide.HEIGHT_STATES}
    actual_states = {record["kind"] for record in metrics["icons"] if record["kind"].startswith("height_")}
    require(actual_states == expected_states and len(actual_states) == 15,
            "guide must show three height presentations for each of five categories")
    require(any(record["kind"] == "clock_1638" for record in metrics["icons"]),
            "guide lost the live seven-segment game-clock example")
    boxes = []
    for name, record in metrics["texts"].items():
        x, y, _, _ = record["rectangle"]
        if name in guide.BUTTON_CROPS:
            continue
        for left, top, right, bottom in record["bounds"]:
            boxes.append((name, (x*2+left, y*2+top, x*2+right, y*2+bottom)))
    for index, (name, box) in enumerate(boxes):
        for other_name, other in boxes[index+1:]:
            require(box[2] <= other[0] or other[2] <= box[0] or box[3] <= other[1] or other[3] <= box[1],
                    f"guide text overlaps: {name} / {other_name}")
        for icon in metrics["icons"]:
            x, y, width, height = icon["rectangle"]
            require(box[2] <= x*2 or (x+width)*2 <= box[0] or box[3] <= y*2 or (y+height)*2 <= box[1],
                    f"guide text crosses an icon cell: {name} / {icon['kind']}")
    return len(boxes)


def verify(root: Path, asset_root: Path) -> dict:
    root, asset_root = root.resolve(), asset_root.resolve()
    require(asset_root.is_dir(), "guide assets directory is missing")
    expected_names = {language + ".tga" for language in guide.LANGUAGES} | {"manifest.json"}
    require({path.name for path in asset_root.iterdir()} == expected_names,
            "guide assets must contain exactly eleven atlases and one manifest")
    require(all(path.is_file() and not path.is_symlink() for path in asset_root.iterdir()),
            "guide assets contain directories or symbolic links")
    manifest = json.loads((asset_root / "manifest.json").read_text(encoding="utf-8"))
    localizations = guide.read_localizations(root)
    codepoints = guide.verify_fonts(root, localizations)
    expected = {
        "schema_version": 1, "reference_size": [760, 1184], "raster_scale": 2,
        "body_crop": [0, 64, 760, 1120],
        "button_crops": {"guide_button": [0, 0, 92, 24], "settings_button": [0, 24, 92, 24]},
        "languages": list(guide.LANGUAGES), "text_alignment": "font-metric-baseline",
        "font_weight": "regular", "font_fit_scale": 1.0,
        "primary_text_color": [237, 244, 249, 255], "secondary_text_color": [206, 220, 232, 255],
        "body_background": [31, 43, 57, 235], "verified_codepoints": codepoints,
        "localizations_sha256": guide.canonical_sha256(localizations),
        "layout_sha256": guide.canonical_sha256({"texts": guide.text_layout(), "icons": guide.icon_layout()}),
        "icon_provenance": icons.verify_sources(),
    }
    for key, value in expected.items():
        require(manifest.get(key) == value, f"guide manifest {key} is stale or invalid")
    require(guide.canonical_sha256(manifest.get("font_inputs")) == guide.canonical_sha256(guide.FONT_INPUTS),
            "guide font inputs differ from the regular-weight pins")
    source_names = (guide.SOURCE_HEADER, "tools/f6_guide_assets.py", "tools/f6_guide_icons.py", "tools/f6_glass_assets.py")
    require(manifest.get("source_sha256") == {name: guide.sha256(root / name) for name in source_names},
            "guide sources changed after generation")
    files = manifest.get("files", [])
    require(len(files) == 11 and [entry["path"] for entry in files] == [language + ".tga" for language in guide.LANGUAGES],
            "guide manifest language inventory differs")
    text_cells = 0
    for language, record in zip(guide.LANGUAGES, files):
        path = asset_root / record["path"]
        require(record["width"] == 1520 and record["height"] == 2368 and record["sha256"] == guide.sha256(path),
                f"guide asset identity differs: {path.name}")
        image = verify_raster(path)
        # Repainting at the pinned font sizes catches stale or truncated
        # payloads; explicit metric checks retain source text and crop safety.
        rendered, metrics = guide.render_language(root, language, localizations[language])
        require(image.tobytes() == rendered.tobytes(), f"guide pixels are stale: {path.name}")
        require(record["metrics"] == metrics, f"guide text or icon metrics are stale: {path.name}")
        text_cells += verify_text_metrics(localizations[language], metrics["texts"])
        verify_guide_content(localizations[language], metrics)
        require(len(metrics["icons"]) == 40, "guide icon inventory lost a radar/map, height, clock or scene example")
    return {"languages": 11, "text_cells": text_cells, "codepoints": codepoints,
            "icon_examples": 40, "header_clearance_checks": verify_native(root),
            "minimum_linear_white_contrast": verify_contrast()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--asset-root", type=Path)
    args = parser.parse_args()
    result = verify(args.project_root, args.asset_root or args.project_root / "assets/ui/guide")
    print("F6 guide verification passed: " + json.dumps(result, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
