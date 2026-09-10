#!/usr/bin/env python3
"""Build the localized F6 icon guide as eleven fixed-size, two-times atlases."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

import f6_glass_assets as shared
import f6_guide_icons as icons


LANGUAGES = ("en", "ja", "ko", "zh-hans", "zh-hant", "fr", "de", "es-es", "ru", "th", "pt-br")
SCALE = 2
REFERENCE_SIZE = (760, 1184)
BODY_CROP = (0, 64, 760, 1120)
BUTTON_CROPS = {"guide_button": (0, 0, 92, 24), "settings_button": (0, 24, 92, 24)}
PRIMARY = (237, 244, 249, 255)
SECONDARY = (206, 220, 232, 255)
BODY_BACKGROUND = (31, 43, 57, 235)
FONT_INPUTS = {
    "cjk": (".sdk/RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf", "05D71B179EF97B82CF1BB91CEF290C600A510F77F39B4964359E3EF88378C79D"),
    "latin": ("tools/f6-fonts/LiberationSans-Regular.ttf", "F8ACE1F892B2BD9DC1792BA7F097FA7588F84FED48321480E04DE5390828221F"),
    "thai": ("tools/f6-fonts/NotoSansThai-Variable.ttf", "5A1C559BB539583C8A1FD99D1C5B9491E5E14478C9CD2BD0970D5C3096CC9EF8"),
}
SOURCE_HEADER = "include/dswros/radar_guide_localization.hpp"
FIELD_NAMES = (
    "guide_button", "settings_button", "title", "treasure_title",
    *(f"treasure_{i}" for i in range(4)),
    "radar_title", "radar_column", "map_column", *(f"radar_{i}" for i in range(7)),
    "clock_body", "height_title", *(f"height_state_{i}" for i in range(4)),
    *(f"height_label_{i}" for i in range(5)), "height_body",
    "scene_title", *(f"scene_{i}" for i in range(3)), "distance_title",
    *(f"distance_label_{i}" for i in range(4)),
    *(f"distance_description_{i}" for i in range(4)), "scene_pointer_body",
)
TREASURE_KINDS = ("treasure_common", "treasure_green", "treasure_gold", "treasure_blue")
RADAR_KINDS = ("fly", "mole", "wave", "boss", "assault", "area", "egg")
# Display order is separate from the localization/source icon ordering.
RADAR_DISPLAY_ORDER = (0, 4, 2, 3, 1, 5, 6)
SCENE_KINDS = ("scene_treasure", "scene_area", "scene_mini_game")
HEIGHT_KINDS = ("treasure", "mini_game", "area", "boss", "assault")
# Keep the localization source intact; the guide only documents these three states.
HEIGHT_STATES = ("above", "level", "below")
HEIGHT_COLUMN_WIDTH = 168


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def canonical_sha256(value) -> str:
    return hashlib.sha256(json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode("utf-8")).hexdigest().upper()


def read_localizations(root: Path) -> dict[str, dict[str, str]]:
    source = (root / SOURCE_HEADER).read_text(encoding="utf-8")
    values = [json.loads('"' + value + '"') for value in re.findall(r'L"((?:[^"\\]|\\.)*)"', source)]
    if len(values) != len(LANGUAGES) * len(FIELD_NAMES) or not all(value.strip() for value in values):
        raise ValueError("guide requires eleven complete 44-string localization records")
    return {language: dict(zip(FIELD_NAMES, values[i * len(FIELD_NAMES):(i + 1) * len(FIELD_NAMES)]))
            for i, language in enumerate(LANGUAGES)}


def font_path(root: Path, language: str) -> Path:
    role = "thai" if language == "th" else "cjk" if language in ("ja", "ko", "zh-hans", "zh-hant") else "latin"
    return root / FONT_INPUTS[role][0]


def verify_fonts(root: Path, localizations: dict[str, dict[str, str]]) -> int:
    for source, expected in FONT_INPUTS.values():
        path = root / source
        if not path.is_file() or sha256(path) != expected:
            raise ValueError(f"guide font differs from its pinned regular input: {source}")
    required = set()
    for language, fields in localizations.items():
        font = ImageFont.truetype(str(font_path(root, language)), 64)
        absent = font.getmask("\U0010FFFF")
        absent_signature = (absent.size, bytes(absent))
        characters = {character for value in fields.values() for character in value if not character.isspace()}
        for character in characters:
            mask = font.getmask(character)
            if not mask.getbbox() or (mask.size, bytes(mask)) == absent_signature:
                raise ValueError(f"guide/{language} lacks U+{ord(character):04X}")
        required.update(characters)
    return len(required)


def text_layout() -> list[tuple]:
    """Body coordinates are independent of the menu's scroll position."""
    rows = [
        ("title", "title", 36, 12, 688, 36, 22, False, False),
        ("treasure_title", "treasure_title", 36, 54, 688, 28, 16, False, False),
        ("radar_title", "radar_title", 36, 190, 688, 28, 16, False, False),
        ("clock_body", "clock_body", 182, 414, 542, 42, 14, False, True),
        ("height_title", "height_title", 36, 466, 688, 28, 16, False, False),
        ("height_body", "height_body", 36, 740, 688, 42, 14, False, True),
        ("scene_title", "scene_title", 36, 790, 688, 28, 16, False, False),
        ("scene_pointer_body", "scene_pointer_body", 36, 866, 688, 36, 14, False, True),
        ("distance_title", "distance_title", 36, 910, 688, 28, 16, False, False),
    ]
    for section, y in (("treasure", 82), ("radar", 218)):
        for column in range(2):
            x = 36 + column * 360
            rows.extend([
                (f"{section}_radar_{column}", "radar_column", x - 8, y, 56, 24, 12, True, True),
                (f"{section}_map_{column}", "map_column", x + 38, y, 56, 24, 12, True, True),
            ])
    for index in range(4):
        x = 36 + (index % 2) * 360
        rows.append((f"treasure_{index}", f"treasure_{index}", x + 90, 104 + (index // 2) * 42, 238, 40, 14, False, False))
    for slot, index in enumerate(RADAR_DISPLAY_ORDER):
        x = 36 + (slot % 2) * 360
        rows.append((f"radar_{index}", f"radar_{index}", x + 90, 242 + (slot // 2) * 42, 238, 40, 14, False, False))
    for index in range(len(HEIGHT_STATES)):
        rows.append((f"height_state_{index}", f"height_state_{index}", 220 + index * HEIGHT_COLUMN_WIDTH, 496, HEIGHT_COLUMN_WIDTH, 28, 12, True, True))
    for index in range(5):
        rows.append((f"height_label_{index}", f"height_label_{index}", 36, 526 + index * 42, 176, 40, 14, False, False))
    for index in range(3):
        # Align text to the icon body, excluding its lower v-shaped pointer.
        rows.append((f"scene_{index}", f"scene_{index}", 84 + index * 234, 816, 172, 40, 14, False, False))
    for index in range(4):
        y = 942 + index * 40
        rows.extend([
            (f"distance_label_{index}", f"distance_label_{index}", 36, y, 160, 40, 14, False, False),
            (f"distance_description_{index}", f"distance_description_{index}", 204, y, 520, 40, 14, False, True),
        ])
    return rows


def icon_layout() -> list[tuple[str, tuple[int, int, int, int]]]:
    result = []
    for index, kind in enumerate(TREASURE_KINDS):
        x, y = 36 + (index % 2) * 360, 104 + (index // 2) * 42
        result.extend([(kind, (x, y, x + 40, y + 40)),
                       ("map_" + kind, (x + 46, y, x + 86, y + 40))])
    for slot, index in enumerate(RADAR_DISPLAY_ORDER):
        kind = RADAR_KINDS[index]
        x, y = 36 + (slot % 2) * 360, 242 + (slot // 2) * 42
        result.append((kind, (x, y, x + 40, y + 40)))
        if kind != "egg":
            result.append(("map_" + kind, (x + 46, y, x + 86, y + 40)))
    result.append(("clock_1638", (36, 416, 170, 454)))
    for row, kind in enumerate(HEIGHT_KINDS):
        for column, state in enumerate(HEIGHT_STATES):
            x, y = 231 + column * HEIGHT_COLUMN_WIDTH, 526 + row * 42
            result.append((f"height_{kind}_{state}", (x, y, x + HEIGHT_COLUMN_WIDTH - 22, y + 40)))
    for index, kind in enumerate(SCENE_KINDS):
        x = 36 + index * 234
        result.append((kind, (x, 820, x + 40, 860)))
    return result


def paint_text(image: Image.Image, path: Path, value: str, rectangle: tuple[int, int, int, int],
               reference_font_size: int, center: bool, secondary: bool, wrap: bool = True,
               top_aligned: bool = False) -> dict:
    x, y, width, height = (value * SCALE for value in rectangle)
    font = ImageFont.truetype(str(path), reference_font_size * SCALE)
    paragraphs = value.split("\n")
    lines = [line for paragraph in paragraphs
             for line in (shared.wrap_text(paragraph, font, width - 8) if wrap else [paragraph])]
    if not lines or any(not line for line in lines):
        raise ValueError(f"empty guide line: {value!r}")
    ascent, descent = font.getmetrics()
    line_height = (reference_font_size + 6) * SCALE
    total_height = ascent + descent + (len(lines) - 1) * line_height
    baseline = 2 + ascent if top_aligned else (height - total_height) // 2 + ascent
    tile = Image.new("RGBA", (width, height))
    bounds = []
    for row, line in enumerate(lines):
        left, top, right, bottom = font.getbbox(line, anchor="ls")
        run = Image.new("RGBA", (max(1, right - left + 8), max(1, bottom - top + 8)))
        ImageDraw.Draw(run).text((4 - left, 4 - top), line, font=font, anchor="ls",
                                 fill=SECONDARY if secondary else PRIMARY)
        actual = run.getchannel("A").getbbox()
        if actual is None:
            raise ValueError(f"empty guide glyph run: {line!r}")
        glyphs = run.crop(actual)
        px = (width - glyphs.width) // 2 if center else 4
        py = baseline + row * line_height + top + actual[1] - 4
        if px < 2 or py < 2 or px + glyphs.width > width - 2 or py + glyphs.height > height - 2:
            raise ValueError(f"guide text cannot fit its {rectangle[2]}x{rectangle[3]} cell at {reference_font_size}px: {value!r}")
        tile.alpha_composite(glyphs, (px, py))
        bounds.append([px, py, px + glyphs.width, py + glyphs.height])
    image.alpha_composite(tile, (x, y))
    return {"rectangle": list(rectangle), "font_pixels": reference_font_size * SCALE,
            "lines": lines, "bounds": bounds, "centered": center, "secondary": secondary,
            "top_aligned": top_aligned,
            "text_sha256": hashlib.sha256(value.encode("utf-8")).hexdigest().upper()}


def render_language(root: Path, language: str, values: dict[str, str]) -> tuple[Image.Image, dict]:
    image = Image.new("RGBA", tuple(value * SCALE for value in REFERENCE_SIZE))
    ImageDraw.Draw(image).rounded_rectangle((18 * SCALE, 64 * SCALE, 742 * SCALE - 1, 1184 * SCALE - 1),
                                            radius=20 * SCALE, fill=BODY_BACKGROUND)
    path = font_path(root, language)
    text_metrics = {}
    for field, rectangle in BUTTON_CROPS.items():
        text_metrics[field] = paint_text(image, path, values[field], rectangle, 13, True, False, False)
    for identifier, field, x, y, width, height, size, center, secondary in text_layout():
        top_aligned = False
        text_metrics[identifier] = paint_text(image, path, values[field], (x, y + 64, width, height), size, center, secondary,
                                               top_aligned=top_aligned)
    icon_metrics = []
    for kind, rectangle in icon_layout():
        left, top, right, bottom = rectangle
        pixel_rect = (left * SCALE, (top + 64) * SCALE, right * SCALE, (bottom + 64) * SCALE)
        if kind.startswith("height_"):
            ImageDraw.Draw(image).rounded_rectangle(pixel_rect, radius=8 * SCALE, fill=(125, 151, 170, 255))
        icons.draw_legend_icon(image, kind, pixel_rect)
        icon_metrics.append({"kind": kind, "rectangle": [left, top + 64, right - left, bottom - top]})
    # Bird eggs are radar-only; an explicit dash is clearer than an empty map cell.
    dash = (102 * SCALE, (388 + 64) * SCALE)
    ImageDraw.Draw(image).line((dash[0] - 5 * SCALE, dash[1], dash[0] + 5 * SCALE, dash[1]), fill=SECONDARY, width=2)
    return image, {"texts": text_metrics, "icons": icon_metrics, "map_egg_dash": [dash[0] - 10, dash[1], dash[0] + 11, dash[1] + 2]}


def build(root: Path, output: Path) -> dict:
    root, output = root.resolve(), output.resolve()
    localizations = read_localizations(root)
    glyph_count = verify_fonts(root, localizations)
    output.mkdir(parents=True, exist_ok=True)
    allowed = {language + ".tga" for language in LANGUAGES} | {"manifest.json"}
    if any(path.name not in allowed or not path.is_file() or path.is_symlink() for path in output.iterdir()):
        raise ValueError("guide output contains unexpected files; refusing to overwrite it")
    files = []
    for language, values in localizations.items():
        image, metrics = render_language(root, language, values)
        path = output / (language + ".tga")
        image.save(path, format="TGA", compression="tga_rle")
        files.append({"path": path.name, "width": image.width, "height": image.height,
                      "sha256": sha256(path), "metrics": metrics})
    sources = {filename: sha256(root / filename) for filename in
               (SOURCE_HEADER, "tools/f6_guide_assets.py", "tools/f6_guide_icons.py", "tools/f6_glass_assets.py")}
    manifest = {"schema_version": 1, "reference_size": list(REFERENCE_SIZE), "raster_scale": SCALE,
                "body_crop": list(BODY_CROP), "button_crops": BUTTON_CROPS, "languages": LANGUAGES,
                "font_inputs": FONT_INPUTS, "text_alignment": "font-metric-baseline", "font_weight": "regular",
                "font_fit_scale": 1.0, "primary_text_color": PRIMARY, "secondary_text_color": SECONDARY,
                "body_background": BODY_BACKGROUND,
                "icon_provenance": icons.verify_sources(),
                "source_sha256": sources, "localizations_sha256": canonical_sha256(localizations),
                "layout_sha256": canonical_sha256({"texts": text_layout(), "icons": icon_layout()}),
                "verified_codepoints": glyph_count, "files": files}
    (output / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return manifest


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=Path, default=root)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    manifest = build(args.project_root, args.output or args.project_root / "assets/ui/guide")
    print(f"F6 guide generated: {len(manifest['files'])} localized atlases; {manifest['verified_codepoints']} verified codepoints")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
