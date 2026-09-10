#!/usr/bin/env python3
"""Build deterministic, regular-weight 2x F6 text for all eleven languages.

Pinned Droid CJK, Liberation Sans and Noto Sans Thai build inputs cover every
main label, popup endonym, tooltip, confirmation and numeric glyph. Fixed labels
share a 32-reference-unit base and their authored role scales, without artificial
bold outlines or language-dependent shrinking. Shared font-metric baselines align
the emitted glyphs in their native rows. The runtime receives pixels only; native
UMG owns interaction, scrolling, layout and scaling.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont
import f6_glass_assets as glass_assets


REFERENCE_WIDTH = 760
REFERENCE_HEIGHT = 876
SCALE = 2
TEXT_COLOR = (237, 244, 249, 255)
SECONDARY_TEXT_COLOR = (206, 220, 232, 255)
SECONDARY_SLOT_NAMES = ("language", "status", "radar", "map", "radar_only", "scene_distance")
SHADOW_COLOR = (0, 0, 0, 0)
STROKE_COLOR = (237, 244, 249, 128)
STROKE_WIDTH = 0
SHADOW_OFFSET = 0
HORIZONTAL_PADDING = 2
VERTICAL_PADDING = 1
MIN_FIT_SCALE = 1.0
FONT_SOURCE = "RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf"
EXPECTED_FONT_SHA256 = (
    "05D71B179EF97B82CF1BB91CEF290C600A510F77F39B4964359E3EF88378C79D"
)
LATIN_FONT_SOURCE = "tools/f6-fonts/LiberationSans-Regular.ttf"
EXPECTED_LATIN_FONT_SHA256 = (
    "F8ACE1F892B2BD9DC1792BA7F097FA7588F84FED48321480E04DE5390828221F"
)
LOCALIZED = glass_assets.read_main_texts(Path(__file__).resolve().parent.parent)
POPUP_LOCALIZED = {"auto": "Use game language", **{
    language: text["language_name"] for language, text in LOCALIZED.items()}}
LATIN_POPUP_LANGUAGES = ("auto", "en", "fr", "de", "es-es", "ru", "pt-br")
LANGUAGE_VALUE_FILES = {
    "fr": "fr-language-value.tga", "es-es": "es-language-value.tga",
}
LANGUAGE_VALUE_LAYOUT = ("language_value", 0, 0, 220, 26, 0.4375, True)


# name, x, y, width, height, native role scale, horizontally centered
# Coordinates are the 760x876 reference-space TextBlock rectangles in
# radar_visibility_hub.cpp.  The raster itself is emitted at SCALE=2.
MAIN_LAYOUT = (
    ("title", 24, 12, 610, 36, 0.6875, False),
    ("bug_report", 512, 831, 206, 24, 0.40625, True),
    ("close", 658, 17, 76, 24, 0.40625, True),
    ("language", 36, 59, 84, 24, 0.375, False),
    ("language_name", 126, 58, 220, 26, 0.4375, True),
    ("status", 388, 59, 90, 24, 0.375, False),
    ("status_value", 493, 59, 102, 24, 0.40625, True),
    ("status_action", 612, 59, 118, 24, 0.40625, True),
    ("marker_visibility", 36, 108, 380, 28, 0.50, False),
    ("radar", 488, 110, 100, 26, 0.40625, True),
    ("map", 614, 110, 100, 26, 0.40625, True),
    ("marker_all", 36, 144, 420, 24, 0.4375, False),
    ("category_0", 36, 312, 420, 24, 0.4375, False),
    ("category_1", 36, 168, 420, 24, 0.4375, False),
    ("category_2", 36, 192, 420, 24, 0.4375, False),
    ("category_3", 36, 216, 420, 24, 0.4375, False),
    ("category_4", 36, 240, 420, 24, 0.4375, False),
    ("category_5", 36, 264, 420, 24, 0.4375, False),
    ("category_6", 36, 288, 420, 24, 0.4375, False),
    ("scene_settings", 36, 364, 490, 28, 0.50, False),
    ("restore_defaults", 44, 831, 206, 24, 0.40625, True),
    ("scene_treasure", 46, 405, 202, 24, 0.40625, True),
    ("scene_area_quest", 280, 405, 202, 24, 0.40625, True),
    ("scene_mini_game", 514, 405, 202, 24, 0.40625, True),
    ("scene_range", 36, 438, 192, 26, 0.4375, False),
    ("scene_limit", 36, 470, 192, 26, 0.4375, False),
    ("scene_distance", 36, 504, 686, 18, 0.375, False),
    ("scene_distance_0", 40, 527, 158, 24, 0.40625, True),
    ("scene_distance_1", 214, 527, 158, 24, 0.40625, True),
    ("scene_distance_2", 388, 527, 158, 24, 0.40625, True),
    ("scene_distance_3", 562, 527, 158, 24, 0.40625, True),
    ("height_indicators", 36, 582, 500, 28, 0.50, False),
    ("radar_only", 580, 585, 142, 22, 0.375, True),
    ("height_category_0", 46, 618, 202, 24, 0.40625, True),
    ("height_category_1", 280, 618, 202, 24, 0.40625, True),
    ("height_category_2", 514, 618, 202, 24, 0.40625, True),
    ("height_category_3", 46, 652, 202, 24, 0.40625, True),
    ("height_category_4", 280, 652, 202, 24, 0.40625, True),
    ("filter_modes", 36, 708, 620, 28, 0.50, False),
    ("mode_label_0", 36, 742, 334, 22, 0.4375, False),
    ("available_0", 40, 773, 152, 24, 0.40625, True),
    ("all_0", 212, 773, 152, 24, 0.40625, True),
    ("mode_label_1", 390, 742, 334, 22, 0.4375, False),
    ("available_1", 394, 773, 152, 24, 0.40625, True),
    ("all_1", 566, 773, 152, 24, 0.40625, True),
)

# language, x, y, width, height, native role scale, horizontally centered
POPUP_LAYOUT = tuple(
    (language, 83 + (index % 3) * 202, 105 + (index // 3) * 40, 180, 24, 0.40625, True)
    for index, language in enumerate(("auto", *glass_assets.LANGUAGES))
)


def parse_args() -> argparse.Namespace:
    project_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--font",
        type=Path,
        default=project_root
        / ".sdk"
        / "RE-UE4SS"
        / "deps"
        / "fonts"
        / "droid"
        / "DroidSansFallback.ttf",
    )
    parser.add_argument(
        "--latin-font",
        type=Path,
        default=project_root / LATIN_FONT_SOURCE,
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=project_root / "assets" / "ui" / "f6",
    )
    return parser.parse_args()


def target_font_size(role_scale: float, slot_height: float) -> int:
    # Every language uses the same reference em and role multiplier.
    return max(1, round(32.0 * role_scale)) * SCALE


def render_glyph_run(
    font: ImageFont.FreeTypeFont,
    value: str,
    color: tuple[int, int, int, int] = TEXT_COLOR,
) -> Image.Image:
    left, top, right, bottom = font.getbbox(
        value, stroke_width=STROKE_WIDTH, anchor="ls"
    )
    margin = STROKE_WIDTH + SHADOW_OFFSET + 2
    image = Image.new(
        "RGBA",
        (
            max(1, right - left + margin * 2 + SHADOW_OFFSET),
            max(1, bottom - top + margin * 2 + SHADOW_OFFSET),
        ),
        (0, 0, 0, 0),
    )
    draw = ImageDraw.Draw(image)
    origin = (margin - left, margin - top)
    draw.text(
        (origin[0] + SHADOW_OFFSET, origin[1] + SHADOW_OFFSET),
        value,
        font=font,
        fill=SHADOW_COLOR,
        anchor="ls",
    )
    draw.text(
        origin,
        value,
        font=font,
        fill=color,
        stroke_width=STROKE_WIDTH,
        stroke_fill=STROKE_COLOR,
        anchor="ls",
    )
    bounds = image.getchannel("A").getbbox()
    if bounds is None:
        raise ValueError(f"text produced no visible pixels: {value!r}")
    run = image.crop(bounds)
    run.info["baseline_top"] = top + bounds[1] - margin
    return run


def slot_baseline(font: ImageFont.FreeTypeFont, slot_height: int) -> int:
    """One baseline per font, size and row height, independent of its wording."""
    ascent, descent = font.getmetrics()
    return (slot_height - ascent - descent) // 2 + ascent


def fitted_run(
    font_path: Path,
    value: str,
    width: float,
    height: float,
    role_scale: float,
    color: tuple[int, int, int, int] = TEXT_COLOR,
) -> tuple[Image.Image, int, int]:
    target_size = target_font_size(role_scale, height)
    size = target_size
    maximum_width = max(
        1, round((width - HORIZONTAL_PADDING * 2.0) * SCALE)
    )
    maximum_height = max(
        1, round((height - VERTICAL_PADDING * 2.0) * SCALE)
    )
    while size > 1:
        font = ImageFont.truetype(str(font_path), size)
        run = render_glyph_run(font, value, color)
        if run.width <= maximum_width and run.height <= maximum_height:
            if size / target_size < MIN_FIT_SCALE:
                raise ValueError(
                    f"text requires excessive font compression: {value!r} "
                    f"({size}/{target_size})"
                )
            return run, size, target_size
        size -= 1
    raise ValueError(f"text cannot fit its slot: {value!r}")


def draw_slot(
    image: Image.Image,
    font_path: Path,
    value: str,
    x: float,
    y: float,
    width: float,
    height: float,
    role_scale: float,
    center: bool = False,
    color: tuple[int, int, int, int] = TEXT_COLOR,
) -> None:
    run, size, _ = fitted_run(font_path, value, width, height, role_scale, color)
    font = ImageFont.truetype(str(font_path), size)
    slot_left = round(x * SCALE)
    slot_top = round(y * SCALE)
    slot_width = round(width * SCALE)
    slot_height = round(height * SCALE)
    px = (
        slot_left + (slot_width - run.width) // 2
        if center
        else slot_left + HORIZONTAL_PADDING * SCALE
    )
    py = slot_top + slot_baseline(font, slot_height) + run.info["baseline_top"]
    if (
        px < slot_left
        or py < slot_top
        or px + run.width > slot_left + slot_width
        or py + run.height > slot_top + slot_height
    ):
        raise ValueError(f"rendered text escaped its slot: {value!r}")
    image.alpha_composite(run, (px, py))


def new_canvas() -> Image.Image:
    return Image.new(
        "RGBA", (REFERENCE_WIDTH * SCALE, REFERENCE_HEIGHT * SCALE),
        (0, 0, 0, 0),
    )


def main_text_values(text: dict[str, object], status: str) -> dict[str, str]:
    categories = text["categories"]
    height_categories = text["height_categories"]
    status_values = text["status_values"]
    status_actions = text["status_actions"]
    if (
        not isinstance(categories, list)
        or not isinstance(height_categories, list)
        or not isinstance(status_values, dict)
        or not isinstance(status_actions, dict)
    ):
        raise ValueError("localized text has an invalid collection shape")
    values = {
        "title": text["title"],
        "bug_report": text["bug_report"],
        "close": text["close"],
        "language": text["language"],
        "language_name": text["language_name"],
        "status": text["status"],
        "status_value": status_values[status],
        "status_action": status_actions[status],
        "marker_visibility": text["marker_visibility"],
        "marker_all": text["all_markers"],
        "radar": text["radar"],
        "map": text["map"],
        "height_indicators": text["height_indicators"],
        "radar_only": text["radar_only"],
        "filter_modes": text["filter_modes"],
        "scene_settings": text["scene_settings"],
        "scene_range": text["scene_range"],
        "scene_limit": text["scene_limit"],
        "scene_distance": text["scene_distance"],
        "scene_distance_0": text["scene_distance_modes"][0],
        "scene_distance_1": text["scene_distance_modes"][1],
        "scene_distance_2": text["scene_distance_modes"][2],
        "scene_distance_3": text["scene_distance_modes"][3],
        "scene_treasure": categories[1],
        "scene_area_quest": categories[5],
        "scene_mini_game": categories[4],
        "restore_defaults": text["restore_defaults"],
    }
    values.update(
        {f"category_{index}": value for index, value in enumerate(categories)}
    )
    values.update(
        {
            f"height_category_{index}": value
            for index, value in enumerate(height_categories)
        }
    )
    for index, category_index in enumerate((5, 3)):
        values[f"mode_label_{index}"] = categories[category_index]
        values[f"available_{index}"] = text["available"]
        values[f"all_{index}"] = text["all"]
    if (
        set(values) != {entry[0] for entry in MAIN_LAYOUT}
        or not all(isinstance(value, str) and value for value in values.values())
    ):
        raise ValueError("localized text does not cover the complete main layout")
    return values


def build_main_overlay(font_path: Path, language: str, status: str) -> Image.Image:
    text = LOCALIZED[language]
    image = new_canvas()
    values = main_text_values(text, status)
    for name, x, y, width, height, role_scale, center in MAIN_LAYOUT:
        draw_slot(
            image, font_path, values[name], x, y, width, height,
            role_scale, center,
            SECONDARY_TEXT_COLOR if name in SECONDARY_SLOT_NAMES else TEXT_COLOR,
        )
    return image


def build_popup_overlay(font_path: Path, latin_font_path: Path) -> Image.Image:
    image = new_canvas()
    # Every endonym and AUTO is rasterized with its own pinned script font.
    for language, x, y, width, height, role_scale, center in POPUP_LAYOUT:
        draw_slot(
            image,
            language_font(language, font_path, latin_font_path),
            POPUP_LOCALIZED[language],
            x,
            y,
            width,
            height,
            role_scale,
            center,
        )
    return image


def build_language_value_overlay(latin_font_path: Path, language: str) -> Image.Image:
    _, x, y, width, height, role_scale, center = LANGUAGE_VALUE_LAYOUT
    image = Image.new("RGBA", (width * SCALE, height * SCALE), (0, 0, 0, 0))
    draw_slot(image, latin_font_path, POPUP_LOCALIZED[language],
              x, y, width, height, role_scale, center)
    return image


def write_tga(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="TGA", compression="tga_rle")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def canonical_sha256(value: Any) -> str:
    encoded = json.dumps(
        value, ensure_ascii=False, separators=(",", ":")
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest().upper()


def glyph_signature(font: ImageFont.FreeTypeFont, value: str) -> tuple[tuple[int, int], bytes]:
    mask = font.getmask(value)
    return mask.size, bytes(mask)


def language_font(language: str, cjk: Path, latin: Path) -> Path:
    thai = Path(__file__).resolve().parent.parent / glass_assets.THAI_FONT_SOURCE
    return glass_assets.tooltip_font(language, cjk, latin, thai)


def verify_glyph_coverage(font_path: Path, latin_font_path: Path) -> int:
    required = {}
    for language, text in LOCALIZED.items():
        required[language] = list(main_text_values(text, "off").values())
        required[language] += list(main_text_values(text, "on").values())
        required[language] += list(main_text_values(text, "fault").values())
        required[language].append(POPUP_LOCALIZED[language])
    required["en"].append(POPUP_LOCALIZED["auto"])
    return glass_assets.verify_tooltip_glyphs(required, font_path, latin_font_path,
        Path(__file__).resolve().parent.parent / glass_assets.THAI_FONT_SOURCE)


def main() -> int:
    args = parse_args()
    font_path = args.font.resolve()
    latin_font_path = args.latin_font.resolve()
    project_root = Path(__file__).resolve().parent.parent
    thai_font_path = project_root / glass_assets.THAI_FONT_SOURCE
    tooltips = glass_assets.read_tooltips(project_root)
    confirmations = glass_assets.read_confirmation_texts(project_root)
    output = args.output.resolve()
    if not font_path.is_file():
        raise SystemExit(f"font input not found: {font_path}")
    font_sha256 = sha256(font_path)
    if font_sha256 != EXPECTED_FONT_SHA256:
        raise SystemExit(
            "font input differs from the pinned DroidSansFallback payload: "
            f"{font_sha256}"
        )
    try:
        if not latin_font_path.is_file() or sha256(latin_font_path) != EXPECTED_LATIN_FONT_SHA256:
            raise ValueError("Latin font input is missing or differs from its pin")
        glyph_count = verify_glyph_coverage(font_path, latin_font_path)
        tooltip_glyph_count = glass_assets.verify_tooltip_glyphs(
            tooltips, font_path, latin_font_path, thai_font_path)
        confirmation_glyph_count = glass_assets.verify_tooltip_glyphs(
            confirmations, font_path, latin_font_path, thai_font_path)
    except ValueError as error:
        raise SystemExit(str(error)) from error

    generated: list[dict[str, object]] = []
    for language in glass_assets.LANGUAGES:
        for status in ("off", "on", "fault"):
            path = output / f"{language}-{status}.tga"
            write_tga(build_main_overlay(language_font(language, font_path, latin_font_path), language, status), path)
            generated.append(
                {
                    "path": path.name,
                    "width": REFERENCE_WIDTH * SCALE,
                    "height": REFERENCE_HEIGHT * SCALE,
                    "sha256": sha256(path),
                }
            )

    popup = output / "language-popup.tga"
    write_tga(build_popup_overlay(font_path, latin_font_path), popup)
    generated.append(
        {
            "path": popup.name,
            "width": REFERENCE_WIDTH * SCALE,
            "height": REFERENCE_HEIGHT * SCALE,
            "sha256": sha256(popup),
        }
    )
    for language, filename in LANGUAGE_VALUE_FILES.items():
        path = output / filename
        value_image = build_language_value_overlay(latin_font_path, language)
        write_tga(value_image, path)
        generated.append({"path": path.name, "width": value_image.width,
                          "height": value_image.height, "sha256": sha256(path)})
    tooltip_metrics = {}
    confirmation_metrics = {}
    numeric_metrics = {}
    for language, values in tooltips.items():
        image, metrics, confirm_metrics = glass_assets.build_tooltip_atlas(values,
            glass_assets.tooltip_font(language, font_path, latin_font_path, thai_font_path), confirmations[language])
        path = output / f"tooltip-{language}.tga"
        write_tga(image, path)
        generated.append({"path": path.name, "width": image.width,
                          "height": image.height, "sha256": sha256(path)})
        tooltip_metrics[language] = metrics
        confirmation_metrics[language] = confirm_metrics
        numeric_metrics[language] = glass_assets.build_numeric_tiles(
            language_font(language, font_path, latin_font_path))[1]
    for filename, image in glass_assets.build_skins().items():
        path = output / filename
        write_tga(image, path)
        generated.append({"path": path.name, "width": image.width,
                          "height": image.height, "sha256": sha256(path)})
    manifest = {
        "schema_version": 3,
        "reference_size": [REFERENCE_WIDTH, REFERENCE_HEIGHT],
        "raster_scale": SCALE,
        "generator_sha256": sha256(Path(__file__).resolve()),
        "layout_sha256": canonical_sha256(
            {"main": MAIN_LAYOUT, "popup": POPUP_LAYOUT,
             "language_value": LANGUAGE_VALUE_LAYOUT}
        ),
        "localizations_sha256": canonical_sha256(LOCALIZED),
        "font_source": FONT_SOURCE,
        "font_sha256": font_sha256,
        "latin_font_source": LATIN_FONT_SOURCE,
        "latin_font_sha256": EXPECTED_LATIN_FONT_SHA256,
        "popup_localizations_sha256": canonical_sha256(POPUP_LOCALIZED),
        "verified_codepoint_count": glyph_count,
        "glass_generator_sha256": sha256(Path(glass_assets.__file__)),
        "thai_font_source": glass_assets.THAI_FONT_SOURCE,
        "thai_font_sha256": glass_assets.THAI_FONT_SHA256,
        "tooltip_ids": glass_assets.TOOLTIP_IDS,
        "tooltip_localizations_sha256": canonical_sha256(tooltips),
        "tooltip_verified_codepoint_count": tooltip_glyph_count,
        "tooltip_reference_size": [320, 72],
        "tooltip_metrics": tooltip_metrics,
        "confirmation_ids": glass_assets.CONFIRMATION_IDS,
        "confirmation_crops": glass_assets.CONFIRMATION_CROPS,
        "confirmation_localizations_sha256": canonical_sha256(confirmations),
        "confirmation_verified_codepoint_count": confirmation_glyph_count,
        "confirmation_metrics": confirmation_metrics,
        "atlas_tile_count": 55,
        "numeric_characters": glass_assets.NUMERIC_CHARACTERS,
        "numeric_crop": glass_assets.NUMERIC_CROP,
        "numeric_advances": glass_assets.NUMERIC_ADVANCES,
        "numeric_metrics": numeric_metrics,
        "main_font_reference_size": 32,
        "main_font_weight": "regular",
        "main_font_stroke": 0,
        "main_text_alignment": "font-metric-baseline",
        "main_text_color": TEXT_COLOR,
        "main_secondary_text_color": SECONDARY_TEXT_COLOR,
        "main_secondary_slots": SECONDARY_SLOT_NAMES,
        "main_languages": glass_assets.LANGUAGES,
        "files": generated,
    }
    manifest_path = output / "manifest.json"
    with manifest_path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    print(f"generated {len(generated)} overlays in {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
