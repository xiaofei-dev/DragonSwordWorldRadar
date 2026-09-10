#!/usr/bin/env python3
"""Verify F6 raster geometry, source text and multilingual popup glyph coverage."""

from __future__ import annotations

import argparse
import ast
import hashlib
import importlib.util
import json
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont
import f6_glass_verify as glass_verify


EXPECTED_FONT_SHA256 = (
    "05D71B179EF97B82CF1BB91CEF290C600A510F77F39B4964359E3EF88378C79D"
)
EXPECTED_LATIN_FONT_SOURCE = "tools/f6-fonts/LiberationSans-Regular.ttf"
EXPECTED_LATIN_FONT_SHA256 = (
    "F8ACE1F892B2BD9DC1792BA7F097FA7588F84FED48321480E04DE5390828221F"
)
EXPECTED_POPUP_LOCALIZED = {
    "auto": "Use game language", "en": "English", "ja": "日本語", "ko": "한국어",
    "zh-hans": "简体中文", "zh-hant": "繁體中文", "fr": "Français",
    "de": "Deutsch", "es-es": "Español (España)", "ru": "Русский",
    "th": "ไทย", "pt-br": "Português (Brasil)",
}
LATIN_POPUP_LANGUAGES = ("auto", "en", "fr", "de", "es-es", "ru", "pt-br")
LANGUAGE_VALUE_FILES = {
    "fr": "fr-language-value.tga", "es-es": "es-language-value.tga",
}
LANGUAGE_VALUE_LAYOUT = ("language_value", 0, 0, 220, 26, 0.4375, True)
EXPECTED_FILES = {
    f"{language}-{status}.tga"
    for language in glass_verify.EXPECTED_LANGUAGES
    for status in ("off", "on", "fault")
} | {"language-popup.tga", "fr-language-value.tga", "es-language-value.tga"}
TEXT_FILES = EXPECTED_FILES.copy()
EXPECTED_FILES.update(glass_verify.EXPECTED_SIZES)
EXPECTED_WIDTH = 1520
EXPECTED_HEIGHT = 1752
TGA_FOOTER = b"\x00" * 8 + b"TRUEVISION-XFILE.\x00"
MAX_CENTER_ERROR = 0.5
MIN_SLOT_INK_HEIGHT_RATIO = 0.28
MAX_SLOT_INK_HEIGHT_RATIO = 0.80
TEXT_COLOR = (237, 244, 249, 255)
SECONDARY_TEXT_COLOR = (206, 220, 232, 255)
SECONDARY_SLOT_NAMES = ("language", "status", "radar", "map", "radar_only", "scene_distance")

Slot = tuple[str, int, int, int, int]

EXPECTED_MAIN_LAYOUT = (
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

EXPECTED_POPUP_LAYOUT = tuple(
    (language, 83 + index % 3 * 202, 105 + index // 3 * 40, 180, 24, 0.40625, True)
    for index, language in enumerate(EXPECTED_POPUP_LOCALIZED)
)


MAIN_OVERLAY_SLOTS: tuple[Slot, ...] = tuple(
    entry[:5] for entry in EXPECTED_MAIN_LAYOUT
)

POPUP_OVERLAY_SLOTS: tuple[Slot, ...] = tuple(
    entry[:5] for entry in EXPECTED_POPUP_LAYOUT
)

CENTERED_SLOT_NAMES = {
    entry[0]
    for entry in (*EXPECTED_MAIN_LAYOUT, *EXPECTED_POPUP_LAYOUT)
    if entry[6]
}
CENTERED_SLOT_NAMES.add(LANGUAGE_VALUE_LAYOUT[0])

LANGUAGE_CODES = (
    "en", "ja", "ko", "zh-hans", "zh-hant", "fr",
    "de", "es-es", "ru", "th", "pt-br",
)
LANGUAGE_ENUM = (
    "English", "Japanese", "Korean", "SimplifiedChinese",
    "TraditionalChinese", "French", "German", "SpanishSpain",
    "Russian", "Thai", "PortugueseBrazil", "Count",
)


@dataclass(frozen=True)
class TgaRaster:
    width: int
    height: int
    alpha: bytearray


def fail(message: str) -> None:
    raise SystemExit(f"F6 localized overlay verification failed: {message}")


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


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        fail(f"cannot read {path}: {error}")
    if not isinstance(value, dict):
        fail(f"JSON root is not an object: {path}")
    return value


def expression_shape(value: ast.expr) -> str:
    return ast.dump(value, annotate_fields=True, include_attributes=False)


def expected_expression(source: str) -> str:
    return expression_shape(ast.parse(source, mode="eval").body)


def find_function(module: ast.Module, name: str) -> ast.FunctionDef:
    for node in module.body:
        if isinstance(node, ast.FunctionDef) and node.name == name:
            return node
    fail(f"overlay generator has no {name} function")


def find_assignment(function: ast.FunctionDef, name: str) -> ast.expr:
    for node in ast.walk(function):
        if (
            isinstance(node, ast.Assign)
            and len(node.targets) == 1
            and isinstance(node.targets[0], ast.Name)
            and node.targets[0].id == name
        ):
            return node.value
    fail(f"{function.name} has no {name} assignment")


def top_level_literals(module: ast.Module) -> dict[str, Any]:
    values: dict[str, Any] = {}
    for node in module.body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        if not isinstance(target, ast.Name):
            continue
        try:
            values[target.id] = ast.literal_eval(node.value)
        except (ValueError, TypeError):
            continue
    return values


def negative_number(value: ast.expr) -> float | None:
    try:
        literal = ast.literal_eval(value)
    except (ValueError, TypeError):
        return None
    if isinstance(literal, (int, float)) and not isinstance(literal, bool):
        return float(literal)
    return None


def localized_key(value: ast.expr) -> str | None:
    if (
        isinstance(value, ast.Subscript)
        and isinstance(value.value, ast.Name)
        and value.value.id == "text"
        and isinstance(value.slice, ast.Constant)
        and isinstance(value.slice.value, str)
    ):
        return value.slice.value
    return None


def draw_slot_calls(function: ast.FunctionDef) -> list[ast.Call]:
    calls = [
        node
        for node in ast.walk(function)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "draw_slot"
    ]
    return sorted(calls, key=lambda call: (call.lineno, call.col_offset))


def explicit_baseline(call: ast.Call) -> ast.expr | None:
    if len(call.args) >= 10:
        return call.args[9]
    for keyword in call.keywords:
        if keyword.arg == "baseline_offset":
            return keyword.value
    return None


def verify_generator_render_contract(path: Path) -> None:
    try:
        module = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    except (OSError, UnicodeError, SyntaxError) as error:
        fail(f"cannot parse overlay generator render contract: {error}")

    constants = top_level_literals(module)
    expected_constants = {
        "REFERENCE_WIDTH": 760,
        "REFERENCE_HEIGHT": 876,
        "SCALE": 2,
        "TEXT_COLOR": (237, 244, 249, 255),
        "SHADOW_COLOR": (8, 26, 38, 150),
        "STROKE_COLOR": (237, 244, 249, 128),
        "STROKE_WIDTH": 1,
    }
    for name, expected in expected_constants.items():
        if constants.get(name) != expected:
            fail(
                f"overlay generator {name} differs: "
                f"expected={expected!r} actual={constants.get(name)!r}"
            )

    target_font_size = find_function(module, "target_font_size")
    expected_assignments = {
        "rounded": "round(32.0 * role_scale)",
        "line_limit": "max(1, int(slot_height // 1.45))",
    }
    for name, expected in expected_assignments.items():
        actual = find_assignment(target_font_size, name)
        if expression_shape(actual) != expected_expression(expected):
            fail(f"target_font_size {name} no longer enforces {expected}")
    returns = [
        node.value
        for node in ast.walk(target_font_size)
        if isinstance(node, ast.Return) and node.value is not None
    ]
    if (
        len(returns) != 1
        or expression_shape(returns[0])
        != expected_expression("max(1, min(rounded, line_limit)) * SCALE")
    ):
        fail("target_font_size return no longer preserves the 32 px/clamped contract")

    fitted_font = find_function(module, "fitted_font")
    fitted_assignments = {
        "maximum_width": "max(1, round((width - 4.0) * SCALE))",
        "maximum_height": "max(1, round((height - 2.0) * SCALE))",
    }
    for name, expected in fitted_assignments.items():
        actual = find_assignment(fitted_font, name)
        if expression_shape(actual) != expected_expression(expected):
            fail(f"fitted_font {name} no longer enforces {expected}")
    getbbox_calls = [
        node
        for node in ast.walk(fitted_font)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "getbbox"
    ]
    if len(getbbox_calls) != 1:
        fail("fitted_font must contain exactly one glyph-bounds check")
    getbbox_keywords = {keyword.arg: keyword.value for keyword in getbbox_calls[0].keywords}
    if (
        "stroke_width" not in getbbox_keywords
        or expression_shape(getbbox_keywords["stroke_width"])
        != expected_expression("STROKE_WIDTH")
    ):
        fail("fitted_font bounds must include STROKE_WIDTH")

    draw_slot = find_function(module, "draw_slot")
    positional = [*draw_slot.args.posonlyargs, *draw_slot.args.args]
    default_names = [argument.arg for argument in positional[-len(draw_slot.args.defaults) :]]
    defaults = dict(zip(default_names, draw_slot.args.defaults))
    if (
        "baseline_offset" not in defaults
        or negative_number(defaults["baseline_offset"]) != -3.0
    ):
        fail("draw_slot default baseline_offset must remain -3.0")
    draw_assignments = {
        "px": "((x + width * 0.5) * SCALE if center else x * SCALE + 1.0)",
        "py": "(y + height * 0.5 + baseline_offset) * SCALE",
        "anchor": '("mm" if center else "lm")',
    }
    for name, expected in draw_assignments.items():
        actual = find_assignment(draw_slot, name)
        if expression_shape(actual) != expected_expression(expected):
            fail(f"draw_slot {name} no longer enforces {expected}")
    text_calls = sorted(
        (
            node
            for node in ast.walk(draw_slot)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and isinstance(node.func.value, ast.Name)
            and node.func.value.id == "draw"
            and node.func.attr == "text"
        ),
        key=lambda call: (call.lineno, call.col_offset),
    )
    if len(text_calls) != 2:
        fail("draw_slot must contain exactly one shadow and one foreground draw")
    shadow_keywords = {keyword.arg: keyword.value for keyword in text_calls[0].keywords}
    foreground_keywords = {
        keyword.arg: keyword.value for keyword in text_calls[1].keywords
    }
    if (
        "fill" not in shadow_keywords
        or expression_shape(shadow_keywords["fill"])
        != expected_expression("SHADOW_COLOR")
        or "stroke_width" in shadow_keywords
        or "stroke_fill" in shadow_keywords
    ):
        fail("draw_slot shadow must remain un-stroked SHADOW_COLOR")
    expected_foreground = {
        "fill": "TEXT_COLOR",
        "stroke_width": "STROKE_WIDTH",
        "stroke_fill": "STROKE_COLOR",
    }
    for name, expected in expected_foreground.items():
        if (
            name not in foreground_keywords
            or expression_shape(foreground_keywords[name])
            != expected_expression(expected)
        ):
            fail(f"draw_slot foreground {name} no longer uses {expected}")

    main_calls = draw_slot_calls(find_function(module, "build_main_overlay"))
    if len(main_calls) != 19:
        fail(f"main overlay draw-call topology differs: {len(main_calls)} != 19")
    explicit_main: dict[str, float] = {}
    for call in main_calls:
        baseline = explicit_baseline(call)
        if baseline is None:
            continue
        if len(call.args) < 3:
            fail("main overlay draw_slot call lacks its text value")
        key = localized_key(call.args[2])
        value = negative_number(baseline)
        if key is None or value is None or key in explicit_main:
            fail("main overlay has an unknown or duplicate explicit baseline")
        explicit_main[key] = value
    if explicit_main != {
        "close": -6.0,
        "language": -2.0,
        "language_name": -4.0,
    }:
        fail(f"main overlay explicit baselines differ: {explicit_main!r}")

    popup_calls = draw_slot_calls(find_function(module, "build_popup_overlay"))
    if len(popup_calls) != 1:
        fail(f"popup overlay draw-call topology differs: {len(popup_calls)} != 1")
    popup_baseline = explicit_baseline(popup_calls[0])
    if popup_baseline is None or negative_number(popup_baseline) != -4.0:
        fail("popup overlay baseline must remain -4.0")


def verify_current_generator_contract(path: Path) -> dict[str, Any]:
    try:
        source = path.read_text(encoding="utf-8")
        module = ast.parse(source, filename=str(path))
    except (OSError, UnicodeError, SyntaxError) as error:
        fail(f"cannot parse current overlay generator contract: {error}")
    constants = top_level_literals(module)
    expected_constants = {
        "REFERENCE_WIDTH": 760,
        "REFERENCE_HEIGHT": 876,
        "SCALE": 2,
        "TEXT_COLOR": (237, 244, 249, 255),
        "SECONDARY_TEXT_COLOR": SECONDARY_TEXT_COLOR,
        "SECONDARY_SLOT_NAMES": SECONDARY_SLOT_NAMES,
        "SHADOW_COLOR": (0, 0, 0, 0),
        "STROKE_COLOR": (237, 244, 249, 128),
        "STROKE_WIDTH": 0,
        "SHADOW_OFFSET": 0,
        "HORIZONTAL_PADDING": 2,
        "VERTICAL_PADDING": 1,
        "MIN_FIT_SCALE": 1.0,
        "FONT_SOURCE": "RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf",
        "EXPECTED_FONT_SHA256": EXPECTED_FONT_SHA256,
        "LATIN_FONT_SOURCE": EXPECTED_LATIN_FONT_SOURCE,
        "EXPECTED_LATIN_FONT_SHA256": EXPECTED_LATIN_FONT_SHA256,
        "LATIN_POPUP_LANGUAGES": LATIN_POPUP_LANGUAGES,
        "LANGUAGE_VALUE_FILES": LANGUAGE_VALUE_FILES,
        "LANGUAGE_VALUE_LAYOUT": LANGUAGE_VALUE_LAYOUT,
    }
    for name, expected in expected_constants.items():
        if constants.get(name) != expected:
            fail(
                f"overlay generator {name} differs: expected={expected!r} "
                f"actual={constants.get(name)!r}"
            )
    if constants.get("MAIN_LAYOUT") != EXPECTED_MAIN_LAYOUT:
        fail("generator main layout differs from the current hub geometry")
    generated = load_generator_module(path)
    if generated.POPUP_LAYOUT != EXPECTED_POPUP_LAYOUT or generated.POPUP_LOCALIZED != EXPECTED_POPUP_LOCALIZED:
        fail("generator popup layout or native endonyms differ")
    localized = generated.LOCALIZED
    if tuple(localized) != LANGUAGE_CODES:
        fail("generator must rasterize all eleven languages in native order")
    target_size = find_function(module, "target_font_size")
    returns = [node.value for node in ast.walk(target_size) if isinstance(node, ast.Return)]
    if len(returns) != 1 or expression_shape(returns[0]) != expected_expression("max(1, round(32.0 * role_scale)) * SCALE"):
        fail("main fonts must keep the same 32-reference base without line-dependent shrinking")

    parse_args_function = find_function(module, "parse_args")
    output_defaults = [
        keyword.value
        for node in ast.walk(parse_args_function)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "add_argument"
        and node.args
        and isinstance(node.args[0], ast.Constant)
        and node.args[0].value == "--output"
        for keyword in node.keywords
        if keyword.arg == "default"
    ]
    if (
        len(output_defaults) != 1
        or expression_shape(output_defaults[0])
        != expected_expression(
            'project_root / "assets" / "ui" / "f6"'
        )
    ):
        fail("generator output is not pinned to assets/ui/f6")

    draw_slot = find_function(module, "draw_slot")
    if "baseline_offset" in {argument.arg for argument in draw_slot.args.args}:
        fail("draw_slot must not apply a hand-tuned baseline offset")
    expected_assignments = {
        "px": "(slot_left + (slot_width - run.width) // 2 "
        "if center else slot_left + HORIZONTAL_PADDING * SCALE)",
        "py": 'slot_top + slot_baseline(font, slot_height) + run.info["baseline_top"]',
    }
    for name, expected in expected_assignments.items():
        if expression_shape(find_assignment(draw_slot, name)) != expected_expression(expected):
            fail(f"draw_slot {name} no longer preserves its authored baseline or alignment")
    baseline = find_function(module, "slot_baseline")
    baseline_returns = [node.value for node in ast.walk(baseline) if isinstance(node, ast.Return)]
    if len(baseline_returns) != 1 or expression_shape(baseline_returns[0]) != expected_expression(
            "(slot_height - ascent - descent) // 2 + ascent"):
        fail("every label must share a font-metric baseline, independent of its glyph bounds")
    composites = [
        node
        for node in ast.walk(draw_slot)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "alpha_composite"
    ]
    if len(composites) != 1:
        fail("draw_slot must alpha-composite exactly one tight glyph run")

    render_run = find_function(module, "render_glyph_run")
    alpha_bounds = [
        node
        for node in ast.walk(render_run)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "getbbox"
        and isinstance(node.func.value, ast.Call)
        and isinstance(node.func.value.func, ast.Attribute)
        and node.func.value.func.attr == "getchannel"
    ]
    crops = [
        node
        for node in ast.walk(render_run)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "crop"
    ]
    if len(alpha_bounds) != 1 or len(crops) != 1:
        fail("render_glyph_run must crop once to actual alpha bounds")
    return localized


def verify_tga(
    path: Path, expected_width: int, expected_height: int
) -> TgaRaster:
    data = path.read_bytes()
    if len(data) < 44:
        fail(f"TGA is too small: {path.name}")
    try:
        (
            id_length,
            color_map_type,
            image_type,
            color_map_first,
            color_map_length,
            color_map_depth,
            x_origin,
            y_origin,
            width,
            height,
            pixel_depth,
            descriptor,
        ) = struct.unpack_from("<BBBHHBHHHHBB", data, 0)
    except struct.error as error:
        fail(f"invalid TGA header in {path.name}: {error}")

    if (
        color_map_type != 0
        or image_type != 10
        or color_map_first != 0
        or color_map_length != 0
        or color_map_depth != 0
        or x_origin != 0
        or y_origin != 0
        or width != expected_width
        or height != expected_height
        or pixel_depth != 32
        or descriptor & 0x0F != 8
    ):
        fail(
            f"{path.name} is not the expected 32-bit RLE true-color TGA "
            f"({width}x{height}, type={image_type}, depth={pixel_depth})"
        )
    if data[-26:] != TGA_FOOTER:
        fail(f"{path.name} lacks the canonical TGA 2.0 footer")

    position = 18 + id_length
    payload_end = len(data) - 26
    expected_pixels = width * height
    decoded_pixels = 0
    transparent_pixels = 0
    visible_pixels = 0
    file_order_alpha = bytearray()
    while decoded_pixels < expected_pixels:
        if position >= payload_end:
            fail(f"{path.name} RLE stream ended before all pixels decoded")
        packet = data[position]
        position += 1
        count = (packet & 0x7F) + 1
        if decoded_pixels + count > expected_pixels:
            fail(f"{path.name} RLE packet exceeds the declared dimensions")
        if packet & 0x80:
            if position + 4 > payload_end:
                fail(f"{path.name} has a truncated RLE pixel")
            alpha = data[position + 3]
            position += 4
            file_order_alpha.extend(bytes((alpha,)) * count)
            if alpha == 0:
                transparent_pixels += count
            else:
                visible_pixels += count
        else:
            byte_count = count * 4
            if position + byte_count > payload_end:
                fail(f"{path.name} has a truncated raw packet")
            for alpha_offset in range(position + 3, position + byte_count, 4):
                alpha = data[alpha_offset]
                file_order_alpha.append(alpha)
                if alpha == 0:
                    transparent_pixels += 1
                else:
                    visible_pixels += 1
            position += byte_count
        decoded_pixels += count

    if position != payload_end:
        fail(f"{path.name} contains bytes after its decoded RLE stream")
    if len(file_order_alpha) != expected_pixels:
        fail(f"{path.name} decoded alpha size differs from its dimensions")
    if transparent_pixels == 0 or visible_pixels == 0:
        fail(f"{path.name} is not a mixed transparent text overlay")

    top_origin = bool(descriptor & 0x20)
    right_origin = bool(descriptor & 0x10)
    visual_alpha = bytearray(expected_pixels)
    for file_y in range(height):
        row_start = file_y * width
        row = file_order_alpha[row_start : row_start + width]
        if right_origin:
            row.reverse()
        visual_y = file_y if top_origin else height - 1 - file_y
        visual_start = visual_y * width
        visual_alpha[visual_start : visual_start + width] = row
    return TgaRaster(width, height, visual_alpha)


def verify_overlay_slots(
    path: Path, raster: TgaRaster, slots: tuple[Slot, ...],
    baseline_bounds: dict[str, tuple[int, int]],
) -> tuple[float, float]:
    allowed = bytearray(raster.width * raster.height)
    scaled_slots: list[tuple[str, int, int, int, int]] = []
    for name, x, y, width, height in slots:
        left = x * 2
        top = y * 2
        right = (x + width) * 2
        bottom = (y + height) * 2
        if (
            left < 0
            or top < 0
            or right > raster.width
            or bottom > raster.height
            or left >= right
            or top >= bottom
        ):
            fail(f"{path.name} slot {name} is outside the overlay canvas")
        scaled_slots.append((name, left, top, right, bottom))
        allowed_row = bytes((1,)) * (right - left)
        for visual_y in range(top, bottom):
            start = visual_y * raster.width + left
            allowed[start : start + right - left] = allowed_row

    for index, alpha in enumerate(raster.alpha):
        if alpha != 0 and allowed[index] == 0:
            x = index % raster.width
            y = index // raster.width
            fail(f"{path.name} has visible pixels outside every slot at ({x},{y})")

    maximum_center_error = 0.0
    minimum_ink_height_ratio = 1.0
    for name, left, top, right, bottom in scaled_slots:
        visible_x: list[int] = []
        visible_y: list[int] = []
        significant_edge_alpha = 0
        for y in range(top, bottom):
            row_start = y * raster.width
            for x in range(left, right):
                alpha = raster.alpha[row_start + x]
                if alpha == 0:
                    continue
                visible_x.append(x)
                visible_y.append(y)
                if x in (left, right - 1) or y in (top, bottom - 1):
                    significant_edge_alpha = max(significant_edge_alpha, alpha)
        if not visible_x:
            fail(f"{path.name} slot {name} has no visible glyph pixels")
        if significant_edge_alpha != 0:
            fail(
                f"{path.name} slot {name} has alpha "
                f"({significant_edge_alpha}) on its clipping edge"
            )
        min_x = min(visible_x)
        max_x = max(visible_x)
        min_y = min(visible_y)
        max_y = max(visible_y)
        expected_top, expected_bottom = baseline_bounds[name]
        vertical_error = max(abs(min_y - top - expected_top),
                             abs(max_y - top - expected_bottom))
        maximum_center_error = max(maximum_center_error, vertical_error)
        if vertical_error > MAX_CENTER_ERROR:
            fail(
                f"{path.name} slot {name} moved off its shared font baseline: "
                f"error={vertical_error:.2f}px"
            )
        if name in CENTERED_SLOT_NAMES:
            horizontal_error = abs(
                (min_x + max_x) / 2.0 - (left + right - 1) / 2.0
            )
            maximum_center_error = max(maximum_center_error, horizontal_error)
            if horizontal_error > MAX_CENTER_ERROR:
                fail(
                    f"{path.name} slot {name} is not horizontally centered: "
                    f"error={horizontal_error:.2f}px"
                )
        elif min_x - left != 4:
            fail(
                f"{path.name} slot {name} left inset differs: "
                f"actual={min_x - left}px expected=4px"
            )
        ink_height = max(visible_y) - min(visible_y) + 1
        slot_height = bottom - top
        ink_height_ratio = ink_height / slot_height
        minimum_ink_height_ratio = min(
            minimum_ink_height_ratio, ink_height_ratio
        )
        if not (
            MIN_SLOT_INK_HEIGHT_RATIO
            <= ink_height_ratio
            <= MAX_SLOT_INK_HEIGHT_RATIO
        ):
            fail(
                f"{path.name} slot {name} ink-height ratio differs: "
                f"{ink_height_ratio:.3f} not in "
                f"[{MIN_SLOT_INK_HEIGHT_RATIO:.2f},"
                f"{MAX_SLOT_INK_HEIGHT_RATIO:.2f}]"
            )
    return maximum_center_error, minimum_ink_height_ratio


def load_generator_module(path: Path):
    spec = importlib.util.spec_from_file_location("f6_verified_generator", path)
    if spec is None or spec.loader is None:
        fail("cannot load the local F6 generator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_generator_localizations(path: Path) -> dict[str, Any]:
    value = load_generator_module(path).LOCALIZED
    if not isinstance(value, dict) or tuple(value) != LANGUAGE_CODES:
        fail("generator must provide all eleven source-derived localizations")
    return value


def flatten_overlay_text(value: dict[str, Any]) -> list[str]:
    try:
        result = [
            value["language_name"],
            value["title"],
            value["language"],
            value["marker_visibility"],
            value["radar"],
            value["map"],
            value["scene"],
            *value["categories"],
            value["height_indicators"],
            value["radar_only"],
            *value["height_categories"],
            value["filter_modes"],
            value["available"],
            value["all"],
            value["close"],
            value["status"],
            value["status_values"]["off"],
            value["status_values"]["on"],
            value["status_values"]["fault"],
            value["status_actions"]["off"],
            value["status_actions"]["on"],
            value["status_actions"]["fault"],
            value["bug_report"],
            value["scene_settings"],
            value["scene_range"],
            value["scene_limit"],
            value["scene_distance"],
            *value["scene_distance_modes"],
            value["restore_defaults"],
            value["all_markers"],
        ]
    except (KeyError, TypeError) as error:
        fail(f"overlay localization shape is incomplete: {error}")
    if len(result) != 43 or not all(isinstance(item, str) and item for item in result):
        fail("overlay localization must contain exactly 43 non-empty source strings")
    return result


def runtime_values_from_block(block: list[str], status: str) -> dict[str, str]:
    """Resolve every visible slot from one complete native language block."""
    if len(block) != 78 or status not in ("off", "on", "fault"):
        fail("runtime slot inventory requires one complete language/status block")
    state = ("off", "on", "fault").index(status)
    values = {
        "language_name": block[0], "title": block[1], "language": block[2],
        "marker_visibility": block[4], "radar": block[5], "map": block[6],
        "height_indicators": block[15], "radar_only": block[16],
        "filter_modes": block[22], "close": block[25], "status": block[26],
        "status_value": block[27 + state], "status_action": block[30 + state],
        "bug_report": block[33], "scene_settings": block[34],
        "scene_range": block[35], "scene_limit": block[36],
        "scene_distance": block[37], "restore_defaults": block[42],
        "marker_all": block[43],
        "scene_treasure": block[9], "scene_area_quest": block[13],
        "scene_mini_game": block[12],
    }
    values.update({f"category_{i}": block[8 + i] for i in range(7)})
    values.update({f"height_category_{i}": block[17 + i] for i in range(5)})
    values.update({f"scene_distance_{i}": block[38 + i] for i in range(4)})
    for index, category_index in enumerate((5, 3)):
        values[f"mode_label_{index}"] = block[8 + category_index]
        values[f"available_{index}"] = block[23]
        values[f"all_{index}"] = block[24]
    return values


def verify_localization_source(
    generator_path: Path, localization_header_path: Path
) -> tuple[str, int]:
    localized = load_generator_localizations(generator_path)
    if tuple(localized) != LANGUAGE_CODES:
        fail("generator must contain all eleven raster localizations")
    try:
        header = localization_header_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read runtime localization header: {error}")
    literals = re.findall(r'L"([^"\\]*)"', header)
    if len(literals) != len(LANGUAGE_CODES) * 78:
        fail(
            "runtime localization table must contain exactly 11 complete "
            "78-string language blocks"
        )
    blocks = {
        language: literals[index * 78 : (index + 1) * 78]
        for index, language in enumerate(LANGUAGE_CODES)
    }
    if any(len(block) != 78 or not all(block) for block in blocks.values()):
        fail("runtime localization table contains an empty or truncated block")
    for language, block in blocks.items():
        for status in ("off", "on", "fault"):
            values = runtime_values_from_block(block, status)
            if (set(values) != {entry[0] for entry in EXPECTED_MAIN_LAYOUT}
                    or not all(values.values())):
                fail(f"{language}/{status} does not cover all 45 visible text slots")
    # Measure the same normal .42 footer face without treating Thai marks or
    # narrow Latin letters as full ems.
    root = localization_header_path.resolve().parents[2]
    cjk = root / ".sdk/RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf"
    latin = root / EXPECTED_LATIN_FONT_SOURCE
    thai = root / "tools/f6-fonts/NotoSansThai-Variable.ttf"
    bug_report_widths = {}
    for language, block in blocks.items():
        font_path = glass_verify.assets.tooltip_font(language, cjk, latin, thai)
        bounds = ImageFont.truetype(str(font_path), 26).getbbox(block[33])
        bug_report_widths[language] = (bounds[2] - bounds[0]) / 2
    widest_bug_report = max(bug_report_widths, key=bug_report_widths.get)
    if bug_report_widths[widest_bug_report] > 202:
        fail("Feedback text exceeds its fixed regular-font footer slot")
    for language in LANGUAGE_CODES:
        expected = flatten_overlay_text(localized[language])
        runtime_block = blocks[language]
        # The fourth runtime string is the retained legacy Automatic label.
        # The explicit-language F6 raster deliberately does not draw it.
        runtime_overlay_text = runtime_block[:3] + runtime_block[4:44]
        if runtime_overlay_text != expected:
            for index, (runtime_value, overlay_value) in enumerate(
                zip(runtime_overlay_text, expected)
            ):
                if runtime_value != overlay_value:
                    fail(
                        f"{language} text #{index} differs between runtime and "
                        f"overlay generator"
                    )
            fail(f"{language} runtime and overlay text counts differ")
    for language, label in EXPECTED_POPUP_LOCALIZED.items():
        if language != "auto" and blocks[language][0] != label:
            fail(f"popup native endonym differs for {language}; do not remove accents")
    return widest_bug_report, bug_report_widths[widest_bug_report]


def normalize_source(value: str) -> str:
    return " ".join(value.split())


def require_source_snippet(source: str, snippet: str, label: str) -> None:
    if normalize_source(snippet) not in normalize_source(source):
        fail(f"runtime hub geometry/scope differs at {label}")


def verify_runtime_geometry_and_scope(
    preferences_path: Path, hub_path: Path
) -> None:
    try:
        preferences = preferences_path.read_text(encoding="utf-8")
        hub = hub_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read runtime geometry/scope source: {error}")
    # Validate the complete panel geometry, including backgrounds and modal
    # hit targets. Text-only checks cannot detect a stale inner-frame bottom.
    expected_geometry = {
        "kReferencePanelWidth": 760.0, "kCardX": 20.0,
        "kCardWidth": 720.0, "kContentX": 36.0,
        "kSectionTitleScale": 0.50,
        "kMarkerTop": 98.0, "kMarkerHeaderY": 108.0,
        "kMarkerAllY": 144.0, "kMarkerRowsY": 168.0, "kMarkerRowStep": 24.0,
        "kMarkerBottom": 344.0, "kSceneTop": 354.0,
        "kSceneTitleY": 364.0, "kSceneChipsY": 402.0,
        "kSceneChipWidth": 222.0, "kSceneChipStep": 234.0,
        "kSceneRangeY": 438.0, "kSceneLimitY": 470.0,
        "kSceneDistanceTitleY": 504.0, "kSceneDistanceY": 524.0,
        "kSceneBottom": 562.0, "kHeightTop": 572.0,
        "kHeightHeaderY": 582.0, "kHeightRowsY": 616.0,
        "kHeightBottom": 688.0, "kFilterTop": 698.0,
        "kFilterHeaderY": 708.0, "kFilterRowsY": 742.0,
        "kFilterOptionsY": 770.0, "kContentBottom": 808.0,
        "kFooterTop": 818.0, "kFooterHeight": 50.0,
        "kFooterButtonY": 828.0, "kFooterButtonWidth": 222.0,
        "kFooterButtonStep": 234.0, "kFooterBottom": 868.0,
        "kReferencePanelHeight": 876.0,
        "kLanguageChoiceWidth": 190.0, "kLanguageChoiceHeight": 32.0,
        "kLanguageChoiceColumnStep": 202.0, "kLanguageChoiceRowStep": 40.0,
        "kLanguageSelectedInset": 2.0, "kLanguageSelectedWidth": 186.0,
        "kLanguageSelectedHeight": 28.0,
        "kLanguageValueX": 126.0, "kLanguageValueY": 58.0,
        "kLanguageValueWidth": 220.0, "kLanguageValueHeight": 26.0,
    }
    resolved = {}
    arithmetic_nodes = (ast.Expression, ast.BinOp, ast.Add, ast.Sub,
                        ast.Mult, ast.Div, ast.Constant, ast.Name, ast.Load)
    for name, expression in re.findall(
        r"constexpr double (\w+)\s*=\s*([^;]+);", hub
    ):
        if name not in expected_geometry:
            continue
        parsed_expression = ast.parse(expression, mode="eval")
        if any(not isinstance(node, arithmetic_nodes)
               for node in ast.walk(parsed_expression)):
            fail(f"non-arithmetic layout constant: {name}")
        try:
            resolved[name] = eval(compile(parsed_expression, "<layout>", "eval"),
                                  {"__builtins__": {}}, resolved)
        except (NameError, TypeError, ValueError) as error:
            fail(f"layout constant cannot resolve: {name}: {error}")
    if resolved != expected_geometry:
        differences = {name: (value, resolved.get(name))
                       for name, value in expected_geometry.items()
                       if value != resolved.get(name)}
        fail(f"runtime section-derived panel geometry differs: {differences}")
    if len(EXPECTED_MAIN_LAYOUT) != 45:
        fail("runtime main-overlay inventory must contain exactly 45 fixed slots")
    for name, x, y, width, height, _, _ in EXPECTED_MAIN_LAYOUT:
        if x < 0 or y < 0 or x + width > 760 or y + height > 868:
            fail(f"text slot escapes the complete content background: {name}")
    for index, (name, x, y, width, height, _, _) in enumerate(EXPECTED_MAIN_LAYOUT):
        for other, ox, oy, ow, oh, _, _ in EXPECTED_MAIN_LAYOUT[index + 1:]:
            if x < ox + ow and x + width > ox and y < oy + oh and y + height > oy:
                fail(f"main text rectangles overlap: {name}/{other}")
    hub_header = hub_path.with_suffix(".hpp").read_text(encoding="utf-8")
    slots_match = re.search(r"enum class LocalizedTextSlot[^\{]*\{([^}]+)\}", hub_header)
    slots = [] if slots_match is None else [s.strip() for s in slots_match[1].split(",") if s.strip()]
    if (len(slots) != 34 or slots[-1] != "Count" or "Scene" in slots
            or slots[-6:-1] != ["SceneTreasure", "SceneAreaQuest", "SceneMiniGame", "GlobalReset", "MarkerAll"]):
        fail("native localized slot inventory must be 33 named + 7 marker + 5 height slots")
    # Each section ends before the next heading, and the last row is above
    # the inner frame. Numeric values and slider hit targets stay disjoint.
    if not (144 + 24 == 168 and 168 + 6 * 24 + 24 < 344 < 354
            and 402 + 30 < 438 and 524 + 30 < 562 < 572
            and 616 + 34 + 28 < 688 < 698 and 770 + 30 < 808 < 818
            and 818 < 828 and 828 + 30 < 868 < 876
            and 36 + 222 < 270 and 270 + 222 < 504 and 504 + 222 < 740
            and 36 + 192 < 244 and 244 + 372 < 634 and 634 + 88 <= 724):
        fail("Scene controls or content sections overlap")
    for viewport_width, viewport_height in ((1280, 720), (1920, 1080),
            (2560, 1440), (3440, 1440), (3840, 2160)):
        reference = min(2.5, max(0.5, min(viewport_width / 1920,
                                         viewport_height / 1080)))
        scale = min(reference, (viewport_width - 32) / 760,
                    (viewport_height - 32) / 876)
        for dpi in (1.0, 1.25, 1.5, 2.0):
            if (760 * (scale / dpi) * dpi > viewport_width - 32 + 0.001
                    or 876 * (scale / dpi) * dpi > viewport_height - 32 + 0.001):
                fail("F6 viewport/DPI fit escapes the requested margins")
    enum_match = re.search(
        r"enum class RadarUiLanguage\s*:\s*std::uint8_t\s*\{([^}]*)\}",
        preferences,
        re.DOTALL,
    )
    if enum_match is None:
        fail("runtime RadarUiLanguage enum is missing")
    enum_values = tuple(
        value.strip()
        for value in enum_match.group(1).split(",")
        if value.strip()
    )
    if enum_values != LANGUAGE_ENUM:
        fail(f"runtime language order differs: {enum_values!r}")

    # Check the selected decoration, not merely the checkbox/text hit boxes.
    # The SG-04 regression used the unrelated 353px marker-header expression.
    choice_width = resolved["kLanguageChoiceWidth"]
    choice_height = resolved["kLanguageChoiceHeight"]
    inset = resolved["kLanguageSelectedInset"]
    selected_width = resolved["kLanguageSelectedWidth"]
    selected_height = resolved["kLanguageSelectedHeight"]
    for scale in (0.5, 0.78, 1.0, 1.5):
        for index in range(len(LANGUAGE_CODES) + 1):
            column, row = index % 3, index // 3
            x = 78.0 + column * resolved["kLanguageChoiceColumnStep"]
            y = 101.0 + row * resolved["kLanguageChoiceRowStep"]
            left, top = (x + inset) * scale, (y + inset) * scale
            right = left + selected_width * scale
            bottom = top + selected_height * scale
            if not (x * scale < left < right < (x + choice_width) * scale
                    and y * scale < top < bottom < (y + choice_height) * scale):
                fail(f"selected popup decoration escapes cell {index} at scale {scale}")
            for other in range(index + 1, len(LANGUAGE_CODES) + 1):
                other_x = (78.0 + (other % 3) * 202.0) * scale
                other_y = (101.0 + (other // 3) * 40.0) * scale
                if (left < other_x + choice_width * scale and right > other_x
                        and top < other_y + choice_height * scale and bottom > other_y):
                    fail("selected popup decoration reaches another language cell")

    snippets = (
        ("add_border(0.0, 0.0, kReferencePanelWidth, kReferencePanelHeight, 0, kPanelFrame)",
         "complete panel frame"),
        ("add_border(1.0, 1.0, kReferencePanelWidth - 2.0, kReferencePanelHeight - 2.0, 1, kPanelBackground)",
         "complete panel background"),
        ("add_border(kCardX, kMarkerTop, kCardWidth, kMarkerBottom - kMarkerTop, 3, kContentBackground)",
         "marker card"),
        ("add_border(kCardX, kSceneTop, kCardWidth, kSceneBottom - kSceneTop, 3, kContentBackground)",
         "Scene card"),
        ("add_border(kCardX, kHeightTop, kCardWidth, kHeightBottom - kHeightTop, 3, kContentBackground)",
         "height card"),
        ("add_border(kCardX, kFilterTop, kCardWidth, kContentBottom - kFilterTop, 3, kContentBackground)",
          "filter card"),
        ("add_border(kCardX, kFooterTop, kCardWidth, kFooterHeight, 3, kContentBackground)",
          "separate footer card"),
        ("add_border(kContentX, kFooterButtonY, kFooterButtonWidth, 30.0, 19, kBugReportButton)",
          "restore footer button"),
        ("add_border(kContentX + kFooterButtonStep, kFooterButtonY, kFooterButtonWidth, 30.0, 19, kBugReportButton)",
          "Endorse footer button"),
        ("add_border(kContentX + 2.0 * kFooterButtonStep, kFooterButtonY, kFooterButtonWidth, 30.0, 19, kBugReportButton)",
          "Feedback footer button"),
        ("add_border(kCardX, 92.0, kCardWidth, kFooterBottom - 92.0, 29, kPopupDim)",
          "full popup dim coverage"),
        ("kCardX, 90.0, kCardWidth, kFooterBottom - 90.0, false, 30",
          "full popup dismiss hit target"),
        ("LocalizedTextSlot::Title, localized.title, 24.0, 12.0, 610.0, 36.0, 5, 0.6875",
          "title"),
        ("LocalizedTextSlot::BugReport, localized.bug_report, 512.0, 831.0, 206.0, 24.0, 21, 0.40625, kTextCenter",
          "Bug Report"),
        ("LocalizedTextSlot::Close, localized.close, 658.0, 17.0, 76.0, 24.0, 21, 0.40625, kTextCenter",
         "Close"),
        ("LocalizedTextSlot::Language, localized.language, 36.0, 59.0, 84.0, 24.0, 6, 0.375",
         "language label"),
        ("LocalizedTextSlot::LanguageValue, language_display.data(), kLanguageValueX, kLanguageValueY, kLanguageValueWidth, kLanguageValueHeight, 6, 0.4375, kTextCenter",
         "language value"),
        ("LocalizedTextSlot::StatusLabel, localized.status, 388.0, 59.0, 90.0, 24.0, 7, 0.375",
         "status label"),
        ("LocalizedTextSlot::StatusValue, status_value, 493.0, 59.0, 102.0, 24.0, 7, 0.40625, kTextCenter",
         "status value"),
        ("LocalizedTextSlot::StatusAction, status_action, 612.0, 59.0, 118.0, 24.0, 7, 0.40625, kTextCenter",
         "status action"),
        ("LocalizedTextSlot::MarkerVisibility, localized.marker_visibility, kContentX, kMarkerHeaderY, 380.0, 28.0, 5, kSectionTitleScale",
         "marker heading"),
        ("LocalizedTextSlot::Radar, localized.radar, 488.0, kMarkerHeaderY + 2.0, 100.0, 26.0, 5, 0.40625, kTextCenter",
         "Radar heading"),
        ("LocalizedTextSlot::Map, localized.map, 614.0, kMarkerHeaderY + 2.0, 100.0, 26.0, 5, 0.40625, kTextCenter",
         "Map heading"),
        ("const double y = kMarkerRowsY + static_cast<double>(row) * kMarkerRowStep",
         "marker rows"),
        ("localized.marker_categories[category_index], kContentX, y, 420.0, 24.0, 5, 0.4375",
         "marker labels"),
        ("const std::array<double, 2> control_x{{516.0, 642.0}}",
         "two-column Radar/Map grid"),
        ("column < control_x.size()",
         "Scene independent from the two-column grid"),
        ("LocalizedTextSlot::SceneSettings, localized.scene_settings, kContentX, kSceneTitleY, 490.0, 28.0, 5, kSectionTitleScale",
         "Scene heading"),
        ("LocalizedTextSlot::GlobalReset, localized.restore_defaults, 44.0, 831.0, 206.0, 24.0, 21, 0.40625, kTextCenter",
          "global restore footer label"),
        ("global_reset_control_ = add_control(kContentX, kFooterButtonY, kFooterButtonWidth, 30.0, false, 22)",
          "global restore hit target"),
        ("endorsement_control_ = add_control(kContentX + kFooterButtonStep, kFooterButtonY, kFooterButtonWidth, 30.0, false, 22)",
          "Endorse footer hit target"),
        ("UObject* bug_report_control = add_control(kContentX + 2.0 * kFooterButtonStep, kFooterButtonY, kFooterButtonWidth, 30.0, false, 22)",
          "Feedback footer hit target"),
        ("const std::array<RadarVisibilityCategory, 3> scene_categories{{ RadarVisibilityCategory::Treasure, RadarVisibilityCategory::AreaQuests, RadarVisibilityCategory::MiniGames}}",
         "Scene category order"),
        ("const std::array<LocalizedTextSlot, 3> scene_category_slots{{ LocalizedTextSlot::SceneTreasure, LocalizedTextSlot::SceneAreaQuest, LocalizedTextSlot::SceneMiniGame}}",
         "three dedicated Scene label slots"),
        ("const double x = kContentX + static_cast<double>(index) * kSceneChipStep",
         "Scene category columns"),
        ("scene_category_slots[index], localized.marker_categories[category], x + 10.0, kSceneChipsY + 3.0, kSceneChipWidth - 20.0, 24.0, 8, 0.40625, kTextCenter",
         "Scene category labels"),
        ("LocalizedTextSlot::SceneRange, localized.scene_range, kContentX, kSceneRangeY, 192.0, 26.0, 5, 0.4375",
         "Scene range label"),
        ("LocalizedTextSlot::SceneLimit, localized.scene_limit, kContentX, kSceneLimitY, 192.0, 26.0, 5, 0.4375",
         "Scene limit label"),
        ("LocalizedTextSlot::SceneDistance, localized.scene_distance, kContentX, kSceneDistanceTitleY, 686.0, 18.0, 5, 0.375",
         "Scene distance label"),
        ("add_widget(slider, 244.0, slider_y[index], 372.0, 26.0, 12)",
         "slider hit targets clear labels and numeric values"),
        ("display.data(), 634.0, slider_y[index], 88.0, 26.0, 7, 0.4375, kTextCenter",
         "Scene numeric value labels"),
        ("const double x = kContentX + static_cast<double>(index) * 174.0",
         "four distance-mode columns"),
        ("distance_slots[index], localized.scene_distance_modes[index], x + 4.0, kSceneDistanceY + 3.0, 158.0, 24.0, 8, 0.40625, kTextCenter",
         "distance-mode labels"),
        ("LocalizedTextSlot::HeightIndicators, localized.height_indicators, kContentX, kHeightHeaderY, 500.0, 28.0, 5, kSectionTitleScale",
         "height heading"),
        ("LocalizedTextSlot::RadarOnly, localized.radar_only, 580.0, kHeightHeaderY + 3.0, 142.0, 22.0, 5, 0.375, kTextCenter",
         "Radar Only"),
        ("const double x = kContentX + static_cast<double>(index % 3U) * kSceneChipStep",
         "height chip columns"),
        ("const double y = kHeightRowsY + static_cast<double>(index / 3U) * 34.0",
         "height chip rows"),
        ("localized.height_categories[index], x + 10.0, y + 2.0, 202.0, 24.0, 8, 0.40625, kTextCenter",
         "height labels"),
        ("LocalizedTextSlot::FilterModes, localized.filter_modes, kContentX, kFilterHeaderY, 620.0, 28.0, 5, kSectionTitleScale",
         "filter heading"),
        ("const double left = kContentX + static_cast<double>(group) * 354.0",
         "two filter groups"),
        ("localized.marker_categories[static_cast<std::size_t>(mode_categories[group])], left, kFilterRowsY, 334.0, 22.0, 5, 0.4375",
         "filter group labels"),
        ("const double x = left + static_cast<double>(option) * 172.0",
         "filter option columns"),
        ("option == 0U ? localized.available : localized.all, x + 4.0, kFilterOptionsY + 3.0, 152.0, 24.0, 8, 0.40625, kTextCenter",
         "filter option labels"),
        ("const double x = 78.0 + static_cast<double>(column) * kLanguageChoiceColumnStep",
         "popup columns"),
        ("const double y = 101.0 + static_cast<double>(row) * kLanguageChoiceRowStep",
         "popup rows"),
        ("choice_label, x + 5.0, y + 4.0, 180.0, 24.0, 35, 0.40625, kTextCenter",
         "popup labels"),
        ("return static_cast<std::size_t>(language) < dswros::kRadarUiLanguageCount;",
         "all-eleven-language main-overlay scope"),
        ("const bool packaged_choice = true;",
         "all twelve popup labels use verified raster text"),
        ("const bool packaged_popup_ready = visible && ensure_language_popup_overlay_unsafe(host_.Get());",
         "popup raster on the English page"),
        ("visible && (!packaged_popup_ready || !packaged_choice) ? kHitTestInvisible : kCollapsed",
         "failed popup image import restores native labels"),
        ("x + kLanguageSelectedInset, y + kLanguageSelectedInset, kLanguageSelectedWidth, kLanguageSelectedHeight, 34, kLanguageSelected",
         "selected popup fill stays inside its own cell"),
    )
    for snippet, label in snippets:
        require_source_snippet(hub, snippet, label)
    verify_language_value_runtime_contract(hub_path, hub)
    verify_regular_text_runtime_contract(hub_path, hub)
    glass_verify.verify_native(hub_path, fail, require_source_snippet, cpp_method_body)


def cpp_method_body(source: str, name: str) -> str:
    marker = f"RadarVisibilityHub::{name}("
    start = source.find(marker)
    if start < 0:
        fail(f"runtime method missing: {name}")
    start = source.find("{", start)
    depth = 0
    for index in range(start, len(source)):
        depth += (source[index] == "{") - (source[index] == "}")
        if depth == 0:
            return source[start:index + 1]
    fail(f"runtime method has no closing body: {name}")


def verify_language_value_runtime_contract(hub_path: Path, hub: str) -> None:
    header = hub_path.with_suffix(".hpp").read_text(encoding="utf-8")
    for snippet in (
        "std::array<RC::Unreal::FWeakObjectPtr, 2> language_value_overlay_textures_{};",
        "std::array<std::uint32_t, 2> language_value_overlay_failures_{};",
    ):
        require_source_snippet(header, snippet, "two bounded optional language-value slots")
    require_source_snippet(hub,
        "language_value_overlay_image, kLanguageValueX, kLanguageValueY, "
        "kLanguageValueWidth, kLanguageValueHeight, 7",
        "value-only image shares the native name rectangle")
    body = cpp_method_body(hub, "refresh_language_value_overlay_unsafe")
    for snippet in (
        "LocalizedTextSlot::LanguageValue)].Get();",
        "text_overlay_active_ ? kCollapsed : kHitTestInvisible",
        "if (text_overlay_active_) return true;",
        "if (resolved_ui_language_ == dswros::RadarUiLanguage::French)",
        "else if (resolved_ui_language_ == dswros::RadarUiLanguage::SpanishSpain)",
        'filename = L"fr-language-value.tga";',
        'filename = L"es-language-value.tga";',
        "auto& failure = language_value_overlay_failures_[index];",
        "if (failure != 0) { text_overlay_failure_ = failure; return true; }",
        "if (!apply_text_overlay_unsafe(image, texture))",
        "set_visibility(image, set_visibility_, kHitTestInvisible);",
    ):
        require_source_snippet(body, snippet, "resolved-language value-only fallback")
    if ("source_language_" in body or "pending_language_" in body
            or "set_native_text_visibility_unsafe" in body
            or "FindAllOf" in body or "StaticFindObject" in body):
        fail("value-only overlay must use resolved AUTO, own one slot and add no discovery")
    apply_index = body.find("if (!apply_text_overlay_unsafe(image, texture))")
    hide_index = body.find("set_visibility(native_value, set_visibility_, kCollapsed);")
    if not (0 <= apply_index < hide_index):
        fail("native language name must remain visible until the image is verified")
    if not (body.find("if (text_overlay_active_) return true;") < body.find("std::size_t index{};") < apply_index):
        fail("successful main text must suppress the duplicate French/Spanish name layer before import")
    apply_body = cpp_method_body(hub, "apply_text_overlay_unsafe")
    require_source_snippet(apply_body,
        'return read_struct_object_property( image, L"Brush", L"ResourceObject") == texture;',
        "image Brush.ResourceObject readback before replacing native text")
    if hub.count("refresh_language_value_overlay_unsafe(") != 4:
        fail("value overlay must have one definition and three edge-only calls")
    for caller in ("open_unsafe", "refresh_localized_text_unsafe", "refresh_mod_status_unsafe"):
        if cpp_method_body(hub, caller).count("refresh_language_value_overlay_unsafe(") != 1:
            fail(f"value overlay call missing from required edge: {caller}")
    reset = cpp_method_body(hub, "reset_runtime_handles")
    for snippet in (
        "language_value_overlay_image_ = FWeakObjectPtr{};",
        "for (auto& texture : language_value_overlay_textures_) { texture = FWeakObjectPtr{}; }",
        "language_value_overlay_failures_ = {};",
    ):
        require_source_snippet(reset, snippet, "value-only weak handles and retry-state cleanup")


def verify_regular_text_runtime_contract(hub_path: Path, hub: str) -> None:
    header = hub_path.with_suffix(".hpp").read_text(encoding="utf-8")
    prefix = re.search(r"packaged_text_overlay_prefix\([^)]*\).*?codes\{\{(.*?)\}\};", hub, re.S)
    if prefix is None or tuple(re.findall(r'L"([^"]+)"', prefix[1])) != LANGUAGE_CODES:
        fail("all eleven main-text filenames must match the resolved-language raster inventory")
    for snippet in (
        "constexpr double kReferenceHubFontSize = 32.0;",
        "kReferenceHubFontSize * unit_scale * role_scale",
        "constexpr std::size_t kNumericGlyphCount = 14;",
        'constexpr wchar_t kNumericCharacters[] = L"0123456789 m-v";',
        "dswros::kRadarTooltipCount + dswros::kRadarConfirmationTextCount + kNumericGlyphCount",
        "const auto images = add_page_images(nullptr, 21);",
        "main_text_overlay_images_[index] = images[index];",
    ):
        require_source_snippet(hub, snippet, "shared regular-font atlas and three retained page fragments")
    advances = re.search(r"kNumericAdvances\{\{(.*?)\}\};", hub, re.S)
    if advances is None or tuple(float(value) for value in advances[1].split(",") if value.strip()) != glass_verify.EXPECTED_NUMERIC_ADVANCES:
        fail("numeric advances must fit the actual unscaled script glyphs")
    for snippet in (
        "std::array<RC::Unreal::FWeakObjectPtr, 3> main_text_overlay_images_{};",
        "std::array<NumericTextRecord, 15> numeric_texts_{};",
    ):
        require_source_snippet(header, snippet, "bounded regular-font consumers")
    main = cpp_method_body(hub, "refresh_packaged_text_overlay_unsafe")
    for snippet in (
        "for (const auto& handle : main_text_overlay_images_)",
        "if (!apply_text_overlay_unsafe(handle.Get(), texture)) return fallback(3);",
        "return set_native_text_visibility_unsafe(true);",
        "if (!set_native_text_visibility_unsafe(false)) return false;",
    ):
        require_source_snippet(main, snippet, "all main fragments require verified texture binding before hiding native text")
    numeric = cpp_method_body(hub, "refresh_numeric_text_unsafe")
    for snippet in (
        "if (!ready) { restore_native(); return false; }",
        "if (!apply_text_overlay_unsafe(record.image.Get(), texture)) return false;",
        "if (length > 6U) { restore_native(); return false; }",
        "if (!found) { restore_native(); return false; }",
        "dswros::kRadarTooltipCount + dswros::kRadarConfirmationTextCount + glyph",
        "cursor + advance * 0.5 - 8.0",
        "index == 14 ? 13U : 12U",
    ):
        require_source_snippet(numeric, snippet, "bounded numeric crops preserve full text or restore native fallback")
    if any(term in numeric for term in ("import_text_overlay", "FindAllOf", "StaticFindObject")):
        fail("numeric changes must reuse the verified atlas without file or object discovery")
    reset = cpp_method_body(hub, "reset_runtime_handles")
    require_source_snippet(reset, "main_text_overlay_images_.fill(FWeakObjectPtr{});", "main raster fragment cleanup")
    require_source_snippet(reset, "for (auto& record : numeric_texts_) record = NumericTextRecord{};", "numeric crop cleanup")


def overlay_values(text: dict[str, Any], status: str) -> dict[str, str]:
    result = {
        "title": text["title"],
        "bug_report": text["bug_report"],
        "close": text["close"],
        "language": text["language"],
        "language_name": text["language_name"],
        "status": text["status"],
        "status_value": text["status_values"][status],
        "status_action": text["status_actions"][status],
        "marker_visibility": text["marker_visibility"],
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
        "scene_treasure": text["categories"][1],
        "scene_area_quest": text["categories"][5],
        "scene_mini_game": text["categories"][4],
        "restore_defaults": text["restore_defaults"],
        "marker_all": text["all_markers"],
    }
    result.update(
        {f"category_{index}": value for index, value in enumerate(text["categories"])}
    )
    result.update(
        {
            f"height_category_{index}": value
            for index, value in enumerate(text["height_categories"])
        }
    )
    for index, category_index in enumerate((5, 3)):
        result[f"mode_label_{index}"] = text["categories"][category_index]
        result[f"available_{index}"] = text["available"]
        result[f"all_{index}"] = text["all"]
    return result


def glyph_signature(
    font: ImageFont.FreeTypeFont, value: str
) -> tuple[tuple[int, int], bytes]:
    mask = font.getmask(value)
    return mask.size, bytes(mask)


def tight_run_pixels(
    font: ImageFont.FreeTypeFont,
    value: str,
    color: tuple[int, int, int, int] = TEXT_COLOR,
) -> Image.Image:
    # Independent expected pixels: one normal-weight text paint, no stroke.
    left, top, right, bottom = font.getbbox(value, anchor="ls")
    image = Image.new("RGBA", (max(1, right-left+8), max(1, bottom-top+8)))
    ImageDraw.Draw(image).text((4-left, 4-top), value, font=font,
                              fill=color, anchor="ls")
    bounds = image.getchannel("A").getbbox()
    if bounds is None:
        fail(f"font rendered no pixels for {value!r}")
    run = image.crop(bounds)
    run.info["baseline_top"] = top + bounds[1] - 4
    return run


def expected_run_top(font: ImageFont.FreeTypeFont, run: Image.Image,
                     slot_height: int) -> int:
    ascent, descent = font.getmetrics()
    return (slot_height - ascent - descent) // 2 + ascent + run.info["baseline_top"]


def expected_baseline_bounds(filename: str, localized: dict[str, Any],
                             cjk: Path, latin: Path) -> dict[str, tuple[int, int]]:
    """Expected ink bounds retain its baseline; descenders do not recenter a row."""
    thai = latin.parent / "NotoSansThai-Variable.ttf"
    if filename == "language-popup.tga":
        runs = [(name, EXPECTED_POPUP_LOCALIZED[name], height, role,
                 glass_verify.assets.tooltip_font(name, cjk, latin, thai))
                for name, _, _, _, height, role, _ in EXPECTED_POPUP_LAYOUT]
    elif filename in LANGUAGE_VALUE_FILES.values():
        language = next(key for key, value in LANGUAGE_VALUE_FILES.items() if value == filename)
        name, _, _, _, height, role, _ = LANGUAGE_VALUE_LAYOUT
        runs = [(name, EXPECTED_POPUP_LOCALIZED[language], height, role, latin)]
    else:
        language, status = filename[:-4].rsplit("-", 1)
        values = overlay_values(localized[language], status)
        font_path = glass_verify.assets.tooltip_font(language, cjk, latin, thai)
        runs = [(name, values[name], height, role, font_path)
                for name, _, _, _, height, role, _ in EXPECTED_MAIN_LAYOUT]
    result = {}
    for name, value, height, role, font_path in runs:
        font = ImageFont.truetype(str(font_path), max(1, round(32 * role)) * 2)
        run = tight_run_pixels(font, value)
        top = expected_run_top(font, run, height * 2)
        result[name] = (top, top + run.height - 1)
    return result


def tight_run_size(font: ImageFont.FreeTypeFont, value: str) -> tuple[int, int]:
    return tight_run_pixels(font, value).size


def verify_font_glyphs_and_fit(
    font_path: Path, latin_font_path: Path, localized: dict[str, Any]
) -> tuple[int, float]:
    if not font_path.is_file() or sha256(font_path) != EXPECTED_FONT_SHA256:
        fail("DroidSansFallback build input is missing or differs from its pin")
    if not latin_font_path.is_file() or sha256(latin_font_path) != EXPECTED_LATIN_FONT_SHA256:
        fail("Liberation Sans build input is missing or differs from its pin")
    license_path = latin_font_path.parent / "LICENSE_LIBERATION"
    if not license_path.is_file() or "SIL OPEN FONT LICENSE Version 1.1" not in license_path.read_text(encoding="utf-8"):
        fail("the build-only Liberation Sans font requires its OFL notice")
    thai_font_path = latin_font_path.parent / "NotoSansThai-Variable.ttf"
    characters = set()
    for language, text in localized.items():
        run_font = glass_verify.assets.tooltip_font(language, font_path, latin_font_path, thai_font_path)
        probe = ImageFont.truetype(str(run_font), 64)
        missing_signature = glyph_signature(probe, "\U0010FFFF")
        required = {c for value in flatten_overlay_text(text) for c in value if not c.isspace()}
        for character in required:
            if glyph_signature(probe, character) == missing_signature or not probe.getmask(character).getbbox():
                fail(f"{language} pinned regular font lacks U+{ord(character):04X}")
        characters.update(required)
    latin_probe = ImageFont.truetype(str(latin_font_path), 64)
    latin_missing = glyph_signature(latin_probe, "\U0010FFFF")
    latin_characters = {
        character for language in LATIN_POPUP_LANGUAGES
        for character in EXPECTED_POPUP_LOCALIZED[language]
        if not character.isspace()
    }
    for character in latin_characters:
        if (glyph_signature(latin_probe, character) == latin_missing
                or not latin_probe.getmask(character).getbbox()):
            fail(f"Liberation Sans lacks required popup glyph U+{ord(character):04X}")
    for accented, plain in (("ç", "c"), ("ñ", "n")):
        if glyph_signature(latin_probe, accented) == glyph_signature(latin_probe, plain):
            fail(f"popup accent must remain visibly distinct: {accented}")

    runs: list[tuple[str, int, int, float, str, Path]] = []
    for language, text in localized.items():
        for status in ("off", "on", "fault"):
            values = overlay_values(text, status)
            for name, _, _, width, height, role_scale, _ in EXPECTED_MAIN_LAYOUT:
                runs.append(
                    (
                        values[name], width, height, role_scale,
                        f"{language}/{status}/{name}", glass_verify.assets.tooltip_font(language, font_path, latin_font_path, thai_font_path),
                    )
                )
    for language, _, _, width, height, role_scale, _ in EXPECTED_POPUP_LAYOUT:
        runs.append(
            (
                EXPECTED_POPUP_LOCALIZED[language],
                width,
                height,
                role_scale,
                f"popup/{language}",
                glass_verify.assets.tooltip_font(language, font_path, latin_font_path, thai_font_path),
            )
        )
    for language in LANGUAGE_VALUE_FILES:
        _, _, _, width, height, role_scale, _ = LANGUAGE_VALUE_LAYOUT
        runs.append((EXPECTED_POPUP_LOCALIZED[language], width, height, role_scale,
                     f"language_value/{language}", latin_font_path))

    minimum_fit = 1.0
    for value, width, height, role_scale, label, run_font in runs:
        target_size = max(1, round(32.0 * role_scale)) * 2
        maximum_width = max(1, round((width - 4.0) * 2))
        maximum_height = max(1, round((height - 2.0) * 2))
        fitted_size = 0
        for size in range(target_size, 1, -1):
            run_width, run_height = tight_run_size(
                ImageFont.truetype(str(run_font), size), value
            )
            if run_width <= maximum_width and run_height <= maximum_height:
                fitted_size = size
                break
        if fitted_size == 0:
            fail(f"{label} cannot fit its slot")
        fit = fitted_size / target_size
        minimum_fit = min(minimum_fit, fit)
        if fit != 1.0:
            fail(f"{label} would require forbidden per-language shrinking: {fit:.3f}")
    return len(set(characters) | latin_characters), minimum_fit


def verify_popup_label_pixels(
    path: Path, font_path: Path, latin_font_path: Path
) -> None:
    # Independent pixel expectation binds each authored native name to its
    # precise font and cell. Correct strings alone cannot detect missing ink.
    actual = Image.open(path).convert("RGBA")
    for language, x, y, width, height, scale, _ in EXPECTED_POPUP_LAYOUT:
        font_input = glass_verify.assets.tooltip_font(language, font_path, latin_font_path, latin_font_path.parent / "NotoSansThai-Variable.ttf")
        value = EXPECTED_POPUP_LOCALIZED[language]
        target_size = max(1, round(32.0 * scale)) * 2
        run = None
        for size in range(target_size, 1, -1):
            candidate = tight_run_pixels(ImageFont.truetype(str(font_input), size), value)
            if candidate.width <= (width - 4) * 2 and candidate.height <= (height - 2) * 2:
                run = candidate
                break
        if run is None:
            fail(f"popup/{language} has no complete fitted text")
        expected = Image.new("RGBA", (width * 2, height * 2))
        font = ImageFont.truetype(str(font_input), target_size)
        expected.alpha_composite(run, ((width * 2 - run.width) // 2,
                                       expected_run_top(font, run, height * 2)))
        crop = actual.crop((x * 2, y * 2, (x + width) * 2, (y + height) * 2))
        if crop.tobytes() != expected.tobytes():
            fail(f"popup/{language} pixels do not preserve the complete native name")


def verify_language_value_pixels(overlay_root: Path, latin_font_path: Path) -> None:
    _, _, _, width, height, scale, _ = LANGUAGE_VALUE_LAYOUT
    font_size = max(1, round(32.0 * scale)) * 2
    font = ImageFont.truetype(str(latin_font_path), font_size)
    for language, filename in LANGUAGE_VALUE_FILES.items():
        actual = Image.open(overlay_root / filename).convert("RGBA")
        expected = Image.new("RGBA", (width * 2, height * 2))
        run = tight_run_pixels(font, EXPECTED_POPUP_LOCALIZED[language])
        expected.alpha_composite(run, ((width * 2 - run.width) // 2,
                                       expected_run_top(font, run, height * 2)))
        if actual.size != expected.size or actual.tobytes() != expected.tobytes():
            fail(f"language_value/{language} pixels omit or alter the native endonym")


def verify_main_label_pixels(overlay_root: Path, localized: dict[str, Any],
                            cjk: Path, latin: Path) -> None:
    """Compare every emitted main glyph to an independent regular-font paint."""
    thai = latin.parent / "NotoSansThai-Variable.ttf"
    for language, text in localized.items():
        font_path = glass_verify.assets.tooltip_font(language, cjk, latin, thai)
        for status in ("off", "on", "fault"):
            actual = Image.open(overlay_root / f"{language}-{status}.tga").convert("RGBA")
            values = overlay_values(text, status)
            for name, x, y, width, height, role, center in EXPECTED_MAIN_LAYOUT:
                font = ImageFont.truetype(str(font_path), max(1, round(32 * role)) * 2)
                color = SECONDARY_TEXT_COLOR if name in SECONDARY_SLOT_NAMES else TEXT_COLOR
                run = tight_run_pixels(font, values[name], color)
                if run.width > (width - 4) * 2 or run.height > (height - 2) * 2:
                    fail(f"{language}/{status}/{name} requires forbidden font shrinking")
                expected = Image.new("RGBA", (width * 2, height * 2))
                px = (width * 2 - run.width) // 2 if center else 4
                expected.alpha_composite(run, (px, expected_run_top(font, run, height * 2)))
                crop = actual.crop((x * 2, y * 2, (x + width) * 2, (y + height) * 2))
                if crop.tobytes() != expected.tobytes():
                    fail(f"{language}/{status}/{name} changes the regular font, weight, size or source text")
        atlas = Image.open(overlay_root / f"tooltip-{language}.tga").convert("RGBA")
        vote = glass_verify.assets.read_confirmation_texts(overlay_root.parents[2])[language][0]
        font = ImageFont.truetype(str(font_path), 26)
        run = tight_run_pixels(font, vote)
        expected = Image.new("RGBA", (412, 48))
        expected.alpha_composite(run, ((412-run.width)//2, expected_run_top(font, run, 48)))
        if atlas.crop((0, 34*144, 412, 34*144+48)).tobytes() != expected.tobytes():
            fail(f"{language} Vote footer must use the same regular 13px role and baseline as its neighboring actions")


def parse_args() -> argparse.Namespace:
    default_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, default=default_root)
    return parser.parse_args()


def main() -> int:
    root = parse_args().project_root.resolve()
    overlay_root = root / "assets" / "ui" / "f6"
    manifest_path = overlay_root / "manifest.json"
    font_path = (
        root
        / ".sdk"
        / "RE-UE4SS"
        / "deps"
        / "fonts"
        / "droid"
        / "DroidSansFallback.ttf"
    )
    latin_font_path = root / EXPECTED_LATIN_FONT_SOURCE
    generator_path = root / "tools" / "Build-F6LocalizedTextOverlays.py"
    localization_header = root / "include" / "dswros" / "radar_localization.hpp"

    localized = verify_current_generator_contract(generator_path)
    bug_report_language, bug_report_width = verify_localization_source(
        generator_path, localization_header
    )
    verify_runtime_geometry_and_scope(
        root / "include" / "dswros" / "radar_preferences.hpp",
        root / "src" / "native" / "radar_visibility_hub.cpp",
    )
    glyph_count, minimum_fit = verify_font_glyphs_and_fit(
        font_path, latin_font_path, localized
    )
    if not overlay_root.is_dir():
        fail(f"overlay directory is missing: {overlay_root}")
    actual_entries = {path.name for path in overlay_root.iterdir() if path.is_file()}
    expected_entries = EXPECTED_FILES | {"manifest.json"}
    if actual_entries != expected_entries:
        fail(
            "overlay file set differs: "
            f"missing={sorted(expected_entries - actual_entries)} "
            f"unexpected={sorted(actual_entries - expected_entries)}"
        )
    if any(path.is_dir() or path.is_symlink() for path in overlay_root.iterdir()):
        fail("overlay directory contains a directory or symbolic link")

    manifest = read_json(manifest_path)
    if (
        manifest.get("schema_version") != 3
        or manifest.get("reference_size") != [760, 876]
        or manifest.get("raster_scale") != 2
        or manifest.get("generator_sha256") != sha256(generator_path)
        or manifest.get("layout_sha256")
        != canonical_sha256(
            {"main": EXPECTED_MAIN_LAYOUT, "popup": EXPECTED_POPUP_LAYOUT,
             "language_value": LANGUAGE_VALUE_LAYOUT}
        )
        or manifest.get("localizations_sha256")
        != canonical_sha256(localized)
        or manifest.get("font_source")
        != "RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf"
        or manifest.get("font_sha256") != EXPECTED_FONT_SHA256
        or manifest.get("latin_font_source") != EXPECTED_LATIN_FONT_SOURCE
        or manifest.get("latin_font_sha256") != EXPECTED_LATIN_FONT_SHA256
        or manifest.get("popup_localizations_sha256") != canonical_sha256(EXPECTED_POPUP_LOCALIZED)
        or manifest.get("verified_codepoint_count") != glyph_count
        or manifest.get("main_languages") != list(LANGUAGE_CODES)
        or manifest.get("main_font_reference_size") != 32
        or manifest.get("main_font_weight") != "regular"
        or manifest.get("main_font_stroke") != 0
        or manifest.get("main_text_alignment") != "font-metric-baseline"
        or manifest.get("main_text_color") != list(TEXT_COLOR)
        or manifest.get("main_secondary_text_color") != list(SECONDARY_TEXT_COLOR)
        or manifest.get("main_secondary_slots") != list(SECONDARY_SLOT_NAMES)
    ):
        fail(
            "manifest identity, geometry, localization, or font pin is invalid"
        )

    entries = manifest.get("files")
    if not isinstance(entries, list) or len(entries) != len(EXPECTED_FILES):
        fail("manifest must contain exactly 53 F6 text, tooltip and skin entries")
    seen: set[str] = set()
    maximum_center_error = 0.0
    minimum_ink_height_ratio = 1.0
    for entry in entries:
        if not isinstance(entry, dict):
            fail("manifest overlay entry is not an object")
        name = entry.get("path")
        if not isinstance(name, str) or name not in EXPECTED_FILES or name in seen:
            fail(f"manifest contains an unsafe, unknown, or duplicate path: {name!r}")
        seen.add(name)
        value_only = name in LANGUAGE_VALUE_FILES.values()
        expected_width, expected_height = (440, 52) if value_only else (EXPECTED_WIDTH, EXPECTED_HEIGHT)
        if name in glass_verify.EXPECTED_SIZES:
            expected_width, expected_height = glass_verify.EXPECTED_SIZES[name]
        if entry.get("width") != expected_width or entry.get("height") != expected_height:
            fail(f"manifest geometry differs for {name}")
        path = overlay_root / name
        if entry.get("sha256") != sha256(path):
            fail(f"manifest SHA-256 differs for {name}")
        raster = verify_tga(path, expected_width, expected_height)
        if name not in TEXT_FILES:
            continue  # Tooltip/skin pixels and complete tile bounds checked below.
        slots = (
            POPUP_OVERLAY_SLOTS
            if name == "language-popup.tga"
            else (LANGUAGE_VALUE_LAYOUT[:5],) if value_only else MAIN_OVERLAY_SLOTS
        )
        center_error, ink_height_ratio = verify_overlay_slots(
            path, raster, slots,
            expected_baseline_bounds(name, localized, font_path, latin_font_path),
        )
        maximum_center_error = max(maximum_center_error, center_error)
        minimum_ink_height_ratio = min(
            minimum_ink_height_ratio, ink_height_ratio
        )
    if seen != EXPECTED_FILES:
        fail(f"manifest overlay set differs: {sorted(EXPECTED_FILES - seen)}")
    verify_popup_label_pixels(overlay_root / "language-popup.tga", font_path, latin_font_path)
    verify_language_value_pixels(overlay_root, latin_font_path)
    verify_main_label_pixels(overlay_root, localized, font_path, latin_font_path)
    tooltip_glyph_count = glass_verify.verify_assets(root, manifest, font_path,
        latin_font_path, fail, canonical_sha256, EXPECTED_MAIN_LAYOUT)

    print(
        "F6 localized overlay verification passed: all 11 runtime language "
        "78-string blocks and 11x3x45 language/status slots audited; "
        "all eleven 45-slot main overlays and twelve popup labels exact; "
        "French/Spanish endonyms and accent pixels verified on every page; "
        f"{glyph_count} required codepoints; minimum fit {minimum_fit:.3f}; "
        f"widest 11-language Bug Report bound "
        f"{bug_report_language}={bug_report_width}/202px; "
        f"maximum baseline/alignment error {maximum_center_error:.1f}px; "
        f"minimum ink-height ratio {minimum_ink_height_ratio:.3f}; "
        f"53 RLE TGAs: 36 fixed text, 11 tooltip/numeric atlases, 6 skins; "
        f"{tooltip_glyph_count} tooltip codepoints across 374 complete safe tiles; "
        f"{manifest['confirmation_verified_codepoint_count']} confirmation codepoints across 77 transparent text tiles; "
        "square checks, Clock-last order, clipped tooltip ownership and source pins exact; "
        "soft blue-gray card transmission 7-11%, gaps 8-11%, "
        "120 text-slot/state checks (45 main, Endorse, 2 values, 12 popup; idle/active), "
        "linear-white small-text contrast at least 4.5:1 (large title 3:1), tooltip opacity 96-99%."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
